#pragma once
#include <iterator>

namespace grenade::vx::common::detail {

/**
 * Output iterator dropping all mutable operations.
 */
template <typename T>
class NullOutputIterator : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
	NullOutputIterator& operator=(T const&)
	{
		return *this;
	}
	NullOutputIterator& operator*()
	{
		return *this;
	}
	NullOutputIterator& operator++()
	{
		return *this;
	}
	NullOutputIterator operator++(int)
	{
		return *this;
	}
};

} // namespace grenade::vx::common::detail
