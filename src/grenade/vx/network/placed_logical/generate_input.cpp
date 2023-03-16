#include "grenade/vx/network/placed_logical/generate_input.h"

#include "grenade/vx/network/placed_atomic/generate_input.h"
#include <algorithm>

namespace grenade::vx::network::placed_logical {

InputGenerator::InputGenerator(
    NetworkGraph const& network_graph,
    network::placed_atomic::NetworkGraph const& hardware_network_graph,
    size_t const batch_size) :
    m_input_generator(hardware_network_graph, batch_size), m_network_graph(network_graph)
{
	if (network_graph.get_hardware_network() != hardware_network_graph.get_network()) {
		throw std::runtime_error(
		    "Input generator requires logical and hardware network graph with matching network.");
	}
}

void InputGenerator::add(
    std::vector<signal_flow::TimedSpike::Time> const& times, PopulationDescriptor const population)
{
	m_input_generator.add(times, m_network_graph.get_population_translation().at(population));
}

void InputGenerator::add(
    std::vector<std::vector<signal_flow::TimedSpike::Time>> const& times,
    PopulationDescriptor const population)
{
	m_input_generator.add(times, m_network_graph.get_population_translation().at(population));
}

void InputGenerator::add(
    std::vector<std::vector<std::vector<signal_flow::TimedSpike::Time>>> const& times,
    PopulationDescriptor const population)
{
	m_input_generator.add(times, m_network_graph.get_population_translation().at(population));
}

signal_flow::IODataMap InputGenerator::done()
{
	return m_input_generator.done();
}

} // namespace grenade::vx::network::placed_logical
