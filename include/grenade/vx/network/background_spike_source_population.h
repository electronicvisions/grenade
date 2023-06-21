#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "haldls/vx/v3/background.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <map>

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** BackgroundSpikeSource spike source population. */
struct GENPYBIND(visible) BackgroundSpikeSourcePopulation
{
	/** Number of individual sources. */
	size_t size{0};

	/** Placement of the source. */
	typedef std::map<
	    halco::hicann_dls::vx::v3::HemisphereOnDLS,
	    halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock>
	    Coordinate;
	Coordinate coordinate;

	/** Configuration of the source. */
	struct Config
	{
		haldls::vx::v3::BackgroundSpikeSource::Period period;
		haldls::vx::v3::BackgroundSpikeSource::Rate rate;
		haldls::vx::v3::BackgroundSpikeSource::Seed seed;
		bool enable_random;

		bool operator==(Config const& other) const SYMBOL_VISIBLE;
		bool operator!=(Config const& other) const SYMBOL_VISIBLE;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Config const& config) SYMBOL_VISIBLE;
	} config;

	BackgroundSpikeSourcePopulation() = default;
	BackgroundSpikeSourcePopulation(size_t size, Coordinate const& coordinate, Config const& config)
	    SYMBOL_VISIBLE;

	bool operator==(BackgroundSpikeSourcePopulation const& other) const SYMBOL_VISIBLE;
	bool operator!=(BackgroundSpikeSourcePopulation const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(
	    std::ostream& os, BackgroundSpikeSourcePopulation const& population) SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
