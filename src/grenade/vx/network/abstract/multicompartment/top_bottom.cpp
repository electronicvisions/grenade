#include "grenade/vx/network/abstract/multicompartment/top_bottom.h"

namespace grenade::vx::network::abstract {

NumberTopBottom::NumberTopBottom() : number_total(0), number_top(0), number_bottom(0) {}

NumberTopBottom::NumberTopBottom(
    size_t number_total_in, size_t number_top_in, size_t number_bottom_in) :
    number_total(number_total_in), number_top(number_top_in), number_bottom(number_bottom_in)
{
	if (number_top_in + number_bottom_in > number_total) {
		throw std::invalid_argument(
		    "Number of total requested Circuits is smaller than sum of Top and Bottom");
	}
}

bool NumberTopBottom::operator==(NumberTopBottom const& other) const
{
	return (
	    (number_total == other.number_total) && (number_bottom == other.number_bottom) &&
	    (number_top == other.number_top));
}

bool NumberTopBottom::operator<(NumberTopBottom const& other) const
{
	return (
	    (number_total < other.number_total) && (number_bottom < other.number_bottom) &&
	    (number_top < other.number_top));
}

bool NumberTopBottom::operator<=(NumberTopBottom const& other) const
{
	return (
	    (number_total <= other.number_total) && (number_bottom <= other.number_bottom) &&
	    (number_top <= other.number_top));
}


bool NumberTopBottom::operator>(NumberTopBottom const& other) const
{
	return (
	    (number_total > other.number_total) || (number_bottom > other.number_bottom) ||
	    (number_top > other.number_top));
}

NumberTopBottom NumberTopBottom::operator+=(NumberTopBottom const& other)
{
	number_total += other.number_total;
	number_top += other.number_top;
	number_bottom += other.number_bottom;

	return (*this);
}

std::unique_ptr<NumberTopBottom> NumberTopBottom::copy() const
{
	return std::make_unique<NumberTopBottom>(*this);
}

std::unique_ptr<NumberTopBottom> NumberTopBottom::move()
{
	return std::make_unique<NumberTopBottom>(std::move(*this));
}

bool NumberTopBottom::is_equal_to(NumberTopBottom const& other) const
{
	return (*this == other);
}

std::ostream& NumberTopBottom::print(std::ostream& os) const
{
	return os << "NumberTotal: " << number_total << " NumberTop: " << number_top
	          << " NumberBottom: " << number_bottom;
}


} // namespace grenade::vx::network::abstract