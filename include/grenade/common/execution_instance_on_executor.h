#pragma once
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/genpybind.h"
#include "hate/visibility.h"
#include <cstddef>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Identifier of execution instance on an executor.
 * Executions can only be synchronous intra-connection.
 */
struct GENPYBIND(visible) ExecutionInstanceOnExecutor
{
	ExecutionInstanceOnExecutor() = default;

	ExecutionInstanceID execution_instance_id;
	ConnectionOnExecutor connection_on_executor;

	explicit ExecutionInstanceOnExecutor(
	    ExecutionInstanceID const& execution_instance_id,
	    ConnectionOnExecutor const& connection_on_executor) SYMBOL_VISIBLE;

	// all variants required for genpybind-based Python wrapping generation to work
	bool operator==(ExecutionInstanceOnExecutor const& other) const SYMBOL_VISIBLE;
	bool operator!=(ExecutionInstanceOnExecutor const& other) const SYMBOL_VISIBLE;
	bool operator<=(ExecutionInstanceOnExecutor const& other) const SYMBOL_VISIBLE;
	bool operator>=(ExecutionInstanceOnExecutor const& other) const SYMBOL_VISIBLE;
	bool operator<(ExecutionInstanceOnExecutor const& other) const SYMBOL_VISIBLE;
	bool operator>(ExecutionInstanceOnExecutor const& other) const SYMBOL_VISIBLE;

	GENPYBIND(expose_as(__hash__))
	size_t hash() const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, ExecutionInstanceOnExecutor const& value)
	    SYMBOL_VISIBLE;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace common
} // namespace grenade

namespace std {

template <>
struct hash<grenade::common::ExecutionInstanceOnExecutor>
{
	std::size_t operator()(grenade::common::ExecutionInstanceOnExecutor const& value) const
	{
		return value.hash();
	}
};

} // namespace std
