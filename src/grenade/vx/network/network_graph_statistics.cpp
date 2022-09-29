#include "grenade/vx/network/network_graph_statistics.h"

#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include "hate/timer.h"
#include <ostream>

namespace grenade::vx::network {

NetworkGraphStatistics extract_statistics(NetworkGraph const& network_graph)
{
	NetworkGraphStatistics statistics;

	assert(network_graph.get_network());
	auto const& network = *network_graph.get_network();

	statistics.m_num_populations = network.populations.size();
	statistics.m_num_projections = network.projections.size();

	for (auto const& [_, population] : network.populations) {
		if (!std::holds_alternative<Population>(population)) {
			continue;
		}
		statistics.m_num_neurons += std::get<Population>(population).neurons.size();
	}

	for (auto const& [_, projection] : network.projections) {
		statistics.m_num_synapses += projection.connections.size();
	}

	statistics.m_neuron_usage =
	    static_cast<double>(statistics.m_num_neurons) /
	    static_cast<double>(halco::hicann_dls::vx::v3::AtomicNeuronOnDLS::size);

	statistics.m_synapse_usage = static_cast<double>(statistics.m_num_synapses) /
	                             static_cast<double>(
	                                 halco::hicann_dls::vx::v3::SynapseRowOnDLS::size *
	                                 halco::hicann_dls::vx::v3::SynapseOnSynapseRow::size);

	std::set<halco::hicann_dls::vx::v3::SynapseDriverOnDLS> used_synapse_drivers;
	for (auto const d :
	     boost::make_iterator_range(boost::vertices(network_graph.get_graph().get_graph()))) {
		auto const& vertex_property = network_graph.get_graph().get_vertex_property(d);
		if (!std::holds_alternative<signal_flow::vertex::SynapseDriver>(vertex_property)) {
			continue;
		}
		used_synapse_drivers.insert(
		    std::get<signal_flow::vertex::SynapseDriver>(vertex_property).get_coordinate());
	}
	statistics.m_num_synapse_drivers = used_synapse_drivers.size();
	statistics.m_synapse_driver_usage =
	    static_cast<double>(statistics.m_num_synapse_drivers) /
	    static_cast<double>(halco::hicann_dls::vx::v3::SynapseDriverOnDLS::size);

	statistics.m_abstract_network_construction_duration = network.construction_duration;
	statistics.m_hardware_network_construction_duration = network_graph.m_construction_duration;
	statistics.m_verification_duration = network_graph.m_verification_duration;
	statistics.m_routing_duration = network_graph.m_routing_duration;

	return statistics;
}


size_t NetworkGraphStatistics::get_num_populations() const
{
	return m_num_populations;
}

size_t NetworkGraphStatistics::get_num_projections() const
{
	return m_num_projections;
}

size_t NetworkGraphStatistics::get_num_neurons() const
{
	return m_num_neurons;
}

size_t NetworkGraphStatistics::get_num_synapses() const
{
	return m_num_synapses;
}

size_t NetworkGraphStatistics::get_num_synapse_drivers() const
{
	return m_num_synapse_drivers;
}

double NetworkGraphStatistics::get_neuron_usage() const
{
	return m_neuron_usage;
}

double NetworkGraphStatistics::get_synapse_usage() const
{
	return m_synapse_usage;
}

double NetworkGraphStatistics::get_synapse_driver_usage() const
{
	return m_synapse_driver_usage;
}

std::chrono::microseconds NetworkGraphStatistics::get_abstract_network_construction_duration() const
{
	return m_abstract_network_construction_duration;
}

std::chrono::microseconds NetworkGraphStatistics::get_hardware_network_construction_duration() const
{
	return m_hardware_network_construction_duration;
}

std::chrono::microseconds NetworkGraphStatistics::get_verification_duration() const
{
	return m_verification_duration;
}

std::chrono::microseconds NetworkGraphStatistics::get_routing_duration() const
{
	return m_routing_duration;
}

std::ostream& operator<<(std::ostream& os, NetworkGraphStatistics const& value)
{
	os << "NetworkGraphStatistics(\n";
	os << "\tnum populations: " << value.m_num_populations << "\n";
	os << "\tnum projections: " << value.m_num_projections << "\n";
	os << "\tnum neurons: " << value.m_num_neurons << "\n";
	os << "\tnum synapses: " << value.m_num_synapses << "\n";
	os << "\tnum synapse drivers: " << value.m_num_synapse_drivers << "\n";
	os << "\tneuron usage: " << value.m_neuron_usage << "\n";
	os << "\tsynapse usage: " << value.m_synapse_usage << "\n";
	os << "\tsynapse driver usage: " << value.m_synapse_driver_usage << "\n";
	os << "\tabstract network construction duration: "
	   << hate::to_string(value.m_abstract_network_construction_duration) << "\n";
	os << "\thardware network construction duration: "
	   << hate::to_string(value.m_hardware_network_construction_duration) << "\n";
	os << "\tverification duration: " << hate::to_string(value.m_verification_duration) << "\n";
	os << "\trouting duration: " << hate::to_string(value.m_routing_duration) << "\n";
	os << ")";
	return os;
}

} // namespace grenade::vx::network
