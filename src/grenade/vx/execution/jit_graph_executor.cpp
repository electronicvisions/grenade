#include "grenade/vx/execution/jit_graph_executor.h"

#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/timer.h"
#include "hxcomm/vx/connection_from_env.h"
#include <map>
#include <stdexcept>
#include <boost/range/adaptor/map.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::execution {

JITGraphExecutor::JITGraphExecutor(bool const enable_differential_config, size_t connection_size) :
    m_connections()
{
	auto hxcomm_connections = hxcomm::vx::get_connection_list_from_env(connection_size);
	for (size_t i = 0; i < hxcomm_connections.size(); ++i) {
		grenade::common::ConnectionOnExecutor identifier(i);
		size_t const connection_size = std::visit(
		    [](auto const& connection) { return connection.size(); }, hxcomm_connections.at(i));
		m_connections.emplace(
		    identifier, backend::StatefulConnection(
		                    backend::InitializedConnection(std::move(hxcomm_connections.at(i))),
		                    std::vector<bool>(connection_size, enable_differential_config)));
	}
}

JITGraphExecutor::JITGraphExecutor(
    std::map<grenade::common::ConnectionOnExecutor, backend::StatefulConnection>&& connections) :
    m_connections(std::move(connections))
{
}

std::set<grenade::common::ConnectionOnExecutor> JITGraphExecutor::contained_connections() const
{
	std::set<grenade::common::ConnectionOnExecutor> ret;
	for (auto const& [identifier, _] : m_connections) {
		ret.insert(identifier);
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, backend::StatefulConnection>&&
JITGraphExecutor::release_connections()
{
	return std::move(m_connections);
}

size_t JITGraphExecutor::size() const
{
	return m_connections.size();
}

std::map<grenade::common::ConnectionOnExecutor, std::vector<hxcomm::ConnectionTimeInfo>>
JITGraphExecutor::get_time_info() const
{
	std::map<grenade::common::ConnectionOnExecutor, std::vector<hxcomm::ConnectionTimeInfo>> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_time_info());
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, std::vector<std::string>>
JITGraphExecutor::get_unique_identifier(std::optional<std::string> const& hwdb_path) const
{
	std::map<grenade::common::ConnectionOnExecutor, std::vector<std::string>> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_unique_identifier(hwdb_path));
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, std::vector<std::string>>
JITGraphExecutor::get_bitfile_info() const
{
	std::map<grenade::common::ConnectionOnExecutor, std::vector<std::string>> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_bitfile_info());
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, std::vector<std::string>>
JITGraphExecutor::get_remote_repo_state() const
{
	std::map<grenade::common::ConnectionOnExecutor, std::vector<std::string>> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_remote_repo_state());
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, std::vector<hxcomm::HwdbEntry>>
JITGraphExecutor::get_hwdb_entry() const
{
	std::map<grenade::common::ConnectionOnExecutor, std::vector<hxcomm::HwdbEntry>> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_hwdb_entry());
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, size_t> JITGraphExecutor::connection_sizes() const
{
	std::map<grenade::common::ConnectionOnExecutor, size_t> sizes;
	for (auto const& [connection_on_executor, connection] : m_connections) {
		sizes.emplace(connection_on_executor, connection.get_chips_on_connection().size());
	}
	return sizes;
}

bool JITGraphExecutor::is_executable_on(grenade::common::Topology const& topology)
{
	std::set<
	    std::pair<grenade::vx::common::ChipOnConnection, grenade::common::ConnectionOnExecutor>>
	    chips_on_executor;
	for (auto const& [connection_on_executor, connection] : m_connections) {
		auto const local_chips = connection.get_chips_on_connection();
		for (auto const& chip_on_connection : connection.get_chips_on_connection()) {
			chips_on_executor.insert({chip_on_connection, connection_on_executor});
		}
	}

	auto const find_chip_of_vertex_property = [&chips_on_executor](auto const& v) {
		if (auto const entity_on_chip = dynamic_cast<signal_flow::vertex::EntityOnChip const*>(&v);
		    entity_on_chip) {
			return chips_on_executor.contains(std::pair{
			    entity_on_chip->chip_on_connection,
			    entity_on_chip->get_execution_instance_on_executor()
			        .value()
			        .connection_on_executor});
		}
		return true;
	};

	auto const find_chip_of_vertex_descriptor = [&topology,
	                                             find_chip_of_vertex_property](auto const vertex) {
		return find_chip_of_vertex_property(topology.get(vertex));
	};

	auto const vertices = topology.vertices();
	return std::all_of(vertices.begin(), vertices.end(), find_chip_of_vertex_descriptor);
}

void JITGraphExecutor::check(grenade::common::Topology const& topology)
{
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	hate::Timer const timer;

	// check all DLSGlobal physical chips used in the topology are present in the provided
	// connections
	if (!is_executable_on(topology)) {
		throw std::runtime_error("Not all chips requested by a topology are on given connections.");
	}

	LOG4CXX_TRACE(
	    logger,
	    "check(): Checked fit of graph, input list and connection in " << timer.print() << ".");
}

} // namespace grenade::vx::execution
