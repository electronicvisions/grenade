#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
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
};

/** External spike source population. */
struct GENPYBIND(visible) ExternalPopulation
{
	/** Number of individual sources. */
	size_t size{0};

	ExternalPopulation() = default;
	ExternalPopulation(size_t size) SYMBOL_VISIBLE;
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
