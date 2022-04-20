#include "grenade/vx/network/connectum.h"

#include "grenade/vx/network/exception.h"
#include "grenade/vx/network/network_graph.h"
#include "hate/timer.h"
#include <functional>
#include <ostream>
#include <log4cxx/logger.h>

namespace grenade::vx::network {

bool ConnectumConnection::operator==(ConnectumConnection const& other) const
{
	return std::tie(source, target, receptor_type, weight) ==
	       std::tie(source, target, other.receptor_type, other.weight);
}

bool ConnectumConnection::operator!=(ConnectumConnection const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, ConnectumConnection const& config)
{
	return os << "ConnectumConnection(source: (" << config.source.first << ", "
	          << config.source.second << ") target: " << config.target
	          << " receptor_type: " << config.receptor_type << " weight: " << config.weight << ")";
}


Connectum generate_connectum_from_abstract_network(NetworkGraph const& network_graph)
{
	Connectum connectum;
	assert(network_graph.get_network());
	auto const& network = *(network_graph.get_network());
	for (auto const& [descriptor, projection] : network.projections) {
		auto const& neurons_post =
		    std::get<Population>(network.populations.at(projection.population_post)).neurons;
		for (auto const& connection : projection.connections) {
			connectum.push_back(
			    ConnectumConnection{std::pair{projection.population_pre, connection.index_pre},
			                        neurons_post.at(connection.index_post),
			                        projection.receptor_type, connection.weight});
		}
	}
	return connectum;
}

namespace {

struct HardwareConnectionPath
{
	vertex::CrossbarNode const& crossbar_node;
	vertex::SynapseDriver const& synapse_driver;
	vertex::SynapseArrayViewSparse const& synapse_array_view;
	std::vector<size_t> matching_synapses;
};

} // namespace

Connectum generate_connectum_from_hardware_network(NetworkGraph const& network_graph)
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	auto logger = log4cxx::Logger::getLogger("grenade.generate_connectum_from_hardware_network()");
	hate::Timer timer;

	Connectum connectum;
	assert(network_graph.get_network());
	auto const& network = *(network_graph.get_network());
	auto const& graph = network_graph.get_graph();

	hate::Timer timer_extract_vertex_properties;
	std::vector<std::reference_wrapper<vertex::CrossbarNode const>> crossbar_nodes;
	std::vector<std::reference_wrapper<vertex::SynapseDriver const>> synapse_drivers;
	std::vector<std::reference_wrapper<vertex::SynapseArrayViewSparse const>> synapse_array_views;
	std::vector<std::reference_wrapper<vertex::BackgroundSpikeSource const>>
	    background_spike_sources;
	std::set<NeuronEventOutputOnDLS> active_neuron_event_outputs;
	for (auto const vertex : boost::make_iterator_range(boost::vertices(graph.get_graph()))) {
		if (std::holds_alternative<vertex::CrossbarNode>(graph.get_vertex_property(vertex))) {
			auto const& crossbar_node =
			    std::get<vertex::CrossbarNode>(graph.get_vertex_property(vertex));
			if (std::find_if(
			        crossbar_nodes.begin(), crossbar_nodes.end(), [crossbar_node](auto const& a) {
				        return a.get() == crossbar_node;
			        }) == crossbar_nodes.end()) {
				crossbar_nodes.push_back(crossbar_node);
			}
		} else if (std::holds_alternative<vertex::SynapseDriver>(
		               graph.get_vertex_property(vertex))) {
			auto const& synapse_driver =
			    std::get<vertex::SynapseDriver>(graph.get_vertex_property(vertex));
			if (std::find_if(
			        synapse_drivers.begin(), synapse_drivers.end(),
			        [synapse_driver](auto const& a) { return a.get() == synapse_driver; }) ==
			    synapse_drivers.end()) {
				synapse_drivers.push_back(synapse_driver);
			}
		} else if (std::holds_alternative<vertex::SynapseArrayViewSparse>(
		               graph.get_vertex_property(vertex))) {
			auto const& synapse_array_view =
			    std::get<vertex::SynapseArrayViewSparse>(graph.get_vertex_property(vertex));
			if (std::find_if(
			        synapse_array_views.begin(), synapse_array_views.end(),
			        [synapse_array_view](auto const& a) {
				        return a.get() == synapse_array_view;
			        }) == synapse_array_views.end()) {
				synapse_array_views.push_back(synapse_array_view);
			}
		} else if (std::holds_alternative<vertex::BackgroundSpikeSource>(
		               graph.get_vertex_property(vertex))) {
			auto const& background_spike_source =
			    std::get<vertex::BackgroundSpikeSource>(graph.get_vertex_property(vertex));
			if (std::find_if(
			        background_spike_sources.begin(), background_spike_sources.end(),
			        [background_spike_source](auto const& a) {
				        return a.get() == background_spike_source;
			        }) == background_spike_sources.end()) {
				background_spike_sources.push_back(background_spike_source);
			}
		} else if (std::holds_alternative<vertex::NeuronEventOutputView>(
		               graph.get_vertex_property(vertex))) {
			auto const& neuron_event_output_view =
			    std::get<vertex::NeuronEventOutputView>(graph.get_vertex_property(vertex));
			for (auto const& [_, ccc] : neuron_event_output_view.get_neurons()) {
				for (auto const& cc : ccc) {
					for (auto const& column : cc) {
						active_neuron_event_outputs.insert(column.toNeuronEventOutputOnDLS());
					}
				}
			}
		}
	}
	LOG4CXX_TRACE(
	    logger,
	    "Extracted unique vertex properties in " << timer_extract_vertex_properties.print() << ".");

	hate::Timer timer_check_non_overlapping_vertices;
	// check that no vertices describe overlapping locations on the hardware
	{
		std::set<vertex::CrossbarNode::Coordinate> unique;
		for (auto const& crossbar_node : crossbar_nodes) {
			unique.insert(crossbar_node.get().get_coordinate());
		}
		if (crossbar_nodes.size() != unique.size()) {
			throw InvalidNetworkGraph("CrossbarNode vertices overlap on hardware.");
		}
	}
	{
		std::set<vertex::SynapseDriver::Coordinate> unique;
		for (auto const& synapse_driver : synapse_drivers) {
			unique.insert(synapse_driver.get().get_coordinate());
		}
		if (synapse_drivers.size() != unique.size()) {
			throw InvalidNetworkGraph("SynapseDriver vertices overlap on hardware.");
		}
	}
	{
		std::set<vertex::BackgroundSpikeSource::Coordinate> unique;
		for (auto const& background_spike_source : background_spike_sources) {
			unique.insert(background_spike_source.get().get_coordinate());
		}
		if (background_spike_sources.size() != unique.size()) {
			throw InvalidNetworkGraph("BackgroundSpikeSource vertices overlap on hardware.");
		}
	}
	{
		std::set<std::pair<SynapseRowOnDLS, SynapseOnSynapseRow>> unique;
		size_t num_synapses = 0;
		for (auto const& synapse_array_view : synapse_array_views) {
			for (auto const& synapse : synapse_array_view.get().get_synapses()) {
				unique.insert(
				    std::pair{SynapseRowOnDLS(
				                  synapse_array_view.get().get_rows()[synapse.index_row],
				                  synapse_array_view.get().get_synram()),
				              synapse_array_view.get().get_columns()[synapse.index_column]});
				num_synapses++;
			}
		}
		if (num_synapses != unique.size()) {
			throw InvalidNetworkGraph("Synapse vertices overlap on hardware.");
		}
	}
	LOG4CXX_TRACE(
	    logger, "Checked for non-overlapping vertices in "
	                << timer_check_non_overlapping_vertices.print() << ".");

	hate::Timer timer_split_population_descriptors;
	std::vector<PopulationDescriptor> external_population_descriptors;
	std::vector<PopulationDescriptor> internal_population_descriptors;
	std::vector<PopulationDescriptor> background_population_descriptors;
	for (auto const& [descriptor, population] : network.populations) {
		if (std::holds_alternative<ExternalPopulation>(population)) {
			external_population_descriptors.push_back(descriptor);
		} else if (std::holds_alternative<Population>(population)) {
			internal_population_descriptors.push_back(descriptor);
		} else if (std::holds_alternative<BackgroundSpikeSourcePopulation>(population)) {
			background_population_descriptors.push_back(descriptor);
		} else {
			throw std::logic_error("Population type not supported.");
		}
	}
	LOG4CXX_TRACE(
	    logger, "Splitted population descriptors by population type in "
	                << timer_split_population_descriptors.print() << ".");

	// get connectum of a single path given an event label
	auto const process_path = [&connectum](
	                              HardwareConnectionPath const& path,
	                              std::pair<PopulationDescriptor, size_t> const& descriptor,
	                              haldls::vx::v3::SpikeLabel const& label) {
		auto const neuron_label = label.get_neuron_label();
		auto const target = path.crossbar_node.get_config().get_target();
		auto const mask = path.crossbar_node.get_config().get_mask();
		if ((neuron_label & mask) != target) { // crossbar node drops event
			return;
		}
		auto const syndrv_mask = path.synapse_driver.get_config().row_address_compare_mask;
		if ((path.synapse_driver.get_coordinate()
		         .toSynapseDriverOnSynapseDriverBlock()
		         .toSynapseDriverOnPADIBus() &
		     syndrv_mask) !=
		    (label.get_row_select_address() & syndrv_mask)) { // synapse driver drops event
			return;
		}
		auto const synapse_label = label.get_synapse_label();
		auto const synram = path.synapse_array_view.get_synram();
		for (auto const& i_syn : path.matching_synapses) {
			auto const& synapse = path.synapse_array_view.get_synapses()[i_syn];
			if (synapse.label != synapse_label) {
				continue;
			}
			auto const& row = path.synapse_array_view.get_rows()[synapse.index_row];
			auto const& column = path.synapse_array_view.get_columns()[synapse.index_column];
			AtomicNeuronOnDLS neuron(column.toNeuronColumnOnDLS(), synram.toNeuronRowOnDLS());
			auto const row_on_synapse_driver = row.toSynapseRowOnSynapseDriver();
			switch (path.synapse_driver.get_config().row_modes[row_on_synapse_driver]) {
				case SynapseDriverConfig::RowMode::excitatory: {
					connectum.push_back(ConnectumConnection{
					    descriptor, neuron, Projection::ReceptorType::excitatory, synapse.weight});
					break;
				}
				case SynapseDriverConfig::RowMode::inhibitory: {
					connectum.push_back(ConnectumConnection{
					    descriptor, neuron, Projection::ReceptorType::inhibitory, synapse.weight});
					break;
				}
				case SynapseDriverConfig::RowMode::excitatory_and_inhibitory: {
					throw std::runtime_error("Synapse in hardware network features "
					                         "excitatory_and_inhibitory row mode.");
				}
				case SynapseDriverConfig::RowMode::disabled: {
					std::stringstream ss;
					ss << "Synapse in hardware network features disabled row mode at "
					   << SynapseRowOnDLS(row, synram) << path.synapse_driver << ".";
					throw std::runtime_error(ss.str());
				}
				default: {
					throw std::logic_error("Synapse driver row mode not supported.");
				}
			}
		}
	};

	hate::Timer timer_extract_hardware_connection_paths;
	std::vector<HardwareConnectionPath> hardware_connection_paths;
	for (auto const crossbar_node : crossbar_nodes) {
		if (!crossbar_node.get()
		         .get_coordinate()
		         .toCrossbarOutputOnDLS()
		         .toPADIBusOnDLS()) { // not forwarding to PADI-bus
			continue;
		}
		for (auto const synapse_driver : synapse_drivers) {
			if (*(crossbar_node.get().get_coordinate().toCrossbarOutputOnDLS().toPADIBusOnDLS()) !=
			    PADIBusOnDLS(
			        synapse_driver.get()
			            .get_coordinate()
			            .toSynapseDriverOnSynapseDriverBlock()
			            .toPADIBusOnPADIBusBlock(),
			        synapse_driver.get()
			            .get_coordinate()
			            .toSynapseDriverBlockOnDLS()
			            .toPADIBusBlockOnDLS())) { // PADI-bus of synapse driver and crossbar node
				                                   // don't match
				continue;
			}
			auto const synapse_rows = synapse_driver.get().get_coordinate().toSynapseRowOnSynram();
			for (auto const& synapse_array_view : synapse_array_views) {
				if (synapse_driver.get()
				        .get_coordinate()
				        .toSynapseDriverBlockOnDLS()
				        .toSynramOnDLS() !=
				    synapse_array_view.get().get_synram()) { // hemisphere of synapse driver and
					                                         // synapse array view does not match
					continue;
				}
				if (std::none_of(
				        synapse_rows.begin(), synapse_rows.end(),
				        [synapse_array_view](auto const& row) {
					        auto const it = std::find(
					            synapse_array_view.get().get_rows().begin(),
					            synapse_array_view.get().get_rows().end(), row);
					        return it != synapse_array_view.get().get_rows().end();
				        })) {
					continue;
				}
				for (auto const row_on_synapse_driver : iter_all<SynapseRowOnSynapseDriver>()) {
					auto const& row = synapse_rows[row_on_synapse_driver];
					std::vector<size_t> matching_synapses;
					size_t i_syn = 0;
					for (auto const& synapse : synapse_array_view.get().get_synapses()) {
						if (synapse_array_view.get().get_rows()[synapse.index_row] == row) {
							matching_synapses.push_back(i_syn);
						}
						i_syn++;
					}
					if (!matching_synapses.empty()) {
						hardware_connection_paths.push_back(HardwareConnectionPath{
						    crossbar_node.get(), synapse_driver.get(), synapse_array_view.get(),
						    std::move(matching_synapses)});
					}
				}
			}
		}
	}
	LOG4CXX_TRACE(
	    logger, "Extracted hardware connection paths in "
	                << timer_extract_hardware_connection_paths.print() << ".");

	hate::Timer timer_build_connectum;
	// assumes event labels match between abstract network and hardware network
	for (auto const& hardware_connection_path : hardware_connection_paths) {
		auto const spl1_address = hardware_connection_path.crossbar_node.get_coordinate()
		                              .toCrossbarInputOnDLS()
		                              .toSPL1Address();
		auto const neuron_event_output = hardware_connection_path.crossbar_node.get_coordinate()
		                                     .toCrossbarInputOnDLS()
		                                     .toNeuronEventOutputOnDLS();
		if (spl1_address) { // crossbar node from L2
			for (auto const& external_population_descriptor : external_population_descriptors) {
				auto const& spike_labels =
				    network_graph.get_spike_labels().at(external_population_descriptor);
				for (size_t index_pre = 0; index_pre < spike_labels.size(); ++index_pre) {
					for (auto const& spike_label : spike_labels.at(index_pre)) {
						assert(spike_label);
						if (*spl1_address !=
						    spike_label->get_spl1_address()) { // wrong spl1 channel
							continue;
						}
						process_path(
						    hardware_connection_path,
						    std::pair{external_population_descriptor, index_pre}, *spike_label);
					}
				}
			}
		} else if (neuron_event_output) { // crossbar node from on-chip neuron
			if (!active_neuron_event_outputs.contains(*neuron_event_output)) {
				continue;
			}
			for (auto const& internal_population_descriptor : internal_population_descriptors) {
				auto const& spike_labels =
				    network_graph.get_spike_labels().at(internal_population_descriptor);
				for (size_t index_pre = 0; index_pre < spike_labels.size(); ++index_pre) {
					if (!spike_labels.at(index_pre).at(0)) { // disabled output
						continue;
					}
					if (*neuron_event_output !=
					    spike_labels.at(index_pre)
					        .at(0)
					        ->get_neuron_event_output()) { // wrong neuron event output
						continue;
					}
					process_path(
					    hardware_connection_path,
					    std::pair{internal_population_descriptor, index_pre},
					    *(spike_labels.at(index_pre).at(0)));
				}
			}
		} else { // crossbar node from background spike source
			for (auto const& background_spike_source : background_spike_sources) {
				if (background_spike_source.get().get_coordinate().toCrossbarInputOnDLS() !=
				    hardware_connection_path.crossbar_node.get_coordinate()
				        .toCrossbarInputOnDLS()) { // background source and crossbar node
					                               // don't match
					continue;
				}
				for (auto const& background_population_descriptor :
				     background_population_descriptors) {
					auto const& spike_labels =
					    network_graph.get_spike_labels().at(background_population_descriptor);
					size_t i = 0;
					for (auto const [hemisphere, padi_bus] :
					     std::get<BackgroundSpikeSourcePopulation>(
					         network.populations.at(background_population_descriptor))
					         .coordinate) {
						if (background_spike_source.get().get_coordinate().toPADIBusOnDLS() ==
						    PADIBusOnDLS(padi_bus, hemisphere.toPADIBusBlockOnDLS())) {
							for (size_t index_pre = 0; index_pre < spike_labels.size();
							     ++index_pre) {
								assert(spike_labels.at(index_pre).at(i));
								process_path(
								    hardware_connection_path,
								    std::pair{background_population_descriptor, index_pre},
								    *(spike_labels.at(index_pre).at(i)));
							}
						}
						i++;
					}
				}
			}
		}
	}
	LOG4CXX_TRACE(
	    logger, "Built connectum from hardware connection paths in "
	                << timer_build_connectum.print() << ".");

	LOG4CXX_DEBUG(
	    logger,
	    "Generated connectum from hardware network in total duration of " << timer.print() << ".");

	return connectum;
}

} // namespace grenade::vx::network
