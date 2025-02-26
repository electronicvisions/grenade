#pragma once

#include "grenade/vx/network/abstract/property.h"
#include <cstddef>
#include <ostream>
#include <stdexcept>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) NumberTopBottom : public Property<NumberTopBottom>
{
	NumberTopBottom();
	NumberTopBottom(size_t number_total_in, size_t number_top_in, size_t number_bottom_in);
	size_t number_total;
	size_t number_top;
	size_t number_bottom;

	bool operator==(NumberTopBottom const& other) const;
	bool operator<(NumberTopBottom const& other) const;
	bool operator<=(NumberTopBottom const& other) const;
	bool operator>(NumberTopBottom const& other) const;
	NumberTopBottom operator+=(NumberTopBottom const& other);

	// Property-Methods: Copy, Move, Equal, Print
	std::unique_ptr<NumberTopBottom> copy() const;
	std::unique_ptr<NumberTopBottom> move();

protected:
	bool is_equal_to(NumberTopBottom const& other) const;
	std::ostream& print(std::ostream& os) const;
};

} // namespace network
} // namespace greande::vx
