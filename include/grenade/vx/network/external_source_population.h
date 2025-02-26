#pragma once
#include "grenade/vx/common/entity_on_chip.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <cstddef>
#include <iosfwd>

namespace grenade::vx { namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** External source population. */
struct GENPYBIND(visible) ExternalSourcePopulation : public common::EntityOnChip
{
	/** Number of individual sources. */
	size_t size{0};

	ExternalSourcePopulation() = default;
	ExternalSourcePopulation(
	    size_t size,
	    common::EntityOnChip::ChipCoordinate chip_coordinate =
	        common::EntityOnChip::ChipCoordinate()) SYMBOL_VISIBLE;

	bool operator==(ExternalSourcePopulation const& other) const SYMBOL_VISIBLE;
	bool operator!=(ExternalSourcePopulation const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, ExternalSourcePopulation const& population)
	    SYMBOL_VISIBLE;
};

} // namespace network
} // namespace grenade::vx
