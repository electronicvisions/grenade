#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v2/background.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
#include "haldls/vx/v2/background.h"
#include "hate/visibility.h"
#include "lola/vx/v2/neuron.h"
#include <cstddef>
#include <vector>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/** Population of on-chip neurons. */
struct GENPYBIND(visible) Population
{
	/** List of on-chip neurons. */
	typedef std::vector<halco::hicann_dls::vx::v2::AtomicNeuronOnDLS> Neurons;
	Neurons neurons{};
	/** Enable spike recording. */
	typedef std::vector<bool> EnableRecordSpikes;
	EnableRecordSpikes enable_record_spikes{};

	Population() = default;
	Population(Neurons const& neurons, EnableRecordSpikes const& enable_record_spikes)
	    SYMBOL_VISIBLE;

	bool operator==(Population const& other) const SYMBOL_VISIBLE;
	bool operator!=(Population const& other) const SYMBOL_VISIBLE;
};

/** External spike source population. */
struct GENPYBIND(visible) ExternalPopulation
{
	/** Number of individual sources. */
	size_t size{0};

	ExternalPopulation() = default;
	ExternalPopulation(size_t size) SYMBOL_VISIBLE;

	bool operator==(ExternalPopulation const& other) const SYMBOL_VISIBLE;
	bool operator!=(ExternalPopulation const& other) const SYMBOL_VISIBLE;
};

/** BackgroundSpikeSource spike source population. */
struct GENPYBIND(visible) BackgroundSpikeSourcePopulation
{
	/** Number of individual sources. */
	size_t size{0};

	/** Placement of the source. */
	typedef std::map<
	    halco::hicann_dls::vx::v2::HemisphereOnDLS,
	    halco::hicann_dls::vx::v2::PADIBusOnPADIBusBlock>
	    Coordinate;
	Coordinate coordinate;

	/** Configuration of the source. */
	struct Config
	{
		haldls::vx::v2::BackgroundSpikeSource::Period period;
		haldls::vx::v2::BackgroundSpikeSource::Rate rate;
		haldls::vx::v2::BackgroundSpikeSource::Seed seed;
		bool enable_random;

		bool operator==(Config const& other) const SYMBOL_VISIBLE;
		bool operator!=(Config const& other) const SYMBOL_VISIBLE;
	} config;

	BackgroundSpikeSourcePopulation() = default;
	BackgroundSpikeSourcePopulation(size_t size, Coordinate const& coordinate, Config const& config)
	    SYMBOL_VISIBLE;

	bool operator==(BackgroundSpikeSourcePopulation const& other) const SYMBOL_VISIBLE;
	bool operator!=(BackgroundSpikeSourcePopulation const& other) const SYMBOL_VISIBLE;
};

/** Descriptor to be used to identify a population. */
struct GENPYBIND(inline_base("*")) PopulationDescriptor
    : public halco::common::detail::BaseType<PopulationDescriptor, size_t>
{
	constexpr explicit PopulationDescriptor(value_type const value = 0) : base_t(value) {}
};

} // namespace network

} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::PopulationDescriptor)

} // namespace std
