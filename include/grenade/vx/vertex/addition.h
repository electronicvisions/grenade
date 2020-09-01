#pragma once
#include <array>
#include <cstddef>
#include <ostream>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "hate/visibility.h"

namespace grenade::vx::vertex {

/**
 * Addition of multiple inputs of Int8 data type.
 */
struct Addition
{
	constexpr static bool can_connect_different_execution_instances = false;

	/**
	 * Construct addition with specified size.
	 * @param size Number of data values per input
	 */
	explicit Addition(size_t size) SYMBOL_VISIBLE;

	constexpr static bool variadic_input = true;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, Addition const& config) SYMBOL_VISIBLE;

	bool operator==(Addition const& other) const SYMBOL_VISIBLE;
	bool operator!=(Addition const& other) const SYMBOL_VISIBLE;

private:
	size_t m_size;
};

} // grenade::vx::vertex
