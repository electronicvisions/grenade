#include "grenade/common/output_data.h"

#include "grenade/common/port_data/batched.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex.h"
#include "hate/indent.h"
#include "hate/join.h"
#include <log4cxx/logger.h>

namespace grenade::common {

OutputData::Executor::~Executor() {}

OutputData::ExecutionInstance::~ExecutionInstance() {}

OutputData::Executor const& OutputData::get_executor() const
{
	return *m_executor;
}

void OutputData::set_executor(Executor const& value)
{
	m_executor = value;
}

bool OutputData::has_executor() const
{
	return static_cast<bool>(m_executor);
}

#define GRENADE_RESULTS_VALID_LOG_ERROR(...)                                                       \
	LOG4CXX_ERROR(                                                                                 \
	    log4cxx::Logger::getLogger("grenade.common.OutputData"), "valid(): " << __VA_ARGS__)

bool OutputData::valid(Topology const& topology) const
{
	if (!has_homogeneous_batch_size()) {
		GRENADE_RESULTS_VALID_LOG_ERROR(
		    "OutputData doesn't have homogeneous batch size:\n"
		    << hate::join(
		           ports, "\n",
		           [](auto const& v) {
			           auto const& [d, e] = v;
			           std::stringstream ss;
			           if (auto const batched_port_data = dynamic_cast<BatchedPortData const*>(&e);
			               batched_port_data) {
				           ss << d << ": " << batched_port_data->batch_size();
			           }
			           return ss.str();
		           })
		    << ".");
		return false;
	}

	for (auto const vertex_descr : topology.vertices()) {
		auto const& vertex = topology.get(vertex_descr);
		auto const output_ports = vertex.get_output_ports();
		for (size_t output_port_on_vertex = 0; output_port_on_vertex < output_ports.size();
		     ++output_port_on_vertex) {
			if (output_ports.at(output_port_on_vertex).requires_or_generates_data ==
			    Vertex::Port::RequiresOrGeneratesData::yes) {
				if (!ports.contains(std::pair{vertex_descr, output_port_on_vertex})) {
					GRENADE_RESULTS_VALID_LOG_ERROR(
					    "OutputData doesn't contain entry to " << vertex_descr << ", "
					                                           << output_port_on_vertex
					                                           << ", which generates output data.");
					return false;
				}
				if (!vertex.valid_output_port_data(
				        output_port_on_vertex,
				        ports.get(std::pair{vertex_descr, output_port_on_vertex}))) {
					GRENADE_RESULTS_VALID_LOG_ERROR(
					    "OutputData entry to " << vertex_descr << ", " << output_port_on_vertex
					                           << " is not valid.");
					return false;
				}
			} else {
				if (ports.contains(std::pair{vertex_descr, output_port_on_vertex})) {
					GRENADE_RESULTS_VALID_LOG_ERROR(
					    "OutputData contains entry to " << vertex_descr << ", "
					                                    << output_port_on_vertex
					                                    << ", which doesn't generate output data.");
					return false;
				}
			}
		}
	}
	return true;
}

void OutputData::merge(OutputData& other)
{
	ports.merge(other.ports);
	execution_instances.merge(other.execution_instances);
	if (m_executor && other.m_executor) {
		throw std::invalid_argument(
		    "OutputData can't be merged when both versions have executor data.");
	} else if (other.m_executor) {
		decltype(m_executor) new_other_executor;
		std::swap(m_executor, other.m_executor);
	}
}

void OutputData::merge(OutputData&& other)
{
	ports.merge(std::move(other.ports));
	execution_instances.merge(std::move(other.execution_instances));
	if (m_executor && other.m_executor) {
		throw std::invalid_argument(
		    "OutputData can't be merged when both versions have executor data.");
	} else if (other.m_executor) {
		m_executor = std::move(other.m_executor);
	}
}

std::ostream& operator<<(std::ostream& os, OutputData const& value)
{
	hate::IndentingOstream ios(os);
	ios << "OutputData(\n";
	ios << hate::Indentation("\t");
	for (auto const& [d, result] : value.ports) {
		ios << d.first << ", " << d.second << ": " << result << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::common
