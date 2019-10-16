#include "grenade/vx/config.h"

#include "lola/vx/hana.h"

namespace grenade::vx {

HemisphereConfig::HemisphereConfig() {}

bool HemisphereConfig::operator==(HemisphereConfig const& other) const
{
	return equal(*this, other);
}

bool HemisphereConfig::operator!=(HemisphereConfig const& other) const
{
	return unequal(*this, other);
}


ChipConfig::ChipConfig() {}

bool ChipConfig::operator==(ChipConfig const& other) const
{
	return equal(*this, other);
}

bool ChipConfig::operator!=(ChipConfig const& other) const
{
	return unequal(*this, other);
}

} // namespace grenade::vx
