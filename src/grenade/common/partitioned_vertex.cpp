#include "grenade/common/partitioned_vertex.h"

#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/time_domain_on_topology.h"
#include <stdexcept>
#include <log4cxx/logger.h>

namespace grenade::common {

PartitionedVertex::StrongComponentInvariant::StrongComponentInvariant(
    std::optional<ExecutionInstanceOnExecutor> execution_instance_on_executor,
    std::optional<TimeDomainOnTopology> time_domain) :
    Vertex::StrongComponentInvariant(std::move(time_domain)),
    execution_instance_on_executor(std::move(execution_instance_on_executor))
{
}

std::unique_ptr<Vertex::StrongComponentInvariant>
PartitionedVertex::StrongComponentInvariant::copy() const
{
	return std::make_unique<StrongComponentInvariant>(*this);
}

std::unique_ptr<Vertex::StrongComponentInvariant>
PartitionedVertex::StrongComponentInvariant::move()
{
	return std::make_unique<StrongComponentInvariant>(std::move(*this));
}

bool PartitionedVertex::StrongComponentInvariant::is_equal_to(
    Vertex::StrongComponentInvariant const& other) const
{
	auto const& other_invariant = static_cast<StrongComponentInvariant const&>(other);
	return Vertex::StrongComponentInvariant::is_equal_to(other) &&
	       (execution_instance_on_executor == other_invariant.execution_instance_on_executor);
}

std::ostream& PartitionedVertex::StrongComponentInvariant::print(std::ostream& os) const
{
	os << "StrongComponentInvariant(";
	if (execution_instance_on_executor) {
		os << *execution_instance_on_executor;
	} else {
		os << "unique";
	}
	os << ", ";
	if (time_domain) {
		os << *time_domain;
	} else {
		os << "unique";
	}
	os << ")";
	return os;
}


PartitionedVertex::PartitionedVertex(
    std::optional<ExecutionInstanceOnExecutor> const& execution_instance_on_executor) :
    m_execution_instance_on_executor(execution_instance_on_executor)
{
}

std::optional<ExecutionInstanceOnExecutor> const&
PartitionedVertex::get_execution_instance_on_executor() const
{
	return m_execution_instance_on_executor;
}

void PartitionedVertex::set_execution_instance_on_executor(
    std::optional<ExecutionInstanceOnExecutor> const& value)
{
	m_execution_instance_on_executor = value;
}

std::unique_ptr<Vertex::StrongComponentInvariant>
PartitionedVertex::get_strong_component_invariant() const
{
	return std::make_unique<StrongComponentInvariant>(
	    m_execution_instance_on_executor, get_time_domain());
}

#define GRENADE_PARTITIONED_VERTEX_VALID_EDGE_FROM_LOG_ERROR(...)                                  \
	LOG4CXX_ERROR(                                                                                 \
	    log4cxx::Logger::getLogger("grenade.common.PartitionedVertex"),                            \
	    "valid_edge_from(): " << __VA_ARGS__)

bool PartitionedVertex::valid_edge_from(Vertex const& source, Edge const& edge) const
{
	if (!Vertex::valid_edge_from(source, edge)) {
		return false;
	}
	if (auto const source_vertex = dynamic_cast<PartitionedVertex const*>(&source); source_vertex) {
		if (source_vertex->m_execution_instance_on_executor && m_execution_instance_on_executor) {
			auto const input_ports = get_input_ports();
			if (edge.port_on_target >= input_ports.size()) {
				return false;
			}
			auto const input_port = input_ports.at(edge.port_on_target);
			if (source_vertex->m_execution_instance_on_executor ==
			    m_execution_instance_on_executor) {
				if (input_port.execution_instance_transition_constraint ==
				    Vertex::Port::ExecutionInstanceTransitionConstraint::required) {
					GRENADE_PARTITIONED_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
					    "Execution instances are not compatible to execution instance transition "
					    "constraint: source("
					    << *source_vertex->m_execution_instance_on_executor << ") vs. target("
					    << input_port.execution_instance_transition_constraint << ", "
					    << *m_execution_instance_on_executor << ").");
					return false;
				}
			} else if (
			    input_port.execution_instance_transition_constraint ==
			    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported) {
				GRENADE_PARTITIONED_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
				    "Execution instances are not compatible to execution instance transition "
				    "constraint: source("
				    << *source_vertex->m_execution_instance_on_executor << ") vs. target("
				    << input_port.execution_instance_transition_constraint << ", "
				    << *m_execution_instance_on_executor << ").");
				return false;
			}
		}
	}
	return true;
}

#define GRENADE_PARTITIONED_VERTEX_VALID_EDGE_TO_LOG_ERROR(...)                                    \
	LOG4CXX_ERROR(                                                                                 \
	    log4cxx::Logger::getLogger("grenade.common.PartitionedVertex"),                            \
	    "valid_edge_to(): " << __VA_ARGS__)

bool PartitionedVertex::valid_edge_to(Vertex const& target, Edge const& edge) const
{
	if (!Vertex::valid_edge_to(target, edge)) {
		return false;
	}
	if (auto const target_vertex = dynamic_cast<PartitionedVertex const*>(&target); target_vertex) {
		if (target_vertex->m_execution_instance_on_executor && m_execution_instance_on_executor) {
			auto const output_ports = get_output_ports();
			if (edge.port_on_source >= output_ports.size()) {
				return false;
			}
			auto const output_port = output_ports.at(edge.port_on_source);
			if (target_vertex->m_execution_instance_on_executor ==
			    m_execution_instance_on_executor) {
				if (output_port.execution_instance_transition_constraint ==
				    Vertex::Port::ExecutionInstanceTransitionConstraint::required) {
					GRENADE_PARTITIONED_VERTEX_VALID_EDGE_TO_LOG_ERROR(
					    "Execution instances are not compatible to execution instance transition "
					    "constraint: target("
					    << output_port.execution_instance_transition_constraint << ", "
					    << *m_execution_instance_on_executor << ") vs. target("
					    << *target_vertex->m_execution_instance_on_executor << ").");
					return false;
				}
			} else if (
			    output_port.execution_instance_transition_constraint ==
			    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported) {
				GRENADE_PARTITIONED_VERTEX_VALID_EDGE_TO_LOG_ERROR(
				    "Execution instances are not compatible to execution instance transition "
				    "constraint: target("
				    << output_port.execution_instance_transition_constraint << ", "
				    << *m_execution_instance_on_executor << ") vs. target("
				    << *target_vertex->m_execution_instance_on_executor << ").");
				return false;
			}
		}
	}
	return true;
}

std::ostream& PartitionedVertex::print(std::ostream& os) const
{
	os << "PartitionedVertex(";
	if (m_execution_instance_on_executor) {
		os << *m_execution_instance_on_executor;
	} else {
		os << "nullopt";
	}
	os << ")";
	return os;
}

bool PartitionedVertex::is_equal_to(Vertex const& other) const
{
	auto const& other_vertex = static_cast<PartitionedVertex const&>(other);
	return m_execution_instance_on_executor == other_vertex.m_execution_instance_on_executor;
}

} // namespace grenade::common
