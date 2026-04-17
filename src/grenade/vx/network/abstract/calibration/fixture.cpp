#include "grenade/vx/network/abstract/calibration/fixture.h"

#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/abstract/mapping/calibrated_neuron.h"
#include "grenade/vx/network/abstract/mapping/chip.h"
#include "grenade/vx/network/abstract/population_cell/calibrated.h"
#include "grenade/vx/signal_flow/vertex/chip.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/variant.h"
#include <boost/iterator/counting_iterator.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace grenade::vx::network::abstract {

void FixtureCalibration::operator()(
    grenade::common::LinkedTopology& topology,
    grenade::vx::execution::JITGraphExecutor& /* executor */) const
{
	using namespace grenade::common;

	// add calibration result to mapping
	for (auto const& inter_graph_hyper_edge_descriptor : topology.inter_graph_hyper_edges()) {
		auto const& inter_topology_hyper_edge = topology.get(inter_graph_hyper_edge_descriptor);
		// skip if not part of calibration
		std::unordered_set<std::type_index> calibrated_inter_topology_hyper_edges{
		    std::type_index(typeid(CalibratedNeuronMapping)), std::type_index(typeid(ChipMapping))};
		if (!calibrated_inter_topology_hyper_edges.contains(
		        std::type_index(typeid(inter_topology_hyper_edge)))) {
			continue;
		}

		auto inter_graph_hyper_edge_copy = inter_topology_hyper_edge.copy();
		if (auto const calibrated_ptr =
		        dynamic_cast<CalibratedNeuronMapping*>(&(*inter_graph_hyper_edge_copy));
		    calibrated_ptr) {
			auto const& reference_vertex_descriptor =
			    topology.references(inter_graph_hyper_edge_descriptor).at(0);
			auto const& reference_vertex = dynamic_cast<grenade::common::Population const&>(
			    topology.get_reference().get(reference_vertex_descriptor));
			auto const& reference_neuron =
			    dynamic_cast<CalibratedNeuron const&>(reference_vertex.get_cell());
			auto const& execution_instance_on_executor =
			    reference_vertex.get_execution_instance_on_executor().value();
			for (auto const& mapped_vertex_descriptor :
			     topology.links(inter_graph_hyper_edge_descriptor)) {
				auto const& mapped_vertex = dynamic_cast<signal_flow::vertex::NeuronView const&>(
				    topology.get(mapped_vertex_descriptor));
				auto const& chip_on_connection = mapped_vertex.chip_on_connection;
				if (!chips.contains(execution_instance_on_executor) ||
				    !chips.at(execution_instance_on_executor).contains(chip_on_connection)) {
					std::stringstream ss;
					ss << "Expected calibration result for " << execution_instance_on_executor
					   << ", " << chip_on_connection
					   << " which is not present in FixtureCalibration.";
					throw std::invalid_argument(ss.str());
				}
				auto const& local_calibration_result = chips.at(execution_instance_on_executor);
				for (size_t cell_on_population = 0; cell_on_population < reference_vertex.size();
				     ++cell_on_population) {
					calibrated_ptr->configs.push_back({});
					halco::hicann_dls::vx::v3::LogicalNeuronOnDLS logical_neuron_on_dls(
					    reference_neuron.shape,
					    calibrated_ptr->anchors.at(cell_on_population).second);
					for (auto const& [compartment_on_neuron, atomic_neurons] :
					     logical_neuron_on_dls.get_placed_compartments()) {
						for (size_t an = 0; an < atomic_neurons.size(); ++an) {
							calibrated_ptr->configs
							    .at(cell_on_population)[grenade::common::CompartmentOnNeuron(
							        compartment_on_neuron)]
							    .push_back(
							        local_calibration_result.at(chip_on_connection)
							            .neuron_block.atomic_neurons.at(atomic_neurons.at(an)));
						}
					}
				}
			}
		} else if (auto const chip_ptr =
		               dynamic_cast<ChipMapping*>(&(*inter_graph_hyper_edge_copy));
		           chip_ptr) {
			auto const& references = topology.references(inter_graph_hyper_edge_descriptor);
			if (std::any_of(references.begin(), references.end(), [&](auto const& reference) {
				    auto const& population =
				        dynamic_cast<Population const&>(topology.get_reference().get(reference));
				    return dynamic_cast<CalibratedNeuron const*>(&population.get_cell());
			    })) {
				for (auto const& mapped_vertex_descriptor :
				     topology.links(inter_graph_hyper_edge_descriptor)) {
					auto const& mapped_vertex = dynamic_cast<signal_flow::vertex::Chip const&>(
					    topology.get(mapped_vertex_descriptor));
					auto const& execution_instance_on_executor =
					    mapped_vertex.get_execution_instance_on_executor().value();
					auto const& chip_on_connection = mapped_vertex.chip_on_connection;
					if (!chips.contains(execution_instance_on_executor) ||
					    !chips.at(execution_instance_on_executor).contains(chip_on_connection)) {
						std::stringstream ss;
						ss << "Expected calibration result for " << execution_instance_on_executor
						   << ", " << chip_on_connection
						   << " which is not present in FixtureCalibration.";
						throw std::invalid_argument(ss.str());
					}
					chip_ptr->base =
					    chips.at(execution_instance_on_executor).at(chip_on_connection);
				}
			}
		}
		topology.set(inter_graph_hyper_edge_descriptor, std::move(*inter_graph_hyper_edge_copy));
	}
}

} // namespace grenade::vx::network::abstract
