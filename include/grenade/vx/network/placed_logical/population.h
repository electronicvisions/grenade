#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_logical/receptor.h"
#include "halco/common/geometry.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "haldls/vx/v3/background.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <iosfwd>
#include <map>
#include <optional>
#include <unordered_set>
#include <vector>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

/** Population of on-chip neurons. */
struct GENPYBIND(visible) Population
{
	/** On-chip neuron. */
	struct Neuron
	{
		/** Location and shape. */
		typedef halco::hicann_dls::vx::v3::LogicalNeuronOnDLS Coordinate;
		Coordinate coordinate;

		/** Compartment properties. */
		struct Compartment
		{
			/**
			 * The spike master is used for outgoing connections and recording of spikes.
			 * Not every compartment is required to feature a spike master, e.g. if it is passive.
			 */
			struct SpikeMaster
			{
				/** Index of neuron on compartment. */
				size_t neuron_on_compartment;

				/** Enable spike recording. */
				bool enable_record_spikes;

				SpikeMaster(size_t neuron_on_compartment, bool enable_record_spikes) SYMBOL_VISIBLE;

				bool operator==(SpikeMaster const& other) const SYMBOL_VISIBLE;
				bool operator!=(SpikeMaster const& other) const SYMBOL_VISIBLE;

				GENPYBIND(stringstream)
				friend std::ostream& operator<<(std::ostream& os, SpikeMaster const& config)
				    SYMBOL_VISIBLE;
			};
			std::optional<SpikeMaster> spike_master;

			/** Receptors per atomic neuron on compartment. */
			typedef std::vector<std::unordered_set<Receptor>> Receptors;
			Receptors receptors;

			Compartment(std::optional<SpikeMaster> const& spike_master, Receptors const& receptors)
			    SYMBOL_VISIBLE;

			bool operator==(Compartment const& other) const SYMBOL_VISIBLE;
			bool operator!=(Compartment const& other) const SYMBOL_VISIBLE;

			GENPYBIND(stringstream)
			friend std::ostream& operator<<(std::ostream& os, Compartment const& config)
			    SYMBOL_VISIBLE;
		};

		/* Map of compartment properties. */
		typedef std::map<halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron, Compartment>
		    Compartments;
		Compartments compartments;

		Neuron(Coordinate const& coordinate, Compartments const& compartments) SYMBOL_VISIBLE;

		bool operator==(Neuron const& other) const SYMBOL_VISIBLE;
		bool operator!=(Neuron const& other) const SYMBOL_VISIBLE;

		/**
		 * Check validity of neuron config.
		 */
		bool valid() const SYMBOL_VISIBLE;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Neuron const& config) SYMBOL_VISIBLE;
	};

	/** List of on-chip neurons. */
	typedef std::vector<Neuron> Neurons;
	Neurons neurons{};

	Population() = default;
	Population(Neurons const& neurons) SYMBOL_VISIBLE;

	bool operator==(Population const& other) const SYMBOL_VISIBLE;
	bool operator!=(Population const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, Population const& population) SYMBOL_VISIBLE;

	/**
	 * Check validity of projection config.
	 */
	bool valid() const SYMBOL_VISIBLE;

	std::set<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> get_atomic_neurons() const
	    SYMBOL_VISIBLE;

	size_t get_atomic_neurons_size() const SYMBOL_VISIBLE;
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

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, ExternalPopulation const& population)
	    SYMBOL_VISIBLE;
};


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


/** Descriptor to be used to identify a population. */
struct GENPYBIND(inline_base("*")) PopulationDescriptor
    : public halco::common::detail::BaseType<PopulationDescriptor, size_t>
{
	constexpr explicit PopulationDescriptor(value_type const value = 0) : base_t(value) {}
};

} // namespace grenade::vx::network::placed_logical

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::placed_logical::PopulationDescriptor)

} // namespace std
