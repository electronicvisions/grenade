#pragma once
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <cstddef>
#include <iosfwd>

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** External spike source population. */
struct GENPYBIND(visible) ExternalPopulation
{
	/** Number of individual sources. */
	size_t size{0};

	ExternalPopulation() = default;
	ExternalPopulation(size_t size) SYMBOL_VISIBLE;

	bool operator==(ExternalPopulation const& other) const SYMBOL_VISIBLE;
	bool operator!=(ExternalPopulation const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, ExternalPopulation const& population)
	    SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
