#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "hate/visibility.h"
#include "hxcomm/vx/connection_variant.h"
#include <vector>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "pyhxcomm/vx/connection_handle.h"
#endif

namespace lola::vx::v3 {
class Chip;
} // namespace lola::vx::v3

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param executor Executor instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param input Inputs to use
 * @return Run time information
 */
signal_flow::IODataMap run(
    execution::JITGraphExecutor& executor,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graphs and fetch results.
 *
 * @param executor Executor instance to be used for running the graph
 * @param network_graphs Network hardware graphs to run
 * @param configs Static chip configurations to use
 * @param inputs Inputs to use
 * @return Run time information
 */
std::vector<signal_flow::IODataMap> run(
    execution::JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param input Inputs to use
 * @return Run time information
 */
signal_flow::IODataMap run(
    execution::backend::Connection& connection,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graphs and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graphs Network hardware graphs to run
 * @param configs Static chip configurations to use
 * @param inputs Inputs to use
 * @return Run time information
 */
std::vector<signal_flow::IODataMap> run(
    execution::backend::Connection& connection,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param input Inputs to use
 * @return Run time information
 */
signal_flow::IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graphs and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graphs Network hardware graphs to run
 * @param configs Static chip configurations to use
 * @param inputs Inputs to use
 * @return Run time information
 */
std::vector<signal_flow::IODataMap> run(
    hxcomm::vx::ConnectionVariant& connection,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param executor Executor instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param input Inputs to use
 * @param playback_hooks Optional playback sequences to inject
 * @return Run time information
 */
signal_flow::IODataMap run(
    execution::JITGraphExecutor& executor,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graphs and fetch results.
 *
 * @param executor Executor instance to be used for running the graph
 * @param network_graphs Network hardware graphs to run
 * @param configs Static chip configurations to use
 * @param inputs Inputs to use
 * @param playback_hooks Optional playback sequences to inject
 * @return Run time information
 */
std::vector<signal_flow::IODataMap> run(
    execution::JITGraphExecutor& executor,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param input Inputs to use
 * @param playback_hooks Optional playback sequences to inject
 * @return Run time information
 */
signal_flow::IODataMap run(
    execution::backend::Connection& connection,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graphs and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graphs Network hardware graphs to run
 * @param configs Static chip configurations to use
 * @param inputs Inputs to use
 * @param playback_hooks Optional playback sequences to inject
 * @return Run time information
 */
std::vector<signal_flow::IODataMap> run(
    execution::backend::Connection& connection,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graph and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graph Network hardware graph to run
 * @param config Static chip configuration to use
 * @param input Inputs to use
 * @param playback_hooks Optional playback sequences to inject
 * @return Run time information
 */
signal_flow::IODataMap run(
    hxcomm::vx::ConnectionVariant& connection,
    execution::JITGraphExecutor::ChipConfigs const& config,
    NetworkGraph const& network_graph,
    signal_flow::IODataMap const& input,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks) SYMBOL_VISIBLE;

/**
 * Execute the given network hardware graphs and fetch results.
 *
 * @param connection Connection instance to be used for running the graph
 * @param network_graphs Network hardware graphs to run
 * @param configs Static chip configurations to use
 * @param inputs Inputs to use
 * @param playback_hooks Optional playback sequences to inject
 * @return Run time information
 */
std::vector<signal_flow::IODataMap> run(
    hxcomm::vx::ConnectionVariant& connection,
    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>> const&
        configs,
    std::vector<std::reference_wrapper<NetworkGraph const>> const& network_graphs,
    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> const& inputs,
    execution::JITGraphExecutor::PlaybackHooks& playback_hooks) SYMBOL_VISIBLE;


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
		    [](T& conn, execution::JITGraphExecutor::ChipConfigs const& config,
		       NetworkGraph const& network_graph, signal_flow::IODataMap const& input,
		       execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
		        -> signal_flow::IODataMap {
			    ConnectionAcquisor acquisor(conn);
			    return run(acquisor.connection, config, network_graph, input, playback_hooks);
		    },
		    pybind11::arg("connection"), pybind11::arg("config"), pybind11::arg("network_graph"),
		    pybind11::arg("input"), pybind11::arg("playback_hooks"));
		m.def(
		    "run",
		    [](T& conn, std::vector<execution::JITGraphExecutor::ChipConfigs> const& configs,
		       std::vector<NetworkGraph*> const& network_graphs,
		       std::vector<signal_flow::IODataMap*> const& inputs,
		       execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
		        -> std::vector<signal_flow::IODataMap> {
			    ConnectionAcquisor acquisor(conn);
			    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>>
			        configs_ref;
			    for (auto const& config : configs) {
				    configs_ref.push_back(config);
			    }
			    std::vector<std::reference_wrapper<NetworkGraph const>> network_graphs_ref;
			    for (auto const& network_graph : network_graphs) {
				    network_graphs_ref.push_back(*network_graph);
			    }
			    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> inputs_ref;
			    for (auto const& input : inputs) {
				    inputs_ref.push_back(*input);
			    }

			    return run(
			        acquisor.connection, configs_ref, network_graphs_ref, inputs_ref,
			        playback_hooks);
		    },
		    pybind11::arg("connection"), pybind11::arg("configs"), pybind11::arg("network_graphs"),
		    pybind11::arg("inputs"), pybind11::arg("playback_hooks"));
		m.def(
		    "run",
		    [](T& conn, execution::JITGraphExecutor::ChipConfigs const& config,
		       NetworkGraph const& network_graph,
		       signal_flow::IODataMap const& input) -> signal_flow::IODataMap {
			    ConnectionAcquisor acquisor(conn);
			    return run(acquisor.connection, config, network_graph, input);
		    },
		    pybind11::arg("connection"), pybind11::arg("config"), pybind11::arg("network_graph"),
		    pybind11::arg("input"));
		m.def(
		    "run",
		    [](T& conn, std::vector<execution::JITGraphExecutor::ChipConfigs> const& configs,
		       std::vector<NetworkGraph*> const& network_graphs,
		       std::vector<signal_flow::IODataMap*> const& inputs)
		        -> std::vector<signal_flow::IODataMap> {
			    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>>
			        configs_ref;
			    for (auto const& config : configs) {
				    configs_ref.push_back(config);
			    }
			    std::vector<std::reference_wrapper<NetworkGraph const>> network_graphs_ref;
			    for (auto const& network_graph : network_graphs) {
				    network_graphs_ref.push_back(*network_graph);
			    }
			    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> inputs_ref;
			    for (auto const& input : inputs) {
				    inputs_ref.push_back(*input);
			    }

			    ConnectionAcquisor acquisor(conn);
			    return run(acquisor.connection, configs_ref, network_graphs_ref, inputs_ref);
		    },
		    pybind11::arg("connection"), pybind11::arg("configs"), pybind11::arg("network_graphs"),
		    pybind11::arg("inputs"));
	}
};

} // namespace detail
#endif

GENPYBIND_MANUAL({
	[[maybe_unused]] ::grenade::vx::network::detail::RunUnrollPyBind11Helper<
	    std::remove_cvref_t<::pyhxcomm::vx::ConnectionHandle>>
	    helper(parent);

	using namespace grenade::vx;
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::backend::Connection>& conn,
	       execution::JITGraphExecutor::ChipConfigs const& config,
	       network::NetworkGraph const& network_graph, signal_flow::IODataMap const& input,
	       execution::JITGraphExecutor::PlaybackHooks& playback_hooks) -> signal_flow::IODataMap {
		    return network::run(conn.get(), config, network_graph, input, playback_hooks);
	    },
	    pybind11::arg("connection"), pybind11::arg("config"), pybind11::arg("network_graph"),
	    pybind11::arg("input"), pybind11::arg("playback_hooks"));
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::backend::Connection>& conn,
	       std::vector<execution::JITGraphExecutor::ChipConfigs> const& configs,
	       std::vector<network::NetworkGraph*> const& network_graphs,
	       std::vector<signal_flow::IODataMap*> const& inputs,
	       execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
	        -> std::vector<signal_flow::IODataMap> {
		    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>>
		        configs_ref;
		    for (auto const& config : configs) {
			    configs_ref.push_back(config);
		    }
		    std::vector<std::reference_wrapper<network::NetworkGraph const>> network_graphs_ref;
		    for (auto const& network_graph : network_graphs) {
			    network_graphs_ref.push_back(*network_graph);
		    }
		    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> inputs_ref;
		    for (auto const& input : inputs) {
			    inputs_ref.push_back(*input);
		    }

		    return network::run(
		        conn.get(), configs_ref, network_graphs_ref, inputs_ref, playback_hooks);
	    },
	    pybind11::arg("connection"), pybind11::arg("configs"), pybind11::arg("network_graphs"),
	    pybind11::arg("inputs"), pybind11::arg("playback_hooks"));
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::backend::Connection>& conn,
	       execution::JITGraphExecutor::ChipConfigs const& config,
	       network::NetworkGraph const& network_graph,
	       signal_flow::IODataMap const& input) -> signal_flow::IODataMap {
		    return network::run(conn.get(), config, network_graph, input);
	    },
	    pybind11::arg("connection"), pybind11::arg("config"), pybind11::arg("network_graph"),
	    pybind11::arg("input"));
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::backend::Connection>& conn,
	       std::vector<execution::JITGraphExecutor::ChipConfigs> const& configs,
	       std::vector<network::NetworkGraph*> const& network_graphs,
	       std::vector<signal_flow::IODataMap*> const& inputs)
	        -> std::vector<signal_flow::IODataMap> {
		    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>>
		        configs_ref;
		    for (auto const& config : configs) {
			    configs_ref.push_back(config);
		    }
		    std::vector<std::reference_wrapper<network::NetworkGraph const>> network_graphs_ref;
		    for (auto const& network_graph : network_graphs) {
			    network_graphs_ref.push_back(*network_graph);
		    }
		    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> inputs_ref;
		    for (auto const& input : inputs) {
			    inputs_ref.push_back(*input);
		    }
		    return network::run(conn.get(), configs_ref, network_graphs_ref, inputs_ref);
	    },
	    pybind11::arg("connection"), pybind11::arg("configs"), pybind11::arg("network_graphs"),
	    pybind11::arg("inputs"));

	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::JITGraphExecutor>& conn,
	       execution::JITGraphExecutor::ChipConfigs const& config,
	       network::NetworkGraph const& network_graph, signal_flow::IODataMap const& input,
	       execution::JITGraphExecutor::PlaybackHooks& playback_hooks) -> signal_flow::IODataMap {
		    return network::run(conn.get(), config, network_graph, input, playback_hooks);
	    },
	    pybind11::arg("connection"), pybind11::arg("config"), pybind11::arg("network_graph"),
	    pybind11::arg("input"), pybind11::arg("playback_hooks"));
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::JITGraphExecutor>& conn,
	       std::vector<execution::JITGraphExecutor::ChipConfigs> const& configs,
	       std::vector<network::NetworkGraph*> const& network_graphs,
	       std::vector<signal_flow::IODataMap*> const& inputs,
	       execution::JITGraphExecutor::PlaybackHooks& playback_hooks)
	        -> std::vector<signal_flow::IODataMap> {
		    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>>
		        configs_ref;
		    for (auto const& config : configs) {
			    configs_ref.push_back(config);
		    }
		    std::vector<std::reference_wrapper<network::NetworkGraph const>> network_graphs_ref;
		    for (auto const& network_graph : network_graphs) {
			    network_graphs_ref.push_back(*network_graph);
		    }
		    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> inputs_ref;
		    for (auto const& input : inputs) {
			    inputs_ref.push_back(*input);
		    }

		    return network::run(
		        conn.get(), configs_ref, network_graphs_ref, inputs_ref, playback_hooks);
	    },
	    pybind11::arg("connection"), pybind11::arg("configs"), pybind11::arg("network_graphs"),
	    pybind11::arg("inputs"), pybind11::arg("playback_hooks"));
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::JITGraphExecutor>& conn,
	       execution::JITGraphExecutor::ChipConfigs const& config,
	       network::NetworkGraph const& network_graph,
	       signal_flow::IODataMap const& input) -> signal_flow::IODataMap {
		    return network::run(conn.get(), config, network_graph, input);
	    },
	    pybind11::arg("connection"), pybind11::arg("config"), pybind11::arg("network_graph"),
	    pybind11::arg("input"));
	parent.def(
	    "run",
	    [](::pyhxcomm::Handle<execution::JITGraphExecutor>& conn,
	       std::vector<execution::JITGraphExecutor::ChipConfigs> const& configs,
	       std::vector<network::NetworkGraph*> const& network_graphs,
	       std::vector<signal_flow::IODataMap*> const& inputs)
	        -> std::vector<signal_flow::IODataMap> {
		    std::vector<std::reference_wrapper<execution::JITGraphExecutor::ChipConfigs const>>
		        configs_ref;
		    for (auto const& config : configs) {
			    configs_ref.push_back(config);
		    }
		    std::vector<std::reference_wrapper<network::NetworkGraph const>> network_graphs_ref;
		    for (auto const& network_graph : network_graphs) {
			    network_graphs_ref.push_back(*network_graph);
		    }
		    std::vector<std::reference_wrapper<signal_flow::IODataMap const>> inputs_ref;
		    for (auto const& input : inputs) {
			    inputs_ref.push_back(*input);
		    }

		    return network::run(conn.get(), configs_ref, network_graphs_ref, inputs_ref);
	    },
	    pybind11::arg("connection"), pybind11::arg("configs"), pybind11::arg("network_graphs"),
	    pybind11::arg("inputs"));
})

} // namespace grenade::vx::network
