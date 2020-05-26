#pragma once
#include "grenade/vx/execution_instance.h"
#include "halco/hicann-dls/vx/chip.h"
#include "hate/visibility.h"

namespace grenade::vx {

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
	coordinate::ExecutionInstance next() SYMBOL_VISIBLE;

	/**
	 * Get current hemisphere.
	 */
	halco::hicann_dls::vx::HemisphereOnDLS get_current_hemisphere() const SYMBOL_VISIBLE;

	/**
	 * Get next execution instance forcing a change of the execution index.
	 */
	coordinate::ExecutionInstance next_index() SYMBOL_VISIBLE;

private:
	halco::hicann_dls::vx::HemisphereOnDLS m_current_hemisphere;
	coordinate::ExecutionIndex m_current_index;
};

} // namespace grenade::vx
