#include "grenade/common/output_data.h"

#include "grenade/common/port_data/batched.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex.h"
#include "hate/indent.h"
#include "hate/join.h"
#include <log4cxx/logger.h>

namespace grenade::common {

OutputData::Global::~Global() {}

OutputData::Global const& OutputData::get_global() const
{
	return *m_global;
}

void OutputData::set_global(Global const& value)
{
	m_global = value;
}

bool OutputData::has_global() const
{
	return static_cast<bool>(m_global);
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
	if (m_global && other.m_global) {
		m_global->merge(*other.m_global);
	} else if (other.m_global) {
		decltype(m_global) new_other_global;
		std::swap(m_global, other.m_global);
	}
}

void OutputData::merge(OutputData&& other)
{
	ports.merge(std::move(other.ports));
	if (m_global && other.m_global) {
		m_global->merge(std::move(*other.m_global));
	} else if (other.m_global) {
		m_global = std::move(other.m_global);
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
