#pragma once
#include "dapr/property.h"
#include "grenade/vx/genpybind.h"
#include <cstddef>
#include <ostream>
#include <stdexcept>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) NumberTopBottom
    : public dapr::Property<NumberTopBottom>
{
	NumberTopBottom();
	NumberTopBottom(size_t number_total_in, size_t number_top_in, size_t number_bottom_in);
	size_t number_total = 0;
	size_t number_top = 0;
	size_t number_bottom = 0;

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

} // namespace abstract
} // namespace greande::vx::network
