#include "grenade/vx/data_map.h"

namespace grenade::vx {

void DataMap::merge(DataMap&& other)
{
	uint5.merge(other.uint5);
	int8.merge(other.int8);
}

void DataMap::merge(DataMap& other)
{
	merge(std::forward<DataMap>(other));
}

void DataMap::clear()
{
	uint5.clear();
	int8.clear();
}

} // namespace grenade::vx
