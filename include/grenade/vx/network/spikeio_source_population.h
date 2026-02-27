#pragma once

#include "grenade/vx/common/entity_on_chip.h"
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/fpga.h"
#include "haldls/vx/fpga.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <vector>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) SpikeIOSourcePopulation : common::EntityOnChip
{
	// --- Per Neuron Flags
	struct GENPYBIND(visible) Neuron
	{
		halco::hicann_dls::vx::SpikeIOAddress label;
		bool enable_record_spikes;

		Neuron(halco::hicann_dls::vx::SpikeIOAddress label, bool enable_record_spikes = false)
		    SYMBOL_VISIBLE;

		bool operator==(Neuron const& other) const = default;
		bool operator!=(Neuron const& other) const = default;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Neuron const& value) SYMBOL_VISIBLE;
	};

	// --- Population Config
	struct GENPYBIND(visible) Config
	{
		bool enable_internal_loopback{true};
		haldls::vx::SpikeIOConfig::DataRateScaler data_rate_scaler{};

		Config() = default;

		bool operator==(Config const& other) const = default;
		bool operator!=(Config const& other) const = default;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Config const& value) SYMBOL_VISIBLE;
	};

	std::vector<Neuron> neurons{};
	Config config{};

	SpikeIOSourcePopulation() = default;
	SpikeIOSourcePopulation(std::vector<Neuron> neurons, Config const& config) SYMBOL_VISIBLE;

	bool operator==(SpikeIOSourcePopulation const& other) const = default;
	bool operator!=(SpikeIOSourcePopulation const& other) const = default;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream&, SpikeIOSourcePopulation const&) SYMBOL_VISIBLE;
};
} // namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK
} // namespace grenade::vx
