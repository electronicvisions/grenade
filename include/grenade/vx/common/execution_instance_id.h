#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <stddef.h>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::common GENPYBIND_TAG_GRENADE_VX_COMMON {

/**
 * Unique temporal identifier for a execution instance.
 * ExecutionIndex separates execution where a memory barrier is needed for data movement completion.
 * No guarantees are made on execution order.
 */
struct GENPYBIND(inline_base("*")) ExecutionIndex
    : public halco::common::detail::BaseType<ExecutionIndex, size_t>
{
	constexpr explicit ExecutionIndex(value_type const value = 0) : base_t(value) {}
};


struct ExecutionInstanceID;
size_t hash_value(ExecutionInstanceID const& e) SYMBOL_VISIBLE;

/**
 * Execution instance identifier.
 * An execution instance describes a unique physically placed isolated execution.
 * It is placed physically on a global DLS instance.
 */
struct GENPYBIND(visible) ExecutionInstanceID
{
	ExecutionInstanceID() = default;

	explicit ExecutionInstanceID(
	    ExecutionIndex execution_index, halco::hicann_dls::vx::v3::DLSGlobal dls) SYMBOL_VISIBLE;

	halco::hicann_dls::vx::v3::DLSGlobal toDLSGlobal() const SYMBOL_VISIBLE;
	ExecutionIndex toExecutionIndex() const SYMBOL_VISIBLE;

	bool operator==(ExecutionInstanceID const& other) const SYMBOL_VISIBLE;
	bool operator!=(ExecutionInstanceID const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, ExecutionInstanceID const& instance)
	    SYMBOL_VISIBLE;

	friend size_t hash_value(ExecutionInstanceID const& e) SYMBOL_VISIBLE;

	GENPYBIND(expose_as(__hash__))
	size_t hash() const SYMBOL_VISIBLE;

private:
	ExecutionIndex m_execution_index;
	halco::hicann_dls::vx::v3::DLSGlobal m_dls_global;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::common

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::common::ExecutionIndex)

template <>
struct hash<grenade::vx::common::ExecutionInstanceID>
{
	size_t operator()(grenade::vx::common::ExecutionInstanceID const& t) const SYMBOL_VISIBLE;
};

} // namespace std
