#pragma once
#include <array>
#include <ostream>
#include <stddef.h>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "hate/visibility.h"

namespace grenade::vx::vertex {

/**
 * Formatted data input from memory.
 */
struct DataInput
{
	constexpr static bool can_connect_different_execution_instances = true;

	/**
	 * Construct DataInput with specified size and data output type.
	 * @param output_type Output data type
	 * @param size Number of data values
	 * @throws std::runtime_error On output data type not supported
	 */
	explicit DataInput(ConnectionType output_type, size_t size) SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, DataInput const& config) SYMBOL_VISIBLE;

	bool operator==(DataInput const& other) const SYMBOL_VISIBLE;
	bool operator!=(DataInput const& other) const SYMBOL_VISIBLE;

private:
	size_t m_size;
	ConnectionType m_output_type;
};

} // grenade::vx::vertex
