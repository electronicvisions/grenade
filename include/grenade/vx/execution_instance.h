#pragma once
#include <ostream>
#include <stddef.h>
#include "halco/common/geometry.h"
#include "halco/hicann-dls/vx/chip.h"
#include "hate/visibility.h"

namespace grenade::vx::coordinate {

/**
 * Unique temporal identifier for a execution instance.
 * ExecutionIndex separates execution where a memory barrier is needed for data movement completion.
 * No guarantees are made on execution order
 * TODO: move to halco? Is this a hardware abstraction layer coordinate?
 */
struct ExecutionIndex : public halco::common::detail::BaseType<ExecutionIndex, size_t>
{
	constexpr explicit ExecutionIndex(value_type const value = 0) : base_t(value) {}
};


/**
 * Execution instance identifier.
 * An execution instance describes a unique physically placed isolated execution.
 * It is placed physically on a global DLS instance.
 */
struct ExecutionInstance
{
	ExecutionInstance() = default;

	explicit ExecutionInstance(ExecutionIndex temporal_index, halco::hicann_dls::vx::DLSGlobal dls)
	    SYMBOL_VISIBLE;

	halco::hicann_dls::vx::DLSGlobal toDLSGlobal() const SYMBOL_VISIBLE;
	ExecutionIndex toExecutionIndex() const SYMBOL_VISIBLE;

	bool operator<(ExecutionInstance const& other) const SYMBOL_VISIBLE;
	bool operator>(ExecutionInstance const& other) const SYMBOL_VISIBLE;
	bool operator<=(ExecutionInstance const& other) const SYMBOL_VISIBLE;
	bool operator>=(ExecutionInstance const& other) const SYMBOL_VISIBLE;
	bool operator==(ExecutionInstance const& other) const SYMBOL_VISIBLE;
	bool operator!=(ExecutionInstance const& other) const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, ExecutionInstance const& instance)
	    SYMBOL_VISIBLE;

private:
	ExecutionIndex m_temporal_index;
	halco::hicann_dls::vx::DLSGlobal m_dls_global;
};

} // namespace grenade::vx::coordinate

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::coordinate::ExecutionIndex)

} // namespace std
