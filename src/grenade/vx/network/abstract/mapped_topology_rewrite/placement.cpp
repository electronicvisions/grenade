#include "grenade/vx/network/abstract/mapped_topology_rewrite/placement.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/population.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/mapping/calibrated_neuron.h"
#include "grenade/vx/network/abstract/mapping/chip.h"
#include "grenade/vx/network/abstract/mapping/external_source_neuron.h"
#include "grenade/vx/network/abstract/mapping/locally_placed_neuron.h"
#include "grenade/vx/network/abstract/mapping/poisson_source_neuron.h"
#include "grenade/vx/network/abstract/mapping/uncalibrated_neuron.h"
#include "grenade/vx/network/abstract/population_cell/calibrated.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include "grenade/vx/signal_flow/vertex/chip.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "halco/hicann-dls/vx/v3/background.h"
#include <ctime>
#include <memory>
#include <stdexcept>
#include <boost/range/iterator_range_core.hpp>

namespace grenade::vx::network::abstract {

PlacementRewrite::PlacementRewrite(
    PlacementResult placement_result, std::shared_ptr<grenade::common::LinkedTopology> topology) :
    TopologyRewrite(std::move(topology)), placement_result(std::move(placement_result))
{
}

void PlacementRewrite::operator()() const
{
	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::map<
	        grenade::vx::common::ChipOnConnection, std::set<grenade::common::VertexOnTopology>>>
	    populations_per_chip_per_execution_instance;
	for (auto const& vertex_descriptor : get_topology().get_reference().vertices()) {
		for (auto const& [execution_instance, execution_instance_placement_result] :
		     placement_result.chips) {
			for (auto const& [chip, chip_placement_result] : execution_instance_placement_result) {
				if (chip_placement_result.neuron_anchors.contains(vertex_descriptor) ||
				    chip_placement_result.background_locations.contains(vertex_descriptor) ||
				    chip_placement_result.external_sources.contains(vertex_descriptor)) {
					auto const& partitioned_vertex =
					    dynamic_cast<grenade::common::PartitionedVertex const&>(
					        get_topology().get_reference().get(vertex_descriptor));
					assert(
					    execution_instance ==
					    partitioned_vertex.get_execution_instance_on_executor().value());
					populations_per_chip_per_execution_instance[execution_instance][chip].insert(
					    vertex_descriptor);
				}
			}
		}
	}

	for (auto const& [execution_instance, populations_per_chip] :
	     populations_per_chip_per_execution_instance) {
		for (auto const& [chip, population_descriptors] : populations_per_chip) {
			auto const& chip_placement_result =
			    placement_result.chips.at(execution_instance).at(chip);

			// add populations to mapped graph
			std::optional<grenade::common::VertexOnTopology> crossbar_l2_input;
			std::optional<ExternalSourceNeuronMapping> external_source_neuron_mapping;

			std::map<
			    halco::hicann_dls::vx::v3::BackgroundSpikeSourceOnDLS,
			    grenade::common::VertexOnTopology>
			    background_spike_sources;

			std::map<
			    grenade::common::VertexOnTopology,
			    std::map<
			        halco::hicann_dls::vx::v3::NeuronRowOnDLS, grenade::common::VertexOnTopology>>
			    locally_placed_neuron_populations;

			std::optional<grenade::common::TimeDomainOnTopology> time_domain;

			for (auto const& vertex_descriptor : population_descriptors) {
				auto const& vertex = get_topology().get_reference().get(vertex_descriptor);

				auto const population = dynamic_cast<grenade::common::Population const&>(vertex);
				time_domain = population.get_time_domain();

				// External sources
				if (auto const external_source =
				        dynamic_cast<grenade::vx::network::abstract::ExternalSourceNeuron const*>(
				            &population.get_cell());
				    external_source) {
					if (!crossbar_l2_input) {
						crossbar_l2_input =
						    get_topology().add_vertex(signal_flow::vertex::CrossbarL2Input(
						        true, chip, time_domain.value(), execution_instance));
						external_source_neuron_mapping = ExternalSourceNeuronMapping({});
					} else {
						if (get_topology()
						        .get(*crossbar_l2_input)
						        .get_input_ports()
						        .at(0)
						        .requires_or_generates_data ==
						    grenade::common::Vertex::Port::RequiresOrGeneratesData::no) {
							get_topology().set(
							    *crossbar_l2_input,
							    signal_flow::vertex::CrossbarL2Input(
							        true, chip, time_domain.value(), execution_instance));
							external_source_neuron_mapping = ExternalSourceNeuronMapping({});
						}
					}

				}
				// Background poisson neurons
				else if (auto const poisson_source = dynamic_cast<
				             grenade::vx::network::abstract::PoissonSourceNeuron const*>(
				             &population.get_cell());
				         poisson_source) {
					for (auto const& [hemisphere, padi_bus_on_block] :
					     chip_placement_result.background_locations.at(vertex_descriptor)) {
						auto const coordinate =
						    halco::hicann_dls::vx::v3::BackgroundSpikeSourceOnDLS(
						        padi_bus_on_block +
						        hemisphere.value() *
						            halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock::size);
						background_spike_sources[coordinate] =
						    get_topology().add_vertex(signal_flow::vertex::BackgroundSpikeSource(
						        coordinate, chip, time_domain.value(), execution_instance));
						get_topology().add_inter_graph_hyper_edge(
						    {background_spike_sources.at(coordinate)}, {vertex_descriptor},
						    PoissonSourceNeuronMapping({}, {}));
					}

				}
				// Internal sources
				else if (auto const locally_placed_neuron = dynamic_cast<
				             grenade::vx::network::abstract::LocallyPlacedNeuron const*>(
				             &population.get_cell());
				         locally_placed_neuron) {
					std::map<
					    halco::hicann_dls::vx::v3::NeuronRowOnDLS,
					    std::vector<halco::hicann_dls::vx::v3::NeuronColumnOnDLS>>
					    neuron_locations;

					for (auto const& [_, neuron_anchor] :
					     chip_placement_result.neuron_anchors.at(vertex_descriptor)) {
						auto const placed_compartments =
						    halco::hicann_dls::vx::v3::LogicalNeuronOnDLS(
						        locally_placed_neuron->shape, neuron_anchor)
						        .get_placed_compartments();
						for (auto const& [compartment, atomic_neurons] : placed_compartments) {
							for (auto const& atomic_neuron : atomic_neurons) {
								neuron_locations[atomic_neuron.toNeuronRowOnDLS()].push_back(
								    atomic_neuron.toNeuronColumnOnDLS());
							}
						}
					}

					std::vector<grenade::common::VertexOnTopology> mapped_vertex_descriptors;
					std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, size_t>
					    mapped_vertex_indices;
					for (size_t r = 0; auto const& [row, columns] : neuron_locations) {
						locally_placed_neuron_populations[vertex_descriptor][row] =
						    get_topology().add_vertex(signal_flow::vertex::NeuronView(
						        columns, row, chip, time_domain.value(), execution_instance));
						mapped_vertex_descriptors.push_back(
						    locally_placed_neuron_populations.at(vertex_descriptor).at(row));
						mapped_vertex_indices[row.toHemisphereOnDLS()] = r;
						r++;
					}

					LocallyPlacedNeuronMapping::Anchors anchors;

					// TODO: could be easier if routing result is stored directly per
					// partitioned vertex, possible since the links are in the graph
					auto const& vertex_section =
					    dynamic_cast<grenade::common::Population const&>(
					        get_topology().get_reference().get(vertex_descriptor))
					        .get_shape();
					auto const vertex_index_sequence =
					    grenade::common::CuboidMultiIndexSequence({population.size()})
					        .related_sequence_subset_restriction(
					            population.get_shape(), vertex_section);
					for (size_t i = 0; auto const& index : vertex_index_sequence->get_elements()) {
						anchors[index.value.at(0)] = {
						    mapped_vertex_indices,
						    chip_placement_result.neuron_anchors.at(vertex_descriptor).at(i)};
						i++;
					}

					if (auto const uncalibrated_neuron =
					        dynamic_cast<UncalibratedNeuron const*>(locally_placed_neuron);
					    uncalibrated_neuron) {
						get_topology().add_inter_graph_hyper_edge(
						    std::move(mapped_vertex_descriptors), {vertex_descriptor},
						    UncalibratedNeuronMapping({}, std::move(anchors)));
					} else if (auto const calibrated_neuron =
					               dynamic_cast<CalibratedNeuron const*>(locally_placed_neuron);
					           calibrated_neuron) {
						get_topology().add_inter_graph_hyper_edge(
						    std::move(mapped_vertex_descriptors), {vertex_descriptor},
						    CalibratedNeuronMapping({}, {}, std::move(anchors)));
					}
				}
			}

			std::vector<grenade::common::VertexOnTopology>
			    external_source_neuron_vertex_descriptors;
			external_source_neuron_vertex_descriptors.insert(
			    external_source_neuron_vertex_descriptors.end(),
			    placement_result.chips.at(execution_instance).at(chip).external_sources.begin(),
			    placement_result.chips.at(execution_instance).at(chip).external_sources.end());
			if (crossbar_l2_input) {
				get_topology().add_inter_graph_hyper_edge(
				    {*crossbar_l2_input}, external_source_neuron_vertex_descriptors,
				    *external_source_neuron_mapping);
			}

			if (!locally_placed_neuron_populations.empty()) {
				auto const mapped_vertex_descriptor = get_topology().add_vertex(
				    signal_flow::vertex::Chip(chip, time_domain.value(), execution_instance));
				std::vector<grenade::common::VertexOnTopology> vertex_descriptors;
				for (auto const& [vertex_descriptor, _] : locally_placed_neuron_populations) {
					vertex_descriptors.push_back(vertex_descriptor);
				}
				get_topology().add_inter_graph_hyper_edge(
				    {mapped_vertex_descriptor}, std::move(vertex_descriptors),
				    ChipMapping(std::nullopt));
			}
		}
	}
}

} // namespace grenade::vx::network::abstract
