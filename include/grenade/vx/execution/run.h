#pragma once
#include "grenade/common/output_data.h"
#include "grenade/vx/execution/detail/connection_acquisor.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "hate/visibility.h"
#include <vector>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "pyhxcomm/vx/connection_handle.h"
#include <log4cxx/logger.h>
#endif


namespace grenade::common {

struct Topology;
struct InputData;

} // namespace grenade::common

namespace grenade::vx::execution {

/**
 * Run the specified graph with specified inputs on the supplied executor.
 * @param executor Executor to use
 * @param graph Graph to execute
 * @param initial_config Map of initial configuration
 * @param input List of input values to use
 * @param hooks Map of playback sequence collections to be inserted at specified
 * execution instances
 */
grenade::common::OutputData run(
    JITGraphExecutor& executor,
    std::shared_ptr<grenade::common::Topology const> const& topology,
    grenade::common::InputData const& data,
    JITGraphExecutor::Hooks&& hooks = {}) SYMBOL_VISIBLE;

/**
 * Run the specified graphs with specified inputs on the supplied executor.
 * @param executor Executor to use
 * @param graphs Graphs to execute (one per snippet)
 * @param configs Maps of configurations (one per snippet)
 * @param input Lists of input values to use (one per snippet)
 * @param hooks Map of playback sequence collections to be inserted at specified
 * execution instances
 */
std::vector<grenade::common::OutputData> run(
    JITGraphExecutor& executor,
    std::vector<std::shared_ptr<grenade::common::Topology const>> const& topology,
    std::vector<std::reference_wrapper<grenade::common::InputData const>> const& data,
    JITGraphExecutor::Hooks&& hooks = {}) SYMBOL_VISIBLE;


#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
namespace detail {

template <typename... Ts>
struct RunUnrollPyBind11Helper
{
	RunUnrollPyBind11Helper(pybind11::module&){};
};

template <typename T, typename... Ts>
struct RunUnrollPyBind11Helper<std::variant<T, Ts...>>
    : RunUnrollPyBind11Helper<std::variant<Ts...>>
{
	using parent_t = RunUnrollPyBind11Helper<std::variant<Ts...>>;

	RunUnrollPyBind11Helper(pybind11::module& m) : parent_t(m)
	{
		m.def(
		    "run",
		    [](T& conn, std::shared_ptr<grenade::common::Topology const> const& topology,
		       grenade::common::InputData const& data,
		       execution::JITGraphExecutor::Hooks& hooks) -> grenade::common::OutputData {
			    execution::detail::ConnectionAcquisor<T> acquisor(conn);
			    return run(*acquisor.executor, topology, data, std::move(hooks));
		    },
		    pybind11::arg("connection"), pybind11::arg("topology"), pybind11::arg("data"),
		    pybind11::arg("hooks"));
		m.def(
		    "run",
		    [](T& conn,
		       std::vector<std::shared_ptr<grenade::common::Topology const>> const& topologies,
		       std::vector<grenade::common::InputData*> const& data,
		       execution::JITGraphExecutor::Hooks& hooks)
		        -> std::vector<grenade::common::OutputData> {
			    execution::detail::ConnectionAcquisor<T> acquisor(conn);
			    std::vector<std::reference_wrapper<grenade::common::InputData const>> data_ref;
			    for (auto const& d : data) {
				    data_ref.push_back(*d);
			    }
			    return run(*acquisor.executor, topologies, data_ref, std::move(hooks));
		    },
		    pybind11::arg("connection"), pybind11::arg("topologies"), pybind11::arg("data"),
		    pybind11::arg("hooks"));
		m.def(
		    "run",
		    [](T& conn, std::shared_ptr<grenade::common::Topology const> const& topology,
		       grenade::common::InputData const& data) -> grenade::common::OutputData {
			    execution::detail::ConnectionAcquisor<T> acquisor(conn);
			    return run(*acquisor.executor, topology, data);
		    },
		    pybind11::arg("connection"), pybind11::arg("topology"), pybind11::arg("data"));
		m.def(
		    "run",
		    [](T& conn,
		       std::vector<std::shared_ptr<grenade::common::Topology const>> const& topologies,
		       std::vector<grenade::common::InputData*> const& data)
		        -> std::vector<grenade::common::OutputData> {
			    execution::detail::ConnectionAcquisor<T> acquisor(conn);
			    std::vector<std::reference_wrapper<grenade::common::InputData const>> data_ref;
			    for (auto const& d : data) {
				    data_ref.push_back(*d);
			    }

			    return run(*acquisor.executor, topologies, data_ref);
		    },
		    pybind11::arg("connection"), pybind11::arg("topologies"), pybind11::arg("data"));
	}
};

} // namespace detail
#endif

GENPYBIND_MANUAL({
	[[maybe_unused]] ::grenade::vx::execution::detail::RunUnrollPyBind11Helper<
	    std::remove_cvref_t<::pyhxcomm::vx::ConnectionHandle>>
	    helper(parent);

	using namespace grenade::vx;
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::JITGraphExecutor>& conn,
	       std::shared_ptr<grenade::common::Topology const> const& topology,
	       grenade::common::InputData const& data,
	       execution::JITGraphExecutor::Hooks& hooks) -> grenade::common::OutputData {
		    return execution::run(conn.get(), topology, data, std::move(hooks));
	    },
	    pybind11::arg("connection"), pybind11::arg("topology"), pybind11::arg("data"),
	    pybind11::arg("hooks"));
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::JITGraphExecutor>& conn,
	       std::vector<std::shared_ptr<grenade::common::Topology const>> const& topologies,
	       std::vector<grenade::common::InputData*> const& data,
	       execution::JITGraphExecutor::Hooks& hooks) -> std::vector<grenade::common::OutputData> {
		    std::vector<std::reference_wrapper<grenade::common::InputData const>> data_ref;
		    for (auto const& d : data) {
			    data_ref.push_back(*d);
		    }
		    return execution::run(conn.get(), topologies, data_ref, std::move(hooks));
	    },
	    pybind11::arg("connection"), pybind11::arg("topologies"), pybind11::arg("data"),
	    pybind11::arg("hooks"));
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::JITGraphExecutor>& conn,
	       std::shared_ptr<grenade::common::Topology const> const& topology,
	       grenade::common::InputData const& data) -> grenade::common::OutputData {
		    return execution::run(conn.get(), topology, data);
	    },
	    pybind11::arg("connection"), pybind11::arg("topology"), pybind11::arg("data"));
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::JITGraphExecutor>& conn,
	       std::vector<std::shared_ptr<grenade::common::Topology const>> const& topologies,
	       std::vector<grenade::common::InputData*> const& data)
	        -> std::vector<grenade::common::OutputData> {
		    std::vector<std::reference_wrapper<grenade::common::InputData const>> data_ref;
		    for (auto const& d : data) {
			    data_ref.push_back(*d);
		    }
		    return execution::run(conn.get(), topologies, data_ref);
	    },
	    pybind11::arg("connection"), pybind11::arg("topologies"), pybind11::arg("data"));
})


} // namespace grenade::vx::execution
