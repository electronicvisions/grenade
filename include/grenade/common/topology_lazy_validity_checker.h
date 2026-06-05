#pragma once

namespace grenade::common {

struct Topology;


/**
 * Scope, which disables validity checks per operation on a topology and checks the full topology
 * at destruction.
 */
struct TopologyLazyValidityChecker
{
	/**
	 * Register lazy validity checker with topology.
	 * @throws std::invalid_argument On another lazy validity checker already being active for
	 * topology
	 */
	TopologyLazyValidityChecker(Topology& topology);

	/**
	 * De-register lazy validity checker with topology and check validity of all topology elements.
	 */
	~TopologyLazyValidityChecker();

private:
	Topology& m_topology;
};

} // namespace grenade::common
