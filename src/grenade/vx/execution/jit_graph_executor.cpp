#include "grenade/vx/execution/jit_graph_executor.h"

#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/common/entity_on_chip.h"
#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/timer.h"
#include "hxcomm/vx/connection_from_env.h"
#include <map>
#include <stdexcept>
#include <boost/range/adaptor/map.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::execution {

JITGraphExecutor::JITGraphExecutor(bool const enable_differential_config) : m_connections()
{
	auto hxcomm_connections = hxcomm::vx::get_connection_list_from_env();
	for (size_t i = 0; i < hxcomm_connections.size(); ++i) {
		grenade::common::ConnectionOnExecutor identifier(i);
		m_connections.emplace(
		    identifier, backend::StatefulConnection(
		                    backend::InitializedConnection(std::move(hxcomm_connections.at(i))),
		                    enable_differential_config));
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

std::map<grenade::common::ConnectionOnExecutor, hxcomm::ConnectionTimeInfo>
JITGraphExecutor::get_time_info() const
{
	std::map<grenade::common::ConnectionOnExecutor, hxcomm::ConnectionTimeInfo> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_time_info());
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, std::string>
JITGraphExecutor::get_unique_identifier(std::optional<std::string> const& hwdb_path) const
{
	std::map<grenade::common::ConnectionOnExecutor, std::string> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_unique_identifier(hwdb_path));
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, std::string> JITGraphExecutor::get_bitfile_info()
    const
{
	std::map<grenade::common::ConnectionOnExecutor, std::string> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_bitfile_info());
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, std::string>
JITGraphExecutor::get_remote_repo_state() const
{
	std::map<grenade::common::ConnectionOnExecutor, std::string> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_remote_repo_state());
	}
	return ret;
}

std::map<grenade::common::ConnectionOnExecutor, hxcomm::HwdbEntry>
JITGraphExecutor::get_hwdb_entry() const
{
	std::map<grenade::common::ConnectionOnExecutor, hxcomm::HwdbEntry> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_hwdb_entry());
	}
	return ret;
}

bool JITGraphExecutor::is_executable_on(signal_flow::Graph const& graph)
{
	std::set<grenade::vx::common::EntityOnChip::ChipOnExecutor> chips_on_executor;
	for (auto const& [connection_on_executor, connection] : m_connections) {
		auto const local_chips = connection.get_chips_on_connection();
		for (auto const& chip_on_connection : connection.get_chips_on_connection()) {
			chips_on_executor.insert({chip_on_connection, connection_on_executor});
		}
	}

	auto const find_chip_of_vertex_property = [chips_on_executor](auto const& v) {
		if constexpr (std::is_base_of_v<
		                  signal_flow::vertex::EntityOnChip, std::decay_t<decltype(v)>>) {
			return chips_on_executor.contains(v.chip_on_executor);
		}
		return true;
	};

	auto const find_chip_of_vertex_descriptor = [graph,
	                                             find_chip_of_vertex_property](auto const vertex) {
		return std::visit(find_chip_of_vertex_property, graph.get_vertex_property(vertex));
	};

	auto const vertices = boost::make_iterator_range(boost::vertices(graph.get_graph()));
	return std::all_of(vertices.begin(), vertices.end(), find_chip_of_vertex_descriptor);
}

void JITGraphExecutor::check(signal_flow::Graph const& graph)
{
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	hate::Timer const timer;

	// check execution instance graph is acyclic
	if (!graph.is_acyclic_execution_instance_graph()) {
		throw std::runtime_error("Execution instance graph is not acyclic.");
	}

	// check all physical chips used in the graph are present in the provided connections
	if (!is_executable_on(graph)) {
		throw std::runtime_error("Graph requests connection not provided.");
	}

	LOG4CXX_TRACE(
	    logger,
	    "check(): Checked fit of graph, input list and connection in " << timer.print() << ".");
}

} // namespace grenade::vx::execution
