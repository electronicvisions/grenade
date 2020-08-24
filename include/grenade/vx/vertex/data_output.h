#pragma once
#include <array>
#include <optional>
#include <ostream>
#include <stddef.h>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "hate/visibility.h"

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx::vertex {

/**
 * Formatted data output to memory.
 */
struct DataOutput
{
	constexpr static bool can_connect_different_execution_instances = true;

	DataOutput() = default;

	/**
	 * Construct DataOutput with specified size and data input type.
	 * @param input_type Input data type
	 * @param size Number of data values
	 * @throws std::runtime_error On input data type not supported
	 */
	explicit DataOutput(ConnectionType input_type, size_t size) SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, DataOutput const& config) SYMBOL_VISIBLE;

	bool operator==(DataOutput const& other) const SYMBOL_VISIBLE;
	bool operator!=(DataOutput const& other) const SYMBOL_VISIBLE;

private:
	size_t m_size{};
	ConnectionType m_input_type{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // grenade::vx::vertex
