#pragma once
#include "grenade/vx/common/execution_instance_id.h"
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
	common::ExecutionInstanceID next() SYMBOL_VISIBLE;

	/**
	 * Get current hemisphere.
	 */
	halco::hicann_dls::vx::v3::HemisphereOnDLS get_current_hemisphere() const SYMBOL_VISIBLE;

	/**
	 * Get next execution instance forcing a change of the execution index.
	 */
	common::ExecutionInstanceID next_index() SYMBOL_VISIBLE;

private:
	halco::hicann_dls::vx::v3::HemisphereOnDLS m_current_hemisphere;
	common::ExecutionInstanceID m_current_instance;
};

} // namespace grenade::vx::compute::detail
