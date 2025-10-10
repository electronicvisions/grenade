#pragma once
#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>

namespace grenade::common::detail {

template <typename T, typename BackendT>
struct DescriptorTransform
{
	boost::bimap<boost::bimaps::set_of<T>, boost::bimaps::set_of<BackendT>> const* map;

	template <typename U>
	constexpr T operator()(U const& value) const
	{
		assert(map);
		return T(map->right.at(value));
	}
};

} // namespace grenade::common::detail
