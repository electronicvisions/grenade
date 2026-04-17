#pragma once
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"

namespace grenade::vx::execution {
struct JITGraphExecutor;
} // namespace grenade::vx::execution

namespace grenade::common {
struct LinkedTopology;
} // namespace grenade::common

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Calibration operation, which annotates the translation from calibrated to hardware settings into
 * the topology.
 * It uses the placed hardware entities and the mapping, represented by the inter-topology hyper
 * edges to the partitioned reference topology to collect calibration targets. Afterwards, the
 * calibration is performed. The results are set at the inter-topology hyper edges between the
 * hardware topology entities and the partitioned topology entities.
 * TODO: could be in common namespace if we have a common parent for the executor
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE Calibration
{
	/**
	 * TODO: find a way to remove executor from abstract interface and construct JIT calib with
	 * executor reference.
	 */
	virtual void GENPYBIND(hidden) operator()(
	    grenade::common::LinkedTopology& topology,
	    grenade::vx::execution::JITGraphExecutor& executor) const = 0;
};

} // namespace abstract
} // namespace grenade::vx::network
