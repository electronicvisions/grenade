#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/routing/greedy/synapse_driver_on_dls_manager.h"
#include "grenade/vx/network/routing/router.h"
#include "grenade/vx/network/routing_result.h"
#include "hate/visibility.h"
#include <chrono>
#include <iosfwd>
#include <memory>
#include <optional>

#if defined(__GENPYBIND__) or defined(__GENPYBIND_GENERATED__)
#include "grenade/vx/network/network.h"
#include <pybind11/chrono.h>
#else
namespace grenade::vx::network {
struct Network;
} // namespace grenade::vx::network
#endif

namespace grenade::vx::network { namespace routing GENPYBIND_TAG_GRENADE_VX_NETWORK_ROUTING {

/**
 * Router using a (partially) greedy algorithm.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) GreedyRouter : public Router
{
	/**
	 * Options of the routing algorithm.
	 */
	struct Options
	{
		Options() SYMBOL_VISIBLE;

		/**
		 * Policy to be used in synapse driver allocation.
		 */
		typedef greedy::SynapseDriverOnDLSManager::AllocationPolicy AllocationPolicy;
		AllocationPolicy synapse_driver_allocation_policy;
		typedef greedy::SynapseDriverOnDLSManager::AllocationPolicyGreedy AllocationPolicyGreedy
		    GENPYBIND(opaque);
		typedef greedy::SynapseDriverOnDLSManager::AllocationPolicyBacktracking
		    AllocationPolicyBacktracking GENPYBIND(opaque);

		/**
		 * Optional timeout to be used in synapse driver allocation algorithm for iteration over
		 * label combinations.
		 */
		std::optional<std::chrono::milliseconds> synapse_driver_allocation_timeout;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Options const& options) SYMBOL_VISIBLE;
	};

	GENPYBIND_MANUAL({
		auto cls = pybind11::class_<
		    ::grenade::vx::network::routing::GreedyRouter::Options::AllocationPolicy>(
		    parent, "_Options_AllocationPolicy");
		cls.def(
		       pybind11::init<::grenade::vx::network::routing::greedy::SynapseDriverOnDLSManager::
		                          AllocationPolicyGreedy>(),
		       pybind11::arg("value") = ::grenade::vx::network::routing::greedy::
		           SynapseDriverOnDLSManager::AllocationPolicyGreedy())
		    .def(
		        pybind11::init<::grenade::vx::network::routing::greedy::SynapseDriverOnDLSManager::
		                           AllocationPolicyBacktracking>(),
		        pybind11::arg("value"));
		parent.attr("Options").attr("AllocationPolicy") = parent.attr("_Options_AllocationPolicy");
	})

	GreedyRouter(Options const& options = Options());
	GreedyRouter(GreedyRouter const&) = delete;
	GreedyRouter& operator=(GreedyRouter const&) = delete;

	std::vector<std::unique_ptr<Router>> routers GENPYBIND(hidden);

	virtual ~GreedyRouter();

	virtual RoutingResult operator()(std::shared_ptr<Network> const& network) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace routing
} // namespace grenade::vx::network
