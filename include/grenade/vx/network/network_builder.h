#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "hate/visibility.h"
#include <map>
#include <memory>
#include <variant>

namespace log4cxx {
class Logger;
} // namespace log4cxx

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

class GENPYBIND(visible) NetworkBuilder
{
public:
	/**
	 * Add on-chip population.
	 * The population is expected to feature unique and unused neuron locations.
	 * @param population Population to add
	 */
	PopulationDescriptor add(Population const& population) SYMBOL_VISIBLE;

	/**
	 * Add off-chip population.
	 * @param population Population to add
	 */
	PopulationDescriptor add(ExternalPopulation const& population) SYMBOL_VISIBLE;

	/**
	 * Add projection between already added populations.
	 * The projection is expected to be free of single connections present in already added
	 * projections. A single connection is considered equal, if it connects the same pre- and
	 * post-synaptic neurons and features the same receptor type.
	 * @param projection Projection to add
	 */
	ProjectionDescriptor add(Projection const& projection) SYMBOL_VISIBLE;

	NetworkBuilder() SYMBOL_VISIBLE;

	std::shared_ptr<Network> done() SYMBOL_VISIBLE;

private:
	std::map<PopulationDescriptor, std::variant<Population, ExternalPopulation>> m_populations{};
	std::map<ProjectionDescriptor, Projection> m_projections{};
	log4cxx::Logger* m_logger;
};

} // namespace network

} // namespace grenade::vx
