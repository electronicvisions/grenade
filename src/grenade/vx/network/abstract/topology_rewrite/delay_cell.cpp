#include "grenade/vx/network/abstract/topology_rewrite/delay_cell.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/edge_on_topology.h"
#include "grenade/common/inter_topology_hyper_edge/recorder.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/recorder.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/plasticity_rule.h"
#include "grenade/vx/network/abstract/population_cell/delay.h"
#include <memory>

namespace grenade::vx::network::abstract {

DelayCellRewrite::DelayCellRewrite(std::shared_ptr<grenade::common::LinkedTopology> topology) :
    TopologyRewrite(std::move(topology))
{
}

void DelayCellRewrite::operator()() const
{
	using namespace grenade::common;
	std::set<TimeDomainOnTopology> present_time_domains;
	for (auto const vertex_descriptor : get_topology().vertices()) {
		auto const& time_domain = get_topology().get(vertex_descriptor).get_time_domain();
		if (time_domain) {
			present_time_domains.insert(*time_domain);
		}
	}
	TimeDomainOnTopology new_time_domain;
	if (!present_time_domains.empty()) {
		new_time_domain = *present_time_domains.rbegin() + TimeDomainOnTopology(1);
	}
	for (auto const vertex_descriptor : get_topology().vertices()) {
		if (auto const population =
		        dynamic_cast<Population const*>(&(get_topology().get(vertex_descriptor)));
		    population) {
			if (auto const delay_cell = dynamic_cast<DelayCell const*>(&population->get_cell());
			    delay_cell) {
				auto const old_time_domain = *population->get_time_domain();
				auto reachable_from_vertices =
				    get_topology().get_reachable_vertices_from(vertex_descriptor);

				// collect new vertices to atomically alter them on the topology
				// to avoid time domain mismatch conflicts
				Topology::Vertices new_vertices;

				// remove time domain from delay cell
				new_vertices.set(
				    vertex_descriptor, Population(
				                           DelayCell(false), population->get_shape(),
				                           population->get_parameter_space(), std::nullopt,
				                           population->get_execution_instance_on_executor()));

				// assign new time domain to all outgoing vertices on previously same time domain
				for (auto const& reachable_from_vertex_descriptor : reachable_from_vertices) {
					auto vertex = get_topology().get(reachable_from_vertex_descriptor).copy();
					if (reachable_from_vertex_descriptor == vertex_descriptor) {
						// already added to new vertices
						continue;
					}
					if (vertex->get_time_domain() != old_time_domain) {
						// different time domain than delay cell population
						continue;
					}
					if (auto const population = dynamic_cast<Population*>(&(*vertex)); population) {
						population->set_time_domain(new_time_domain);
					} else if (auto const projection = dynamic_cast<Projection*>(&(*vertex));
					           projection) {
						projection->set_time_domain(new_time_domain);
					} else if (auto const recorder = dynamic_cast<Recorder*>(&(*vertex));
					           recorder) {
						recorder->set_time_domain(new_time_domain);
					} else if (auto const plasticity_rule =
					               dynamic_cast<PlasticityRule*>(&(*vertex));
					           plasticity_rule) {
						plasticity_rule->set_time_domain(new_time_domain);
					}
					new_vertices.set(reachable_from_vertex_descriptor, std::move(*vertex));
				}

				get_topology().set(std::move(new_vertices));

				// set time domain translation
				get_topology().inter_topology_time_domain_edges.emplace(
				    new_time_domain, old_time_domain);
				new_time_domain += TimeDomainOnTopology(1);
			}
		}
	}
}

} // namespace grenade::vx::network::abstract
