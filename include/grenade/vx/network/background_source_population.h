#pragma once
#include "grenade/vx/common/entity_on_chip.h"
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "haldls/vx/v3/background.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <map>

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** Background source population. */
struct GENPYBIND(visible) BackgroundSourcePopulation : common::EntityOnChip
{
	struct Neuron
	{
		bool enable_record_spikes;

		Neuron(bool enable_record_spikes = false) SYMBOL_VISIBLE;

		bool operator==(Neuron const& other) const = default;
		bool operator!=(Neuron const& other) const = default;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Neuron const& value) SYMBOL_VISIBLE;
	};

	std::vector<Neuron> neurons{};

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

	BackgroundSourcePopulation() = default;
	BackgroundSourcePopulation(
	    std::vector<Neuron> neurons,
	    Coordinate const& coordinate,
	    Config const& config,
	    common::EntityOnChip::ChipCoordinate chip_coordinate =
	        common::EntityOnChip::ChipCoordinate()) SYMBOL_VISIBLE;

	bool operator==(BackgroundSourcePopulation const& other) const SYMBOL_VISIBLE;
	bool operator!=(BackgroundSourcePopulation const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, BackgroundSourcePopulation const& population)
	    SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network
