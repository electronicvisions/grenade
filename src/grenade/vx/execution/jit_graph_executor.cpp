#include "grenade/vx/execution/jit_graph_executor.h"

#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/signal_flow/graph.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/timer.h"
#include "hxcomm/vx/connection_from_env.h"
#include <map>
#include <stdexcept>
#include <boost/range/adaptor/map.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::execution {

JITGraphExecutor::JITGraphExecutor(bool const enable_differential_config) :
    m_connections(),
    m_connection_state_storages(),
    m_enable_differential_config(enable_differential_config)
{
	auto hxcomm_connections = hxcomm::vx::get_connection_list_from_env();
	for (size_t i = 0; i < hxcomm_connections.size(); ++i) {
		halco::hicann_dls::vx::v3::DLSGlobal identifier(i);
		m_connections.emplace(identifier, backend::Connection(std::move(hxcomm_connections.at(i))));
		m_connection_state_storages.emplace(
		    std::piecewise_construct, std::forward_as_tuple(identifier),
		    std::forward_as_tuple(m_enable_differential_config, m_connections.at(identifier)));
	}
}

JITGraphExecutor::JITGraphExecutor(
    std::map<halco::hicann_dls::vx::v3::DLSGlobal, backend::Connection>&& connections,
    bool const enable_differential_config) :
    m_connections(std::move(connections)),
    m_connection_state_storages(),
    m_enable_differential_config(enable_differential_config)
{
	for (auto const& [identifier, _] : m_connections) {
		m_connection_state_storages.emplace(
		    std::piecewise_construct, std::forward_as_tuple(identifier),
		    std::forward_as_tuple(m_enable_differential_config, m_connections.at(identifier)));
	}
}

std::set<halco::hicann_dls::vx::v3::DLSGlobal> JITGraphExecutor::contained_connections() const
{
	std::set<halco::hicann_dls::vx::v3::DLSGlobal> ret;
	for (auto const& [identifier, _] : m_connections) {
		ret.insert(identifier);
	}
	return ret;
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, backend::Connection>&&
JITGraphExecutor::release_connections()
{
	m_connection_state_storages.clear();
	return std::move(m_connections);
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, hxcomm::ConnectionTimeInfo>
JITGraphExecutor::get_time_info() const
{
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, hxcomm::ConnectionTimeInfo> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_time_info());
	}
	return ret;
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> JITGraphExecutor::get_unique_identifier(
    std::optional<std::string> const& hwdb_path) const
{
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_unique_identifier(hwdb_path));
	}
	return ret;
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> JITGraphExecutor::get_bitfile_info()
    const
{
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_bitfile_info());
	}
	return ret;
}

std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string>
JITGraphExecutor::get_remote_repo_state() const
{
	std::map<halco::hicann_dls::vx::v3::DLSGlobal, std::string> ret;
	for (auto const& [identifier, connection] : m_connections) {
		ret.emplace(identifier, connection.get_remote_repo_state());
	}
	return ret;
}

bool JITGraphExecutor::is_executable_on(signal_flow::Graph const& graph)
{
	auto const connection_dls_globals = boost::adaptors::keys(m_connections);
	auto const& execution_instance_map = graph.get_execution_instance_map();
	auto const& vertex_descriptor_map = graph.get_vertex_descriptor_map();

	auto const vertices = boost::make_iterator_range(boost::vertices(graph.get_graph()));
	return std::none_of(vertices.begin(), vertices.end(), [&](auto const vertex) {
		return std::find(
		           connection_dls_globals.begin(), connection_dls_globals.end(),
		           execution_instance_map.left.at(vertex_descriptor_map.left.at(vertex))
		               .toDLSGlobal()) == connection_dls_globals.end();
	});
}

void JITGraphExecutor::check(signal_flow::Graph const& graph)
{
	auto logger = log4cxx::Logger::getLogger("grenade.JITGraphExecutor");
	hate::Timer const timer;

	// check execution instance graph is acyclic
	if (!graph.is_acyclic_execution_instance_graph()) {
		throw std::runtime_error("Execution instance graph is not acyclic.");
	}

	// check all DLSGlobal physical chips used in the graph are present in the provided connections
	if (!is_executable_on(graph)) {
		throw std::runtime_error("Graph requests connection not provided.");
	}

	LOG4CXX_TRACE(
	    logger,
	    "check(): Checked fit of graph, input list and connection in " << timer.print() << ".");
}

} // namespace grenade::vx::execution
