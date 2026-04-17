#pragma once
#include "ccalix/spiking_calib_options.h"
#include "ccalix/spiking_calib_target.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/calibration.h"
#include "hate/visibility.h"
#include <optional>
#include <string>
#include <vector>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Just-in-time calibration for single operation point parameter space.
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE JITCalibration : public Calibration
{
	JITCalibration() = default;

	std::optional<std::vector<std::string>> cache_paths;
	ccalix::SpikingCalibOptions options;
	ccalix::SpikingCalibTarget target;

	virtual void operator()(
	    grenade::common::LinkedTopology& topology,
	    grenade::vx::execution::JITGraphExecutor& executor) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
