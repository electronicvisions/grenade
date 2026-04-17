#pragma once
#include "grenade/common/execution_instance_on_executor.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/visibility.h"

namespace grenade::vx::compute::detail {

/**
 * Manager for execution instances of a single chip.
 * Execution indices are allocated linearly and hemispheres are used alternatingly.
 */
class SingleChipExecutionInstanceManager
{
public:
	SingleChipExecutionInstanceManager() = default;

	/**
	 * Get next execution instance.
	 */
	grenade::common::ExecutionInstanceOnExecutor next() SYMBOL_VISIBLE;

	/**
	 * Get current hemisphere.
	 */
	halco::hicann_dls::vx::v3::HemisphereOnDLS get_current_hemisphere() const SYMBOL_VISIBLE;

	/**
	 * Get next execution instance forcing a change of the execution index.
	 */
	grenade::common::ExecutionInstanceOnExecutor next_index() SYMBOL_VISIBLE;

private:
	halco::hicann_dls::vx::v3::HemisphereOnDLS m_current_hemisphere;
	grenade::common::ExecutionInstanceOnExecutor m_current_instance;
};

} // namespace grenade::vx::compute::detail
