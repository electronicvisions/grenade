#pragma once
#include "grenade/vx/common/entity_on_chip.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/receptor.h"
#include "halco/common/geometry.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "haldls/vx/v3/background.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** Population of on-chip neurons. */
struct GENPYBIND(visible) Population : public common::EntityOnChip
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
	Population(
	    Neurons const& neurons,
	    common::EntityOnChip::ChipOnExecutor chip_on_executor =
	        common::EntityOnChip::ChipOnExecutor()) SYMBOL_VISIBLE;

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

} // namespace network
} // namespace grenade::vx
