#include "grenade/vx/network/abstract/mapping/cadc_recorder.h"

#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/common/inter_topology_hyper_edge_on_linked_topology.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/recorder/cadc.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"

namespace grenade::vx::network::abstract {

CADCRecorderMapping::CADCRecorderMapping() {}

bool CADCRecorderMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	for (auto const& mapped_vertex_descriptor : linked_vertex_descriptors) {
		auto const& mapped_vertex = topology.get(mapped_vertex_descriptor);
		if (dynamic_cast<grenade::vx::signal_flow::vertex::CADCMembraneReadoutView const*>(
		        &mapped_vertex) == nullptr) {
			return false;
		}
	}
	if (auto const recorder = dynamic_cast<grenade::vx::network::abstract::CADCRecorder const*>(
	        &dynamic_cast<grenade::common::LinkedTopology const&>(topology.get_reference())
	             .get_reference()
	             .get(reference_vertex_descriptors.at(0)));
	    recorder) {
		return true;
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>>
CADCRecorderMapping::map_output_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        linked_vertex_output_data,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	if (linked_vertex_output_data.empty()) {
		throw std::invalid_argument("Mapping results not possible without mapped vertex results.");
	}
	size_t const batch_size = dynamic_cast<grenade::common::BatchedPortData const&>(
	                              linked_vertex_output_data.at(0).at(0).value().get())
	                              .batch_size();

	auto const& partitioned_vertex = dynamic_cast<CADCRecorder const&>(
	    topology.get_reference().get(reference_vertex_descriptors.at(0)));

	CADCRecorder::Results::Samples model_samples(batch_size);
	for (auto& batch_entry : model_samples) {
		batch_entry.resize(partitioned_vertex.get_shape().size());
	}

	std::vector<std::reference_wrapper<
	    grenade::vx::signal_flow::vertex::CADCMembraneReadoutView::Results const>>
	    mapped_cadc_results;
	for (auto const& mapped_vertex_result : linked_vertex_output_data) {
		mapped_cadc_results.emplace_back(std::cref(
		    dynamic_cast<grenade::vx::signal_flow::vertex::CADCMembraneReadoutView::Results const&>(
		        mapped_vertex_result.at(0).value().get())));
	}

	std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> atomic_neurons;
	std::map<
	    grenade::common::VertexOnTopology,
	    std::pair<grenade::common::Edge, grenade::common::EdgeOnTopology>>
	    mapped_edges;

	for (auto const in_edge_descriptor :
	     topology.get_reference().in_edges(reference_vertex_descriptors.at(0))) {
		auto const source_vertex_descriptor = topology.get_reference().source(in_edge_descriptor);
		auto const& in_edge = topology.get_reference().get(in_edge_descriptor);
		auto const& partitioned_source_population =
		    dynamic_cast<grenade::common::Population const&>(
		        topology.get_reference().get(source_vertex_descriptor));
		auto const target_channels_on_recorder =
		    grenade::common::CuboidMultiIndexSequence({partitioned_vertex.get_input_ports()
		                                                   .at(in_edge.port_on_target)
		                                                   .get_channels()
		                                                   .size()})
		        .related_sequence_subset_restriction(
		            partitioned_vertex.get_input_ports().at(in_edge.port_on_target).get_channels(),
		            in_edge.get_channels_on_target())
		        ->get_elements();
		auto const& internal_source_neuron =
		    dynamic_cast<LocallyPlacedNeuron const&>(partitioned_source_population.get_cell());

		std::set<size_t> cell_on_population_dimensions;
		std::set<size_t> compartment_on_neuron_dimensions;
		std::set<size_t> atomic_neuron_on_compartment_dimensions;
		for (size_t i = 0;
		     auto const& dimension_unit : in_edge.get_channels_on_source().get_dimension_units()) {
			if (dimension_unit &&
			    dynamic_cast<grenade::common::CellOnPopulationDimensionUnit const*>(
			        &(*dimension_unit))) {
				cell_on_population_dimensions.insert(i);
			} else if (
			    dimension_unit &&
			    dynamic_cast<grenade::common::CompartmentOnNeuronDimensionUnit const*>(
			        &(*dimension_unit))) {
				compartment_on_neuron_dimensions.insert(i);
			} else {
				atomic_neuron_on_compartment_dimensions.insert(i);
			}
			i++;
		}

		assert(cell_on_population_dimensions.size() == 1);
		assert(compartment_on_neuron_dimensions.size() == 1);
		assert(atomic_neuron_on_compartment_dimensions.size() == 1);
		size_t const neuron_on_population_dimension = *cell_on_population_dimensions.begin();
		size_t const compartment_on_neuron_dimension = *compartment_on_neuron_dimensions.begin();
		size_t const atomic_neuron_on_compartment_dimension =
		    *atomic_neuron_on_compartment_dimensions.begin();

		std::optional<grenade::common::InterTopologyHyperEdgeOnLinkedTopology>
		    population_inter_topology_hyper_edge_descriptor;
		for (auto const& inter_topology_hyper_edge_descriptor :
		     topology.inter_graph_hyper_edges_by_reference(source_vertex_descriptor)) {
			if (auto const population_inter_topology_hyper_edge =
			        dynamic_cast<LocallyPlacedNeuronMapping const*>(
			            &topology.get(inter_topology_hyper_edge_descriptor));
			    population_inter_topology_hyper_edge) {
				population_inter_topology_hyper_edge_descriptor =
				    inter_topology_hyper_edge_descriptor;
			}
		}

		auto const& population_inter_topology_hyper_edge =
		    dynamic_cast<LocallyPlacedNeuronMapping const&>(
		        topology.get(population_inter_topology_hyper_edge_descriptor.value()));

		auto const& mapped_neuron_views =
		    topology.links(population_inter_topology_hyper_edge_descriptor.value());

		// This maps neurons on the source population to their index on the population,
		// wich might differ (due to previous splitting of a larger population).
		std::map<size_t, size_t> neuron_indices;
		for (size_t neuron_index = 0; auto const& neuron_on_population :
		                              partitioned_source_population.get_output_ports()
		                                  .at(in_edge.port_on_source)
		                                  .get_channels()
		                                  .distinct_projection(cell_on_population_dimensions)
		                                  ->get_elements()) {
			neuron_indices[neuron_on_population.value.at(0)] = neuron_index;
			neuron_index++;
		}

		auto const& in_edge_channels_on_source = in_edge.get_channels_on_source();
		for (size_t population_atomic_neuron_index = 0;
		     auto const& in_edge_channel : in_edge_channels_on_source.get_elements()) {
			auto const& neuron_on_population =
			    in_edge_channel.value.at(neuron_on_population_dimension);
			halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron compartment_on_neuron(
			    in_edge_channel.value.at(compartment_on_neuron_dimension));
			auto const atomic_neuron_on_compartment =
			    in_edge_channel.value.at(atomic_neuron_on_compartment_dimension);

			auto const atomic_neuron =
			    halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
			        internal_source_neuron.shape, population_inter_topology_hyper_edge.anchors
			                                          .at(neuron_indices.at(neuron_on_population))
			                                          .second)
			        .get_placed_compartments()
			        .at(compartment_on_neuron)
			        .at(atomic_neuron_on_compartment);
			atomic_neurons.push_back(atomic_neuron);

			auto const& mapped_neuron_view_descriptor = mapped_neuron_views.at(
			    std::min(mapped_neuron_views.size() - 1, atomic_neuron.toNeuronRowOnDLS().value()));
			auto const& mapped_neuron_view = dynamic_cast<signal_flow::vertex::NeuronView const&>(
			    topology.get(mapped_neuron_view_descriptor));
			size_t const column = std::distance(
			    mapped_neuron_view.get_columns().begin(),
			    std::find(
			        mapped_neuron_view.get_columns().begin(),
			        mapped_neuron_view.get_columns().end(), atomic_neuron.toNeuronColumnOnDLS()));
			assert(column < mapped_neuron_view.get_columns().size());
			mapped_edges.emplace(
			    mapped_neuron_view_descriptor,
			    std::pair{
			        grenade::common::Edge(
			            grenade::common::ListMultiIndexSequence(
			                {grenade::common::MultiIndex({column})}),
			            grenade::common::ListMultiIndexSequence(
			                {grenade::common::MultiIndex({population_atomic_neuron_index})})),
			        in_edge_descriptor});
			population_atomic_neuron_index++;
		}
	}

	std::map<
	    halco::hicann_dls::vx::v3::SynramOnDLS,
	    std::map<signal_flow::vertex::CADCMembraneReadoutView::Columns::value_type, size_t>>
	    reverse_lookup;
	for (size_t i = 0; auto const& an : atomic_neurons) {
		reverse_lookup[an.toNeuronRowOnDLS().toSynramOnDLS()]
		              [an.toNeuronColumnOnDLS().toSynapseOnSynapseRow()] = i;
		i++;
	}

	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		auto const& linked_vertex =
		    dynamic_cast<signal_flow::vertex::CADCMembraneReadoutView const&>(
		        topology.get(linked_vertex_descriptors.at(i)));
		auto const& local_reverse_lookup = reverse_lookup.at(linked_vertex.get_synram());
		auto const& mapped_cadc_result = mapped_cadc_results.at(i).get();
		for (size_t b = 0; b < batch_size; ++b) {
			auto& model_samples_batch_entry = model_samples.at(b);
			auto const& mapped_samples_batch_entry = mapped_cadc_result.samples.at(b);
			for (auto const& mapped_sample : mapped_samples_batch_entry) {
				for (size_t j = 0; j < mapped_sample.data.size(); ++j) {
					model_samples_batch_entry
					    .at(local_reverse_lookup.at(linked_vertex.get_columns().at(j)))
					    .push_back(std::make_pair(mapped_sample.time, mapped_sample.data.at(j)));
				}
			}
		}
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ret(1);
	ret.back().emplace_back(std::make_unique<CADCRecorder::Results>(std::move(model_samples)));
	return ret;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> CADCRecorderMapping::copy() const
{
	return std::make_unique<CADCRecorderMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> CADCRecorderMapping::move()
{
	return std::make_unique<CADCRecorderMapping>(std::move(*this));
}

std::ostream& CADCRecorderMapping::print(std::ostream& os) const
{
	return os << "CADCRecorderMapping()";
}

bool CADCRecorderMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return InterTopologyHyperEdge::is_equal_to(other);
}

} // namespace grenade::vx::network::abstract
