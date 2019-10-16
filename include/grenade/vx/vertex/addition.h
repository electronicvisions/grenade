#pragma once
#include <stddef.h>
#include <vector>
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
	 * @param num_inputs Number of inputs
	 */
	explicit Addition(size_t size, size_t num_inputs = 2) SYMBOL_VISIBLE;

	std::vector<Port> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

private:
	size_t m_size;
	size_t m_num_inputs;
};

} // grenade::vx::vertex
