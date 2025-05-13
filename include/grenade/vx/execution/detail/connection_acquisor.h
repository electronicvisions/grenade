#pragma once
#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include <memory>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "pyhxcomm/vx/connection_handle.h"

namespace grenade::vx::execution::detail {

/**
 * RAII extraction of connection and emplacement back into handle
 */
template <typename T>
struct ConnectionAcquisor
{
	T& handle;
	std::unique_ptr<JITGraphExecutor> executor;

	ConnectionAcquisor(T& t) : handle(t), executor()
	{
		std::map<grenade::common::ConnectionOnExecutor, backend::StatefulConnection> connections;
		connections.emplace(
		    grenade::common::ConnectionOnExecutor(),
		    backend::StatefulConnection(backend::InitializedConnection(std::move(t.get()))));
		executor = std::make_unique<JITGraphExecutor>(std::move(connections));

		auto logger = log4cxx::Logger::getLogger("grenade.execution");
		LOG4CXX_WARN(
		    logger, "Using hxcomm.Connection is deprecated, use the "
		            "grenade.execution.JITGraphExecutor context manager "
		            "instead. Since using hxcomm.Connection directly requires performing "
		            "a hardware initialization, no state is carried-over anyways.");
	}

	~ConnectionAcquisor()
	{
		assert(executor);
		handle.get() = std::move(
		    std::get<typename T::connection_type>(executor->release_connections()
		                                              .at(grenade::common::ConnectionOnExecutor())
		                                              .release()
		                                              .release()));
	}
};

} // namespace grenade::vx::execution::detail
#endif
