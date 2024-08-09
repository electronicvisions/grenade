#include "grenade/vx/network/network_graph_statistics.h"

#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_graph.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include "hate/indent.h"
#include <ostream>
#include <sstream>

namespace grenade::vx::network {

NetworkGraphStatistics extract_statistics(NetworkGraph const& network_graph)
{
	NetworkGraphStatistics statistics;

	assert(network_graph.get_network());
	auto const& network = *network_graph.get_network();

	for (auto const& [id, execution_instance] : network.execution_instances) {
		auto& statistics_execution_instance = statistics.m_execution_instances[id];

		statistics_execution_instance.m_num_populations = execution_instance.populations.size();
		statistics_execution_instance.m_num_projections = execution_instance.projections.size();

		for (auto const& [_, population] : execution_instance.populations) {
			if (!std::holds_alternative<Population>(population)) {
				continue;
			}
			statistics_execution_instance.m_num_neurons +=
			    std::get<Population>(population).neurons.size();
		}

		for (auto const& [_, projection] : execution_instance.projections) {
			statistics_execution_instance.m_num_synapses += projection.connections.size();
		}

		statistics_execution_instance.m_neuron_usage =
		    static_cast<double>(statistics_execution_instance.m_num_neurons) /
		    static_cast<double>(halco::hicann_dls::vx::v3::AtomicNeuronOnDLS::size);

		size_t num_used_hw_synapses = 0;
		std::set<halco::hicann_dls::vx::v3::SynapseDriverOnDLS> used_synapse_drivers;
		for (auto const& [d, _] : boost::make_iterator_range(
		         network_graph.get_graph().get_vertex_descriptor_map().right.equal_range(
		             network_graph.get_graph().get_execution_instance_map().right.at(id)))) {
			auto const& vertex_property = network_graph.get_graph().get_vertex_property(d);
			if (std::holds_alternative<signal_flow::vertex::SynapseDriver>(vertex_property)) {
				used_synapse_drivers.insert(
				    std::get<signal_flow::vertex::SynapseDriver>(vertex_property).get_coordinate());
			} else if (std::holds_alternative<signal_flow::vertex::SynapseArrayViewSparse>(
			               vertex_property)) {
				num_used_hw_synapses +=
				    std::get<signal_flow::vertex::SynapseArrayViewSparse>(vertex_property)
				        .get_synapses()
				        .size();
			}
		}
		statistics_execution_instance.m_synapse_usage =
		    static_cast<double>(num_used_hw_synapses) /
		    static_cast<double>(
		        halco::hicann_dls::vx::v3::SynapseRowOnDLS::size *
		        halco::hicann_dls::vx::v3::SynapseOnSynapseRow::size);

		statistics_execution_instance.m_num_synapse_drivers = used_synapse_drivers.size();

		statistics_execution_instance.m_synapse_driver_usage =
		    static_cast<double>(statistics_execution_instance.m_num_synapse_drivers) /
		    static_cast<double>(halco::hicann_dls::vx::v3::SynapseDriverOnDLS::size);
	}

	statistics.m_abstract_network_construction_duration = network.construction_duration;
	statistics.m_hardware_network_construction_duration = network_graph.m_construction_duration;
	statistics.m_verification_duration = network_graph.m_verification_duration;
	statistics.m_routing_duration = network_graph.m_routing_duration;

	return statistics;
}


size_t NetworkGraphStatistics::ExecutionInstance::get_num_populations() const
{
	return m_num_populations;
}

size_t NetworkGraphStatistics::ExecutionInstance::get_num_projections() const
{
	return m_num_projections;
}

size_t NetworkGraphStatistics::ExecutionInstance::get_num_neurons() const
{
	return m_num_neurons;
}

size_t NetworkGraphStatistics::ExecutionInstance::get_num_synapses() const
{
	return m_num_synapses;
}

size_t NetworkGraphStatistics::ExecutionInstance::get_num_synapse_drivers() const
{
	return m_num_synapse_drivers;
}

double NetworkGraphStatistics::ExecutionInstance::get_neuron_usage() const
{
	return m_neuron_usage;
}

double NetworkGraphStatistics::ExecutionInstance::get_synapse_usage() const
{
	return m_synapse_usage;
}

double NetworkGraphStatistics::ExecutionInstance::get_synapse_driver_usage() const
{
	return m_synapse_driver_usage;
}

std::ostream& operator<<(std::ostream& os, NetworkGraphStatistics::ExecutionInstance const& value)
{
	hate::IndentingOstream ios(os);
	ios << "ExecutionInstance(\n";
	ios << hate::Indentation("\t");
	ios << "num populations: " << value.m_num_populations << "\n";
	ios << "num projections: " << value.m_num_projections << "\n";
	ios << "num neurons: " << value.m_num_neurons << "\n";
	ios << "num synapses: " << value.m_num_synapses << "\n";
	ios << "num synapse drivers: " << value.m_num_synapse_drivers << "\n";
	ios << "neuron usage: " << value.m_neuron_usage << "\n";
	ios << "synapse usage: " << value.m_synapse_usage << "\n";
	ios << "synapse driver usage: " << value.m_synapse_driver_usage << "\n";
	ios << hate::Indentation() << ")";
	return os;
}


std::map<grenade::common::ExecutionInstanceID, NetworkGraphStatistics::ExecutionInstance> const&
NetworkGraphStatistics::get_execution_instances() const
{
	return m_execution_instances;
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
	hate::IndentingOstream ios(os);
	ios << "NetworkGraphStatistics(\n";
	for (auto const& [id, execution_instance] : value.m_execution_instances) {
		ios << hate::Indentation("\t") << id << ":\n";
		ios << hate::Indentation("\t\t") << execution_instance << "\n";
	}
	ios << hate::Indentation("\t");
	ios << "abstract network construction duration: "
	    << hate::to_string(value.m_abstract_network_construction_duration) << "\n";
	ios << "hardware network construction duration: "
	    << hate::to_string(value.m_hardware_network_construction_duration) << "\n";
	ios << "verification duration: " << hate::to_string(value.m_verification_duration) << "\n";
	ios << "routing duration: " << hate::to_string(value.m_routing_duration) << "\n";
	ios << hate::Indentation() << ")";
	return os;
}

} // namespace grenade::vx::network
