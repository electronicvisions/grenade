#pragma once
#include "grenade/vx/common/entity_on_chip.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <cstddef>
#include <iosfwd>
#include <vector>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/** External source population. */
struct GENPYBIND(visible) ExternalSourcePopulation : public common::EntityOnChip
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

	ExternalSourcePopulation() = default;
	ExternalSourcePopulation(
	    std::vector<Neuron> neurons,
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
