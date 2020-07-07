#include "grenade/vx/range_split.h"

namespace grenade::vx {

RangeSplit::RangeSplit(size_t const split_size) : m_split_size(split_size) {}

std::vector<RangeSplit::SubRange> RangeSplit::operator()(size_t const size) const
{
	std::vector<SubRange> split_ranges;

	size_t offset = 0;
	while (offset < size) {
		size_t const local_size = std::min(m_split_size, size - offset);

		SubRange local_sub_range;
		local_sub_range.size = local_size;
		local_sub_range.offset = offset;
		split_ranges.push_back(local_sub_range);

		offset += local_size;
	}
	return split_ranges;
}

} // namespace grenade::vx
