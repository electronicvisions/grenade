#include "grenade/vx/network/abstract/topology_rewrite/population_execution_instance_transition.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/edge_on_topology.h"
#include "grenade/common/inter_topology_hyper_edge/recorder.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/recorder.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/plasticity_rule.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include <memory>

namespace grenade::vx::network::abstract {

PopulationExecutionInstanceTransitionRewrite::PopulationExecutionInstanceTransitionRewrite(
    std::shared_ptr<grenade::common::LinkedTopology> topology) :
    TopologyRewrite(std::move(topology))
{
}

void PopulationExecutionInstanceTransitionRewrite::operator()() const
{
	using namespace grenade::common;

	// assign new time domains
	std::set<TimeDomainOnTopology> present_time_domains;
	std::map<
	    TimeDomainOnTopology,
	    std::map<ExecutionInstanceOnExecutor, std::unordered_set<VertexOnTopology>>>
	    vertices_per_execution_instance_per_time_domain;
	for (auto const vertex_descriptor : get_topology().vertices()) {
		auto const& vertex = get_topology().get(vertex_descriptor);
		auto const& time_domain = vertex.get_time_domain();
		if (time_domain) {
			present_time_domains.insert(*time_domain);
			vertices_per_execution_instance_per_time_domain
			    [*time_domain][dynamic_cast<PartitionedVertex const&>(vertex)
			                       .get_execution_instance_on_executor()
			                       .value()]
			        .insert(vertex_descriptor);
		}
	}

	TimeDomainOnTopology new_time_domain;
	if (!present_time_domains.empty()) {
		new_time_domain = *present_time_domains.rbegin() + TimeDomainOnTopology(1);
	}

	// store and clear inter topology time domain edges
	auto const inter_topology_time_domain_edges = get_topology().inter_topology_time_domain_edges;
	get_topology().inter_topology_time_domain_edges.clear();

	// assign unique time domain per execution instance
	Topology::Vertices new_vertices;
	for (auto const& [time_domain, vertices_per_execution_instance] :
	     vertices_per_execution_instance_per_time_domain) {
		for (auto const& [execution_instance, vertices] : vertices_per_execution_instance) {
			for (auto const& vertex_descriptor : vertices) {
				auto vertex = get_topology().get(vertex_descriptor).copy();
				if (auto const population = dynamic_cast<Population*>(&(*vertex)); population) {
					population->set_time_domain(new_time_domain);
				} else if (auto const projection = dynamic_cast<Projection*>(&(*vertex));
				           projection) {
					projection->set_time_domain(new_time_domain);
				} else if (auto const recorder = dynamic_cast<Recorder*>(&(*vertex)); recorder) {
					recorder->set_time_domain(new_time_domain);
				} else if (auto const plasticity_rule = dynamic_cast<PlasticityRule*>(&(*vertex));
				           plasticity_rule) {
					plasticity_rule->set_time_domain(new_time_domain);
				}
				new_vertices.set(vertex_descriptor, std::move(*vertex));
			}

			get_topology().inter_topology_time_domain_edges.emplace(
			    new_time_domain, inter_topology_time_domain_edges.at(time_domain));
			new_time_domain += TimeDomainOnTopology(1);
		}
	}

	get_topology().set(std::move(new_vertices));

	// add spike recorder and external source neuron population at execution instance transitions
	for (auto const vertex_descriptor : get_topology().vertices()) {
		if (auto const population =
		        dynamic_cast<Population const*>(&(get_topology().get(vertex_descriptor)));
		    population) {
			// FIXME: do this for locally-placed and background and delay, but just copy external
			// population
			auto const population_execution_instance =
			    population->get_execution_instance_on_executor();
			auto const population_time_domain = population->get_time_domain();
			std::vector<EdgeOnTopology> out_edges(
			    get_topology().out_edges(vertex_descriptor).begin(),
			    get_topology().out_edges(vertex_descriptor).end());
			for (auto const& out_edge_descriptor : out_edges) {
				auto const target_descriptor = get_topology().target(out_edge_descriptor);
				auto const& target =
				    dynamic_cast<PartitionedVertex const&>(get_topology().get(target_descriptor));
				auto const target_execution_instance = target.get_execution_instance_on_executor();
				auto const target_time_domain = target.get_time_domain();

				if (target_execution_instance == population_execution_instance) {
					continue;
				}

				auto const out_edge = get_topology().get(out_edge_descriptor).copy();

				std::set<size_t> neuron_dimensions;
				std::set<size_t> compartment_dimensions;
				for (size_t i = 0; i < out_edge->get_channels_on_source().dimensionality(); ++i) {
					if (dynamic_cast<CellOnPopulationDimensionUnit const*>(
					        &(*out_edge->get_channels_on_source().get_dimension_units().at(i)))) {
						neuron_dimensions.insert(i);
					} else if (dynamic_cast<CompartmentOnNeuronDimensionUnit const*>(
					               &(*out_edge->get_channels_on_source().get_dimension_units().at(
					                   i)))) {
						compartment_dimensions.insert(i);
					}
				}
				assert(compartment_dimensions.size() == 1);

				auto const out_edge_cell_channels_on_source =
				    out_edge->get_channels_on_source().projection(neuron_dimensions);

				get_topology().remove_edge(out_edge_descriptor);

				// create spike recorder on population execution instance if it is on a time domain
				std::optional<VertexOnTopology> spike_recorder_descriptor;
				if (population_time_domain) {
					SpikeRecorder spike_recorder(
					    out_edge->get_channels_on_source(), *population_time_domain);
					spike_recorder.set_execution_instance_on_executor(
					    population_execution_instance);

					spike_recorder_descriptor =
					    get_topology().add_vertex(std::move(spike_recorder));

					get_topology().add_edge(
					    vertex_descriptor, *spike_recorder_descriptor,
					    Edge(
					        out_edge->get_channels_on_source(), out_edge->get_channels_on_source(),
					        out_edge->port_on_source, 0));
				}

				// add external source neuron population on target execution instance if it is on a
				// time domain
				std::optional<VertexOnTopology> target_population_descriptor;
				if (target_time_domain) {
					Population target_population(
					    ExternalSourceNeuron(), *out_edge_cell_channels_on_source,
					    ExternalSourceNeuron::ParameterSpace(
					        out_edge->get_channels_on_source().size()),
					    target_time_domain, target_execution_instance);

					target_population_descriptor =
					    get_topology().add_vertex(std::move(target_population));
				}

				// add edges
				if (target_population_descriptor) {
					if (spike_recorder_descriptor) {
						get_topology().add_edge(
						    *spike_recorder_descriptor, *target_population_descriptor,
						    Edge(
						        out_edge->get_channels_on_source(),
						        *out_edge_cell_channels_on_source->cartesian_product(
						            CuboidMultiIndexSequence({1})),
						        0, 0));
					} else {
						get_topology().add_edge(
						    vertex_descriptor, *target_population_descriptor,
						    Edge(
						        out_edge->get_channels_on_source(),
						        *out_edge_cell_channels_on_source->cartesian_product(
						            CuboidMultiIndexSequence({1})),
						        0, 0));
					}

					get_topology().add_edge(
					    *target_population_descriptor, target_descriptor,
					    Edge(
					        *out_edge_cell_channels_on_source->cartesian_product(
					            CuboidMultiIndexSequence(
					                {1}, {CompartmentOnNeuronDimensionUnit()})),
					        out_edge->get_channels_on_target(), 0, out_edge->port_on_target));
				} else {
					if (spike_recorder_descriptor) {
						get_topology().add_edge(
						    *spike_recorder_descriptor, target_descriptor,
						    Edge(
						        out_edge->get_channels_on_source(),
						        out_edge->get_channels_on_target(), 0, out_edge->port_on_target));
					} else {
						get_topology().add_edge(vertex_descriptor, target_descriptor, *out_edge);
					}
				}
			}
		}
	}
}

} // namespace grenade::vx::network::abstract
