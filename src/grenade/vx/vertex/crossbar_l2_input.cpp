#include "grenade/vx/vertex/crossbar_l2_input.h"

#include "grenade/cerealization.h"
#include "grenade/vx/vertex/external_input.h"

#include <ostream>
#include <stdexcept>

namespace grenade::vx::vertex {

std::ostream& operator<<(std::ostream& os, CrossbarL2Input const&)
{
	os << "CrossbarL2Input()";
	return os;
}

bool CrossbarL2Input::operator==(CrossbarL2Input const&) const
{
	return true;
}

bool CrossbarL2Input::operator!=(CrossbarL2Input const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void CrossbarL2Input::serialize(Archive&, std::uint32_t const)
{}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::CrossbarL2Input)
CEREAL_CLASS_VERSION(grenade::vx::vertex::CrossbarL2Input, 0)
