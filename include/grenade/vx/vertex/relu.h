#pragma once
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "hate/visibility.h"
#include <array>
#include <cstddef>
#include <ostream>

namespace grenade::vx::vertex {

/**
 * ReLU of multiple inputs of Int8 data type.
 */
struct ReLU
{
	constexpr static bool can_connect_different_execution_instances = false;

	/**
	 * Construct operation with specified size.
	 * @param size Number of data values per input
	 */
	explicit ReLU(size_t size) SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, ReLU const& config) SYMBOL_VISIBLE;

private:
	size_t m_size;
};

} // grenade::vx::vertex
