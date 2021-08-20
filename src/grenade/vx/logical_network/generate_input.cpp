#include "grenade/vx/logical_network/generate_input.h"

#include "grenade/vx/network/generate_input.h"
#include <algorithm>

namespace grenade::vx::logical_network {

InputGenerator::InputGenerator(
    NetworkGraph const& network_graph, network::NetworkGraph const& hardware_network_graph) :
    m_input_generator(hardware_network_graph), m_network_graph(network_graph)
{
	if (network_graph.get_hardware_network() != hardware_network_graph.get_network()) {
		throw std::runtime_error(
		    "Input generator requires logical and hardware network graph with matching network.");
	}
}

void InputGenerator::add(
    std::vector<TimedSpike::Time> const& times, PopulationDescriptor const population)
{
	m_input_generator.add(times, m_network_graph.get_population_translation().at(population));
}

void InputGenerator::add(
    std::vector<std::vector<TimedSpike::Time>> const& times, PopulationDescriptor const population)
{
	m_input_generator.add(times, m_network_graph.get_population_translation().at(population));
}

IODataMap InputGenerator::done()
{
	return m_input_generator.done();
}

} // namespace grenade::vx::logical_network
