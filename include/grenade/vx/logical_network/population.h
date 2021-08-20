#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/logical_network/receptor.h"
#include "grenade/vx/network/population.h"
#include "halco/common/geometry.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <iosfwd>
#include <unordered_set>
#include <vector>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace logical_network GENPYBIND_MODULE {

/** Population of on-chip neurons. */
struct GENPYBIND(visible) Population
{
	/** List of on-chip neurons. */
	typedef std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> Neurons;
	Neurons neurons{};

	/** List of receptors for each neuron. */
	typedef std::vector<std::unordered_set<Receptor>> Receptors;
	Receptors receptors{};

	/** Enable spike recording. */
	typedef std::vector<bool> EnableRecordSpikes;
	EnableRecordSpikes enable_record_spikes{};

	Population() = default;
	Population(
	    Neurons const& neurons,
	    Receptors const& receptors,
	    EnableRecordSpikes const& enable_record_spikes) SYMBOL_VISIBLE;

	bool operator==(Population const& other) const SYMBOL_VISIBLE;
	bool operator!=(Population const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, Population const& population) SYMBOL_VISIBLE;
};

typedef grenade::vx::network::ExternalPopulation ExternalPopulation;
typedef grenade::vx::network::BackgroundSpikeSourcePopulation BackgroundSpikeSourcePopulation;

/** Descriptor to be used to identify a population. */
struct GENPYBIND(inline_base("*")) PopulationDescriptor
    : public halco::common::detail::BaseType<PopulationDescriptor, size_t>
{
	constexpr explicit PopulationDescriptor(value_type const value = 0) : base_t(value) {}
};

} // namespace logical_network

GENPYBIND_MANUAL({
	parent.attr("logical_network").attr("ExternalPopulation") = parent.attr("ExternalPopulation");
	parent.attr("logical_network").attr("BackgroundSpikeSourcePopulation") =
	    parent.attr("BackgroundSpikeSourcePopulation");
})

} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::logical_network::PopulationDescriptor)

} // namespace std
