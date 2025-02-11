#pragma once
#include "grenade/common/output_data.h"
#include "grenade/vx/genpybind.h"
#include <chrono>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/chrono.h>
#endif

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Global results for BrainScaleS-2 backend.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) ExecutorGlobal : grenade::common::OutputData::Executor
{
	/**
	 * Total duration of execution.
	 * This includes graph traversal, compilation of playback programs and post-processing of
	 * result. data.
	 */
	std::chrono::nanoseconds execution_duration{0};

	virtual std::unique_ptr<grenade::common::OutputData::Executor> copy() const override;
	virtual std::unique_ptr<grenade::common::OutputData::Executor> move() override;

protected:
	virtual bool is_equal_to(grenade::common::OutputData::Executor const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
