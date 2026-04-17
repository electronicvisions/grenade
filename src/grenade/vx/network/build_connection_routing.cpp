#include "grenade/vx/network/build_connection_routing.h"

#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/projection_connector/static.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"
#include "hate/math.h"
#include "lola/vx/v3/synapse.h"
#include <set>
#include <stdexcept>

namespace grenade::vx::network {

ConnectionRoutingResult build_connection_routing(
    grenade::common::LinkedTopology const& topology,
    std::vector<grenade::common::VertexOnTopology> const& local_vertex_descriptors)
{
	ConnectionRoutingResult result;

	// distribute weight onto multiple synapses
	// we split the weight like: 123 -> 63 + 60
	// we distribute over all neurons in every compartment
	std::map<std::tuple<size_t, grenade::common::CompartmentOnNeuron, size_t>, size_t> in_degree;
	for (auto const& vertex_descriptor : local_vertex_descriptors) {
		if (auto const* projection = dynamic_cast<grenade::common::Projection const*>(
		        &topology.get_reference().get(vertex_descriptor));
		    projection) {
			auto const static_connector =
			    dynamic_cast<grenade::common::StaticConnector const*>(&projection->get_connector());
			if (!static_connector) {
				throw std::runtime_error("Connector type not supported.");
			}
			auto const projection_output_ports = projection->get_output_ports();
			auto& local_result = result[vertex_descriptor];
			auto const projection_input_sequence = projection->get_connector().get_input_sequence();
			auto const projection_output_sequence =
			    projection->get_connector().get_output_sequence();
			auto const rectangular_section =
			    projection_input_sequence->cartesian_product(*projection_output_sequence);
			auto const section = static_connector->get_synapse_connections(*rectangular_section);
			local_result.resize(section->size());

			std::map<size_t, std::reference_wrapper<abstract::LocallyPlacedNeuron const>>
			    post_neurons;
			std::map<size_t, grenade::common::CompartmentOnNeuron> post_neuron_compartments;
			std::map<size_t, grenade::common::ReceptorOnCompartment> post_neuron_receptors;

			for (auto const& out_edge_descriptor :
			     topology.get_reference().out_edges(vertex_descriptor)) {
				auto const& target_vertex_descriptor =
				    topology.get_reference().target(out_edge_descriptor);
				if (auto const* target_population =
				        dynamic_cast<grenade::common::Population const*>(
				            &topology.get_reference().get(target_vertex_descriptor));
				    target_population) {
					auto const& target_neuron = dynamic_cast<abstract::LocallyPlacedNeuron const&>(
					    target_population->get_cell());
					auto const& out_edge = topology.get_reference().get(out_edge_descriptor);
					auto const post_neuron_indices =
					    grenade::common::CuboidMultiIndexSequence(
					        {projection_output_ports.at(out_edge.port_on_source)
					             .get_channels()
					             .size()})
					        .related_sequence_subset_restriction(
					            projection_output_ports.at(out_edge.port_on_source).get_channels(),
					            out_edge.get_channels_on_source());
					auto const local_post_neuron_elements =
					    out_edge.get_channels_on_target().get_elements();
					std::set<size_t> output_compartment_on_neuron_dimensions;
					std::set<size_t> output_receptor_on_compartment_dimensions;
					for (size_t i = 0; i < out_edge.get_channels_on_target().dimensionality();
					     ++i) {
						if (dynamic_cast<grenade::common::CompartmentOnNeuronDimensionUnit const*>(
						        &(*out_edge.get_channels_on_target().get_dimension_units().at(
						            i)))) {
							output_compartment_on_neuron_dimensions.insert(i);
						} else if (dynamic_cast<
						               grenade::common::ReceptorOnCompartmentDimensionUnit const*>(
						               &(*out_edge.get_channels_on_target()
						                      .get_dimension_units()
						                      .at(i)))) {
							output_receptor_on_compartment_dimensions.insert(i);
						}
					}
					assert(output_compartment_on_neuron_dimensions.size() == 1);
					assert(output_receptor_on_compartment_dimensions.size() == 1);
					for (size_t i = 0;
					     auto const& post_neuron_index : post_neuron_indices->get_elements()) {
						post_neurons.emplace(post_neuron_index.value.at(0), target_neuron);
						post_neuron_compartments.emplace(
						    post_neuron_index.value.at(0),
						    grenade::common::CompartmentOnNeuron(
						        local_post_neuron_elements.at(i).value.at(
						            *output_compartment_on_neuron_dimensions.begin())));
						post_neuron_receptors.emplace(
						    post_neuron_index.value.at(0),
						    grenade::common::ReceptorOnCompartment(
						        local_post_neuron_elements.at(i).value.at(
						            *output_receptor_on_compartment_dimensions.begin())));
						i++;
					}
				}
			}

			auto const synapse_parameterization_section =
			    projection->get_connector().get_synapse_parameterization_indices(
			        *rectangular_section);

			auto const section_elements = section->get_elements();

			std::set<size_t> output_dimensions;
			std::set<size_t> output_cell_on_population_dimensions;
			std::set<size_t> output_compartment_on_neuron_dimensions;
			for (size_t i = projection_input_sequence->dimensionality();
			     i < section->dimensionality(); ++i) {
				output_dimensions.insert(i);
			}
			std::vector<grenade::common::MultiIndex> synapse_post_section_elements;
			for (auto const& section_element : section_elements) {
				grenade::common::MultiIndex synapse_post_section_element;
				for (auto const& output_dimension : output_dimensions) {
					synapse_post_section_element.value.push_back(
					    section_element.value.at(output_dimension));
				}
				synapse_post_section_elements.push_back(synapse_post_section_element);
			}
			std::vector<grenade::common::MultiIndex>
			    synapse_post_section_cell_on_population_elements;
			for (auto const& synapse_post_section_element : synapse_post_section_elements) {
				grenade::common::MultiIndex element;
				for (auto const& dimension : output_cell_on_population_dimensions) {
					element.value.push_back(synapse_post_section_element.value.at(
					    dimension + projection_input_sequence->dimensionality()));
				}
				synapse_post_section_cell_on_population_elements.push_back(element);
			}
			auto const output_sequence_cell_on_population =
			    projection_output_sequence->projection(output_cell_on_population_dimensions)
			        ->get_elements();
			auto const projection_output_sequence_elements =
			    projection_output_sequence->get_elements();

			std::vector<std::map<grenade::common::ReceptorOnCompartment, size_t>> num_synapses(
			    local_result.size());
			if (auto const* uncalibrated_synapse =
			        dynamic_cast<abstract::UncalibratedSynapse const*>(&projection->get_synapse());
			    uncalibrated_synapse) {
				auto const& parameter_space =
				    dynamic_cast<abstract::UncalibratedSynapse::ParameterSpace const&>(
				        projection->get_synapse_parameter_space());

				for (size_t s = 0;
				     auto const& synapse_index : synapse_parameterization_section->get_elements()) {
					auto const& max_weight =
					    parameter_space.max_weights.at(synapse_index.value.at(0));
					size_t const post_neuron_index = std::distance(
					    projection_output_sequence_elements.begin(),
					    std::find(
					        projection_output_sequence_elements.begin(),
					        projection_output_sequence_elements.end(),
					        synapse_post_section_elements.at(s)));
					// find neurons with matching receptor
					auto const post_receptor_on_compartment =
					    post_neuron_receptors.at(post_neuron_index);
					num_synapses.at(s)[post_receptor_on_compartment] = std::max(
					    hate::math::round_up_integer_division(
					        max_weight.value(), lola::vx::v3::SynapseMatrix::Weight::max),
					    static_cast<size_t>(1));
					s++;
				}
			} else if (auto const* uncalibrated_signed_synapse =
			               dynamic_cast<abstract::UncalibratedSignedSynapse const*>(
			                   &projection->get_synapse());
			           uncalibrated_signed_synapse) {
				auto const& parameter_space =
				    dynamic_cast<abstract::UncalibratedSignedSynapse::ParameterSpace const&>(
				        projection->get_synapse_parameter_space());

				for (size_t s = 0;
				     auto const& synapse_index : synapse_parameterization_section->get_elements()) {
					auto const& max_weight =
					    parameter_space.max_weights.at(synapse_index.value.at(0));
					num_synapses.at(s)[uncalibrated_signed_synapse->excitatory_receptor] = std::max(
					    hate::math::round_up_integer_division(
					        max_weight.value(), lola::vx::v3::SynapseMatrix::Weight::max),
					    static_cast<size_t>(1));
					num_synapses.at(s)[uncalibrated_signed_synapse->inhibitory_receptor] = std::max(
					    hate::math::round_up_integer_division(
					        max_weight.value(), lola::vx::v3::SynapseMatrix::Weight::max),
					    static_cast<size_t>(1));
					s++;
				}
			}
			if (num_synapses.empty()) {
				continue;
			}

			size_t max_num_synapses = 0;
			for (auto const& num_synapses_per_receptor : num_synapses) {
				for (auto const& [_, num] : num_synapses_per_receptor) {
					max_num_synapses = std::max(max_num_synapses, num);
				}
			}
			for (size_t p = 0; p < max_num_synapses; ++p) {
				std::vector<size_t> indices;
				for (size_t i = 0; i < num_synapses.size(); ++i) {
					for (auto const& [receptor, num] : num_synapses.at(i)) {
						// only find new hardware synapse, if the current connection requires
						// another one
						if (num <= p) {
							continue;
						}
						size_t const post_neuron_index = std::distance(
						    projection_output_sequence_elements.begin(),
						    std::find(
						        projection_output_sequence_elements.begin(),
						        projection_output_sequence_elements.end(),
						        synapse_post_section_elements.at(i)));
						assert(post_neuron_index < projection_output_sequence_elements.size());
						// find neurons with matching receptor
						auto const post_compartment_on_neuron =
						    post_neuron_compartments.at(post_neuron_index);
						auto const& receptors = post_neurons.at(post_neuron_index)
						                            .get()
						                            .compartments.at(post_compartment_on_neuron)
						                            .receptors;
						std::set<size_t> neurons_with_matching_receptor;
						for (size_t i = 0; i < receptors.size(); ++i) {
							if (receptors.at(i).contains(receptor)) {
								neurons_with_matching_receptor.insert(i);
							}
						}
						if (neurons_with_matching_receptor.empty()) {
							throw std::runtime_error("No neuron on compartment features receptor "
							                         "requested by projection.");
						}
						// FIXME: reimplement functionality for background source hemispheres
						// choose neuron with matching receptor and smallest current in_degree
						auto const neuron_on_compartment = *std::min_element(
						    neurons_with_matching_receptor.begin(),
						    neurons_with_matching_receptor.end(),
						    [&in_degree, post_neuron_index, post_compartment_on_neuron](
						        auto const& a, auto const& b) {
							    return in_degree[std::make_tuple(
							               post_neuron_index, post_compartment_on_neuron, a)] <
							           in_degree[std::make_tuple(
							               post_neuron_index, post_compartment_on_neuron, b)];
						    });
						// choice performed, in_degree++ of chosen neuron
						in_degree[std::make_tuple(
						    post_neuron_index, post_compartment_on_neuron,
						    neuron_on_compartment)]++;

						local_result.at(i).atomic_neurons_on_target_compartment[receptor].push_back(
						    neuron_on_compartment);
						indices.push_back(i);
					}
				}
			}
		}
	}
	return result;
}

} // namespace grenade::vx::network
