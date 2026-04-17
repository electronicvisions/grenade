#pragma once
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/calibration.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include <map>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Fixture "calibration" using provided chip objects.
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE FixtureCalibration : public Calibration
{
	FixtureCalibration() = default;

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::map<grenade::vx::common::ChipOnConnection, lola::vx::v3::Chip>>
	    chips;

	virtual void operator()(
	    grenade::common::LinkedTopology& topology,
	    grenade::vx::execution::JITGraphExecutor& executor) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
