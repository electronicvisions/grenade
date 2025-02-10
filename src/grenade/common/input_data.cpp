#include "grenade/common/input_data.h"

#include "grenade/common/port_data/batched.h"
#include "grenade/common/topology.h"
#include "hate/indent.h"
#include "hate/join.h"
#include <stdexcept>
#include <log4cxx/logger.h>

namespace grenade::common {

namespace {

bool has_homogeneous_batch_size(InputData const& data)
{
	std::optional<size_t> ports_size;
	if (!data.ports.empty()) {
		std::set<size_t> batch_sizes;
		for (auto const& [_, ports] : data.ports) {
			if (auto const batched_port_data = dynamic_cast<BatchedPortData const*>(&ports);
			    batched_port_data) {
				batch_sizes.insert(batched_port_data->batch_size());
			}
		}
		if (batch_sizes.size() > 1) {
			return false;
		}
		if (!batch_sizes.empty()) {
			ports_size = *batch_sizes.begin();
		}
	}
	std::optional<size_t> time_domain_runtimes_size;
	if (!data.time_domain_runtimes.empty()) {
		time_domain_runtimes_size = data.time_domain_runtimes.begin()->second.batch_size();
		for (auto const& [_, local_time_domain_runtimes] : data.time_domain_runtimes) {
			if (local_time_domain_runtimes.batch_size() != *time_domain_runtimes_size) {
				return false;
			}
		}
	}
	if (ports_size && time_domain_runtimes_size && ports_size != time_domain_runtimes_size) {
		return false;
	}
	return true;
}

} // namespace


size_t InputData::batch_size() const
{
	if (!::grenade::common::has_homogeneous_batch_size(*this)) {
		throw std::runtime_error("Batch size not homogeneous.");
	}
	if (!time_domain_runtimes.empty()) {
		return time_domain_runtimes.begin()->second.batch_size();
	}
	for (auto const& [_, port] : ports) {
		if (auto const batched_port_data = dynamic_cast<BatchedPortData const*>(&port);
		    batched_port_data) {
			return batched_port_data->batch_size();
		}
	}
	return 0;
}

#define GRENADE_DYNAMICS_VALID_LOG_ERROR(...)                                                      \
	LOG4CXX_ERROR(                                                                                 \
	    log4cxx::Logger::getLogger("grenade.common.InputData"), "valid(): " << __VA_ARGS__)

bool InputData::valid(Topology const& topology) const
{
	if (!::grenade::common::has_homogeneous_batch_size(*this)) {
		GRENADE_DYNAMICS_VALID_LOG_ERROR(
		    "InputData doesn't have homogeneous batch size:\n"
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
		    << hate::join(
		           time_domain_runtimes, "\n",
		           [](auto const& v) {
			           auto const& [d, e] = v;
			           std::stringstream ss;
			           ss << d << ": " << e.batch_size();
			           return ss.str();
		           })
		    << ".");
		return false;
	}
	for (auto const vertex_descr : topology.vertices()) {
		auto const& vertex = topology.get(vertex_descr);
		auto const input_ports = vertex.get_input_ports();
		for (size_t input_port_on_vertex = 0; input_port_on_vertex < input_ports.size();
		     ++input_port_on_vertex) {
			if (input_ports.at(input_port_on_vertex).requires_or_generates_data ==
			    Vertex::Port::RequiresOrGeneratesData::yes) {
				if (!ports.contains(std::pair{vertex_descr, input_port_on_vertex})) {
					GRENADE_DYNAMICS_VALID_LOG_ERROR(
					    "InputData doesn't contain entry to " << vertex_descr << ", "
					                                          << input_port_on_vertex << ".");
					return false;
				}
				if (!vertex.valid_input_port_data(
				        input_port_on_vertex,
				        ports.get(std::pair{vertex_descr, input_port_on_vertex}))) {
					GRENADE_DYNAMICS_VALID_LOG_ERROR(
					    "InputData entry to " << vertex_descr << ", " << input_port_on_vertex
					                          << " is not valid.");
					return false;
				}
			}
		}
		if (auto const vertex_time_domain = vertex.get_time_domain(); vertex_time_domain) {
			if (!time_domain_runtimes.contains(*vertex_time_domain)) {
				GRENADE_DYNAMICS_VALID_LOG_ERROR(
				    "InputData doesn't contain entry to " << *vertex_time_domain << ".");
				return false;
			}
			if (!vertex.valid(time_domain_runtimes.get(*vertex_time_domain))) {
				GRENADE_DYNAMICS_VALID_LOG_ERROR(
				    "InputData entry to " << *vertex_time_domain << " is not valid.");
				return false;
			}
		}
	}
	return true;
}

std::ostream& operator<<(std::ostream& os, InputData const& data)
{
	hate::IndentingOstream ios(os);
	ios << "InputData(\n";
	ios << hate::Indentation("\t");
	for (auto const& [v, d] : data.ports) {
		ios << v.first << ", " << v.second << ": " << d << "\n";
	}
	for (auto const& [v, d] : data.time_domain_runtimes) {
		ios << v << ": " << d << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::common
