#include "grenade/common/multi_index.h"

#include "hate/join.h"
#include <ostream>

namespace grenade::common {

MultiIndex::MultiIndex(std::vector<size_t> value) : value(std::move(value)) {}

std::ostream& operator<<(std::ostream& os, MultiIndex const& value)
{
	return os << "MultiIndex(" << hate::join(value.value, ", ") << ")";
}

} // namespace grenade::common
