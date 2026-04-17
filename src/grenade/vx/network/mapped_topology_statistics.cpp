#include "grenade/vx/network/mapped_topology_statistics.h"

#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"
#include "grenade/vx/signal_flow/vertex/synapse_driver.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include "hate/indent.h"
#include "hate/timer.h"
#include <ostream>
#include <sstream>

namespace grenade::vx::network {

MappedTopologyStatistics extract_statistics(grenade::common::LinkedTopology const& topology)
{
	MappedTopologyStatistics statistics;

	auto const& partitioned_topology = topology.get_reference();
	for (auto const& partitioned_vertex_descriptor : partitioned_topology.vertices()) {
		auto const& partitioned_vertex = dynamic_cast<grenade::common::PartitionedVertex const&>(
		    partitioned_topology.get(partitioned_vertex_descriptor));
		auto const execution_instance =
		    partitioned_vertex.get_execution_instance_on_executor().value();

		auto& statistics_execution_instance = statistics.m_execution_instances[execution_instance];

		if (auto const population =
		        dynamic_cast<grenade::common::Population const*>(&partitioned_vertex);
		    population) {
			statistics_execution_instance.m_num_populations++;
			if (auto const locally_placed_neuron =
			        dynamic_cast<abstract::LocallyPlacedNeuron const*>(&population->get_cell());
			    locally_placed_neuron) {
				statistics_execution_instance.m_num_neurons += population->size();
			}
		} else if (auto const projection =
		               dynamic_cast<grenade::common::Projection const*>(&partitioned_vertex);
		           projection) {
			statistics_execution_instance.m_num_projections++;
			statistics_execution_instance.m_num_synapses +=
			    projection->get_connector().get_num_synapses(
			        *projection->get_connector().get_input_sequence()->cartesian_product(
			            *projection->get_connector().get_output_sequence()));
		}
	}

	std::map<grenade::common::ExecutionInstanceOnExecutor, size_t> num_used_hw_synapses;
	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::set<halco::hicann_dls::vx::v3::SynapseDriverOnDLS>>
	    used_synapse_drivers;
	for (auto const& mapped_vertex_descriptor : topology.vertices()) {
		auto const& mapped_vertex = dynamic_cast<grenade::common::PartitionedVertex const&>(
		    topology.get(mapped_vertex_descriptor));
		auto const execution_instance = mapped_vertex.get_execution_instance_on_executor().value();

		if (auto const synapse_driver =
		        dynamic_cast<signal_flow::vertex::SynapseDriver const*>(&mapped_vertex);
		    synapse_driver) {
			used_synapse_drivers[execution_instance].insert(synapse_driver->coordinate);
		}
		if (auto const synapse_array_view =
		        dynamic_cast<signal_flow::vertex::SynapseArrayViewSparse const*>(&mapped_vertex);
		    synapse_array_view) {
			num_used_hw_synapses[execution_instance] += synapse_array_view->get_synapses().size();
		}
	}

	for (auto& [execution_instance, statistics_execution_instance] :
	     statistics.m_execution_instances) {
		statistics_execution_instance.m_neuron_usage =
		    static_cast<double>(statistics_execution_instance.m_num_neurons) /
		    static_cast<double>(halco::hicann_dls::vx::v3::AtomicNeuronOnDLS::size);

		statistics_execution_instance.m_synapse_usage =
		    static_cast<double>(num_used_hw_synapses[execution_instance]) /
		    static_cast<double>(
		        halco::hicann_dls::vx::v3::SynapseRowOnDLS::size *
		        halco::hicann_dls::vx::v3::SynapseOnSynapseRow::size);

		statistics_execution_instance.m_num_synapse_drivers =
		    used_synapse_drivers[execution_instance].size();

		statistics_execution_instance.m_synapse_driver_usage =
		    static_cast<double>(statistics_execution_instance.m_num_synapse_drivers) /
		    static_cast<double>(halco::hicann_dls::vx::v3::SynapseDriverOnDLS::size);
	}

	return statistics;
}


size_t MappedTopologyStatistics::ExecutionInstance::get_num_populations() const
{
	return m_num_populations;
}

size_t MappedTopologyStatistics::ExecutionInstance::get_num_projections() const
{
	return m_num_projections;
}

size_t MappedTopologyStatistics::ExecutionInstance::get_num_neurons() const
{
	return m_num_neurons;
}

size_t MappedTopologyStatistics::ExecutionInstance::get_num_synapses() const
{
	return m_num_synapses;
}

size_t MappedTopologyStatistics::ExecutionInstance::get_num_synapse_drivers() const
{
	return m_num_synapse_drivers;
}

double MappedTopologyStatistics::ExecutionInstance::get_neuron_usage() const
{
	return m_neuron_usage;
}

double MappedTopologyStatistics::ExecutionInstance::get_synapse_usage() const
{
	return m_synapse_usage;
}

double MappedTopologyStatistics::ExecutionInstance::get_synapse_driver_usage() const
{
	return m_synapse_driver_usage;
}

std::ostream& operator<<(std::ostream& os, MappedTopologyStatistics::ExecutionInstance const& value)
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


std::map<
    grenade::common::ExecutionInstanceOnExecutor,
    MappedTopologyStatistics::ExecutionInstance> const&
MappedTopologyStatistics::get_execution_instances() const
{
	return m_execution_instances;
}

std::chrono::microseconds MappedTopologyStatistics::get_abstract_network_construction_duration()
    const
{
	return m_abstract_network_construction_duration;
}

std::chrono::microseconds MappedTopologyStatistics::get_hardware_network_construction_duration()
    const
{
	return m_hardware_network_construction_duration;
}

std::chrono::microseconds MappedTopologyStatistics::get_verification_duration() const
{
	return m_verification_duration;
}

std::chrono::microseconds MappedTopologyStatistics::get_routing_duration() const
{
	return m_routing_duration;
}

std::ostream& operator<<(std::ostream& os, MappedTopologyStatistics const& value)
{
	hate::IndentingOstream ios(os);
	ios << "MappedTopologyStatistics(\n";
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
