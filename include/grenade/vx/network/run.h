#include "grenade/vx/backend/connection.h"
#include "grenade/vx/config.h"
#include "grenade/vx/execution_instance_playback_hooks.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/network/network_graph.h"
#include "hate/visibility.h"
#include "hxcomm/vx/connection_variant.h"

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "pyhxcomm/vx/connection_handle.h"
#endif

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param inputs Inputs to use
 * @return Run time information
 */
IODataMap run(
    backend::Connection& connection,
    ChipConfig const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param inputs Inputs to use
 * @return Run time information
 */
IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    ChipConfig const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param inputs Inputs to use
 * @param playback_hooks Optional playback sequences to inject
 * @return Run time information
 */
IODataMap run(
    backend::Connection& connection,
    ChipConfig const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs,
    ExecutionInstancePlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param inputs Inputs to use
 * @param playback_hooks Optional playback sequences to inject
 * @return Run time information
 */
IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    ChipConfig const& config,
    NetworkGraph const& network_graph,
    IODataMap const& inputs,
    ExecutionInstancePlaybackHooks& playback_hooks) SYMBOL_VISIBLE;


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

	/**
	 * RAII extraction of connection and emplacement back into handle
	 */
	struct ConnectionAcquisor
	{
		T& handle;
		hxcomm::vx::ConnectionVariant connection;

		ConnectionAcquisor(T& t) : handle(t), connection(std::move(t.get())) {}

		~ConnectionAcquisor()
		{
			handle.get() = std::move(std::get<typename T::connection_type>(connection));
		}
	};

	RunUnrollPyBind11Helper(pybind11::module& m) : parent_t(m)
	{
		m.def(
		    "run",
		    [](T& conn, ChipConfig const& config, NetworkGraph const& network_graph,
		       IODataMap const& inputs,
		       ExecutionInstancePlaybackHooks& playback_hooks) -> IODataMap {
			    ConnectionAcquisor acquisor(conn);
			    return run(acquisor.connection, config, network_graph, inputs, playback_hooks);
		    },
		    pybind11::arg("connection"), pybind11::arg("config"), pybind11::arg("network_graph"),
		    pybind11::arg("inputs"), pybind11::arg("playback_hooks"));
		m.def(
		    "run",
		    [](T& conn, ChipConfig const& config, NetworkGraph const& network_graph,
		       IODataMap const& inputs) -> IODataMap {
			    ConnectionAcquisor acquisor(conn);
			    return run(acquisor.connection, config, network_graph, inputs);
		    },
		    pybind11::arg("connection"), pybind11::arg("config"), pybind11::arg("network_graph"),
		    pybind11::arg("inputs"));
	}
};

} // namespace detail
#endif

GENPYBIND_MANUAL({
	[[maybe_unused]] ::grenade::vx::network::detail::RunUnrollPyBind11Helper<
	    std::remove_cvref_t<::pyhxcomm::vx::ConnectionHandle>>
	    helper(parent);
})

} // namespace network

} // namespace grenade::vx
