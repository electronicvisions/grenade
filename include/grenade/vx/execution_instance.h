#pragma once
#include "halco/common/geometry.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <stddef.h>

namespace cereal {
class access;
} // namespace cereal

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


struct ExecutionInstance;
size_t hash_value(ExecutionInstance const& e) SYMBOL_VISIBLE;

/**
 * Execution instance identifier.
 * An execution instance describes a unique physically placed isolated execution.
 * It is placed physically on a global DLS instance.
 */
struct ExecutionInstance
{
	ExecutionInstance() = default;

	explicit ExecutionInstance(
	    ExecutionIndex execution_index, halco::hicann_dls::vx::v2::DLSGlobal dls) SYMBOL_VISIBLE;

	halco::hicann_dls::vx::v2::DLSGlobal toDLSGlobal() const SYMBOL_VISIBLE;
	ExecutionIndex toExecutionIndex() const SYMBOL_VISIBLE;

	bool operator==(ExecutionInstance const& other) const SYMBOL_VISIBLE;
	bool operator!=(ExecutionInstance const& other) const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, ExecutionInstance const& instance)
	    SYMBOL_VISIBLE;

	friend size_t hash_value(ExecutionInstance const& e) SYMBOL_VISIBLE;

private:
	ExecutionIndex m_execution_index;
	halco::hicann_dls::vx::v2::DLSGlobal m_dls_global;

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::coordinate

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::coordinate::ExecutionIndex)

template <>
struct hash<grenade::vx::coordinate::ExecutionInstance>
{
	size_t operator()(grenade::vx::coordinate::ExecutionInstance const& t) const
	{
		return grenade::vx::coordinate::hash_value(t);
	}
};

} // namespace std
