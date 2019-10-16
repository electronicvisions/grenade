#pragma once
#include <array>
#include <stddef.h>
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "hate/visibility.h"

namespace grenade::vx::vertex {

/**
 * Convert Int8 data to SynapseInputLabel data.
 * The operation employed is right-shift by three (works for ML case, TODO: add more).
 */
struct ConvertInt8ToSynapseInputLabel
{
	constexpr static bool can_connect_different_execution_instances = false;

	/**
	 * Construct with specified size.
	 * @param size Number of converted data values
	 */
	explicit ConvertInt8ToSynapseInputLabel(size_t const size) SYMBOL_VISIBLE;

	std::array<Port, 1> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

private:
	size_t m_size;
};

} // grenade::vx::vertex
