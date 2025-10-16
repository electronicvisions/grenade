#include "grenade/vx/network/abstract/topology_rewrite/plasticity_rule.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/edge_on_topology.h"
#include "grenade/common/inter_topology_hyper_edge/recorder.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/common/recorder.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/plasticity_rule.h"
#include <memory>

namespace grenade::vx::network::abstract {

PlasticityRuleRewrite::PlasticityRuleRewrite(
    std::shared_ptr<grenade::common::LinkedTopology> topology) :
    TopologyRewrite(std::move(topology))
{
}

void PlasticityRuleRewrite::operator()() const
{
	using namespace grenade::common;
	for (auto const vertex_descriptor : get_topology().vertices()) {
		if (auto const ptr =
		        dynamic_cast<PlasticityRule const*>(&(get_topology().get(vertex_descriptor)));
		    ptr) {
			ExecutionInstanceOnExecutor execution_instance_on_executor;
			// find execution instance on executor from in_edge source vertex
			for (auto const in_edge : get_topology().in_edges(vertex_descriptor)) {
				auto const source_descriptor = get_topology().source(in_edge);
				auto const& source =
				    dynamic_cast<PartitionedVertex const&>(get_topology().get(source_descriptor));
				if (source.get_execution_instance_on_executor()) {
					execution_instance_on_executor =
					    source.get_execution_instance_on_executor().value();
				}
			}
			auto new_plasticity_rule = ptr->copy();
			dynamic_cast<PartitionedVertex&>(*new_plasticity_rule)
			    .set_execution_instance_on_executor(execution_instance_on_executor);
			get_topology().set(vertex_descriptor, *new_plasticity_rule);
		}
	}
}

} // namespace grenade::vx::network::abstract
