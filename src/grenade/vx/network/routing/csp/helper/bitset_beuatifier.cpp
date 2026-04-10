#include "grenade/vx/network/routing/csp/helper/bitset_beautifier.h"

#include <climits>

namespace grenade::vx::network::routing::csp {


std::string BitsetBeautifier::operator()(int value, size_t number_bits) const
{
	std::stringstream ss;
	std::bitset<sizeof(int) * CHAR_BIT> bits(value);

	size_t highest_bit = std::min(number_bits - 1, bits.size() - 1);
	highest_bit += (highest_bit + 1) % display_block_size;

	for (int i = highest_bit; i >= int(display_block_size) - 1; i -= display_block_size) {
		for (int j = i; j > i - int(display_block_size); j--) {
			ss << bits[j];
		}
		ss << " ";
	}
	return ss.str();
}


} // namespace grenade::vx::network::routing::csp