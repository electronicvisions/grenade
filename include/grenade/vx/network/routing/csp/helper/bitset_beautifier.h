#pragma once

#include <bitset>
#include <sstream>
#include <string>

namespace grenade::vx::network::routing::csp {

/**
 * Helper for outputting int values as bitsets.
 */
struct BitsetBeautifier
{
	const size_t display_block_size = 4;
	/**
	 * Convert integer value to bitset and stream bits in blocks of four digits into string.
	 * @param value Integer value to convert
	 * @param number_bits Number of bits to include into string
	 * @return String containing the number in binary form in blocks of four bits
	 */
	std::string operator()(int value, size_t number_bits = 32) const;
};

} // namespace grenade::vx::network::routing