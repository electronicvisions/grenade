#include "grenade/vx/network/network_graph_builder.h"

#include "halco/hicann-dls/vx/v2/event.h"
#include "halco/hicann-dls/vx/v2/padi.h"
#include "halco/hicann-dls/vx/v2/routing_crossbar.h"
#include "halco/hicann-dls/vx/v2/synapse.h"
#include "halco/hicann-dls/vx/v2/synapse_driver.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <map>
#include <set>
#include <unordered_set>
#include <log4cxx/logger.h>

namespace grenade::vx::network {

using namespace halco::hicann_dls::vx::v2;
using namespace halco::common;

NetworkGraph build_network_graph(
    std::shared_ptr<Network> const& network, RoutingResult const& routing_result)
{
	hate::Timer timer;
	assert(network);
	static log4cxx::Logger* logger = log4cxx::Logger::getLogger("grenade.build_network_graph");

	Graph graph;
	coordinate::ExecutionInstance const instance; // Only one instance used

	NetworkGraphBuilder::Resources resources;

	NetworkGraphBuilder builder(*network);

	// add external populations input
	builder.add_external_input(graph, resources, instance);

	// add on-chip populations without input
	builder.add_populations(graph, resources, routing_result, instance);

	// add MADC recording
	if (network->madc_recording) {
		builder.add_madc_recording(graph, resources, *(network->madc_recording), instance);
	}

	// add neuron event outputs
	builder.add_neuron_event_outputs(graph, resources, instance);

	// add external output
	builder.add_external_output(graph, resources, routing_result, instance);

	// add projections iteratively
	// keep track of unplaced projections
	// each iteration at least a single projection can be placed and therefore size(projections)
	// iterations are needed at most
	std::set<ProjectionDescriptor> unplaced_projections;
	for (auto const& [descriptor, _] : network->projections) {
		unplaced_projections.insert(descriptor);
	}
	for (size_t iteration = 0; iteration < network->projections.size(); ++iteration) {
		std::set<ProjectionDescriptor> newly_placed_projections;
		for (auto const descriptor : unplaced_projections) {
			auto const descriptor_pre = network->projections.at(descriptor).population_pre;
			// skip projection if presynaptic population is not yet present
			if (!resources.populations.contains(descriptor_pre) &&
			    !std::holds_alternative<ExternalPopulation>(
			        network->populations.at(descriptor_pre))) {
				continue;
			}
			std::visit(
			    hate::overloaded(
			        [&](ExternalPopulation const&) {
				        builder.add_projection_from_external_input(
				            graph, resources, descriptor, routing_result, instance);
			        },
			        [&](Population const&) {
				        builder.add_projection_from_internal_input(
				            graph, resources, descriptor, routing_result, instance);
			        }),
			    network->populations.at(descriptor_pre));
			newly_placed_projections.insert(descriptor);
		}
		for (auto const descriptor : newly_placed_projections) {
			unplaced_projections.erase(descriptor);
		}
	}

	auto const spike_labels = builder.get_spike_labels(routing_result);

	LOG4CXX_TRACE(
	    logger, "Built hardware graph representation of network in " << timer.print() << ".");

	return {network,
	        std::move(graph),
	        resources.external_input,
	        resources.external_output,
	        resources.madc_output,
	        spike_labels};
}


NetworkGraphBuilder::NetworkGraphBuilder(Network const& network) :
    m_network(network), m_logger(log4cxx::Logger::getLogger("grenade.NetworkGraphBuilder"))
{}

std::vector<Input> NetworkGraphBuilder::get_inputs(
    Graph const& graph, Graph::vertex_descriptor const descriptor)
{
	auto const& g = graph.get_graph();
	auto const edges = boost::make_iterator_range(boost::in_edges(descriptor, g));
	std::vector<Input> inputs;
	for (auto const& edge : edges) {
		auto const source = boost::source(edge, g);
		auto const port_restriction = graph.get_edge_property_map().at(edge);
		auto const input = port_restriction ? Input(source, *port_restriction) : Input(source);
		inputs.push_back(input);
	}
	return inputs;
}

void NetworkGraphBuilder::add_external_input(
    Graph& graph, Resources& resources, coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// only continue if any external population exists
	if (std::none_of(m_network.populations.begin(), m_network.populations.end(), [](auto const& p) {
		    return std::holds_alternative<ExternalPopulation>(p.second);
	    })) {
		return;
	}

	// add external input for spikes
	vertex::ExternalInput external_input(ConnectionType::DataTimedSpikeSequence, 1);
	auto const event_input_vertex = graph.add(external_input, instance, {});

	// add local execution instance data input for spikes
	vertex::DataInput data_input(ConnectionType::TimedSpikeSequence, 1);
	auto const data_input_vertex = graph.add(data_input, instance, {event_input_vertex});

	// add crossbar l2 input from spike data
	vertex::CrossbarL2Input crossbar_l2_input;
	auto const crossbar_l2_input_vertex =
	    graph.add(crossbar_l2_input, instance, {data_input_vertex});
	resources.external_input = event_input_vertex;
	resources.crossbar_l2_input = crossbar_l2_input_vertex;
	LOG4CXX_TRACE(
	    m_logger, "add_external_input(): Added external input in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_population(
    Graph& graph,
    Resources& resources,
    std::map<HemisphereOnDLS, Input> const& input,
    PopulationDescriptor const& descriptor,
    RoutingResult const& connection_result,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	if (resources.populations.contains(
	        descriptor)) { // population already added, readd by data-reference with new input
		for (auto& [hemisphere, neuron_view_vertex] :
		     resources.populations.at(descriptor).neurons) {
			// use all former inputs
			auto inputs = get_inputs(graph, neuron_view_vertex);
			// append new input
			inputs.push_back(input.at(hemisphere));
			// add by using present data from former vertex with new inputs
			neuron_view_vertex = graph.add(neuron_view_vertex, instance, inputs);
		}
	} else { // population not added before
		auto const& population = std::get<Population>(m_network.populations.at(descriptor));
		// split neuron locations into hemispheres
		std::map<NeuronRowOnDLS, vertex::NeuronView::Columns> neurons;
		for (auto const n : population.neurons) {
			neurons[n.toNeuronRowOnDLS()].push_back(n.toNeuronColumnOnDLS());
		}
		// add a neuron view per hemisphere
		for (auto&& [row, nrn] : neurons) {
			vertex::NeuronView::Configs configs;
			if (!connection_result.internal_neuron_labels.contains(descriptor)) {
				std::stringstream ss;
				ss << "Connection builder result does not contain the neuron labels for "
				   << descriptor << ".";
				throw std::runtime_error(ss.str());
			}
			for (auto const c : nrn) {
				AtomicNeuronOnDLS const an(c, row);
				auto const it = std::find(population.neurons.begin(), population.neurons.end(), an);
				if (it == population.neurons.end()) {
					std::stringstream ss;
					ss << "Connection builder result the neuron label for " << an << ".";
				}
				auto const index = std::distance(population.neurons.begin(), it);
				vertex::NeuronView::Config config{
				    connection_result.internal_neuron_labels.at(descriptor).at(index),
				    false /* TODO: expose reset */};
				configs.push_back(config);
			}
			vertex::NeuronView neuron_view(std::move(nrn), std::move(configs), row);
			auto const hemisphere = row.toHemisphereOnDLS();
			// use input for specific hemisphere
			std::vector<Input> inputs;
			if (input.contains(hemisphere)) {
				inputs.push_back(input.at(hemisphere));
			}
			// add neuron view to graph and vertex descriptor to resources
			auto const neuron_view_vertex = graph.add(neuron_view, instance, inputs);
			resources.populations[descriptor].neurons[row.toHemisphereOnDLS()] = neuron_view_vertex;
		}
	}
	LOG4CXX_TRACE(
	    m_logger,
	    "add_population(): Added population(" << descriptor << ") in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_padi_bus(
    Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::PADIBusOnDLS const& coordinate,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get incoming crossbar nodes via lookup in the resources
	std::vector<Input> inputs;
	for (auto const& [c, d] : resources.crossbar_nodes) {
		auto const local_padi_bus = c.toCrossbarOutputOnDLS().toPADIBusOnDLS();
		if (local_padi_bus && (*local_padi_bus == coordinate)) {
			inputs.push_back(d);
		}
	}
	if (resources.padi_busses.contains(
	        coordinate)) { // padi bus already added, readd with new inputs
		auto& vertex = resources.padi_busses.at(coordinate);
		if (inputs != get_inputs(graph, vertex)) { // only readd when inputs changed
			vertex = graph.add(vertex, instance, inputs);
		}
	} else { // padi bus not added yet
		resources.padi_busses[coordinate] =
		    graph.add(vertex::PADIBus(coordinate), instance, inputs);
	}
	LOG4CXX_TRACE(
	    m_logger, "add_padi_bus(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_crossbar_node(
    Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::CrossbarNodeOnDLS const& coordinate,
    RoutingResult const& connection_result,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get inputs
	std::vector<Input> inputs;
	if (coordinate.toCrossbarInputOnDLS().toSPL1Address()) { // from crossbar l2 input
		if (!resources.crossbar_l2_input) {
			throw std::logic_error("Trying to add crossbar node from L2 without l2 input vertex.");
		}
		inputs.push_back(*resources.crossbar_l2_input);
	} else if (coordinate.toCrossbarInputOnDLS()
	               .toNeuronEventOutputOnDLS()) { // from neuron event output
		auto const neuron_event_output =
		    *(coordinate.toCrossbarInputOnDLS().toNeuronEventOutputOnDLS());
		inputs.push_back(resources.neuron_event_outputs.at(neuron_event_output));
	} else { // other (currently only background sources)
		throw std::logic_error("Unimplemented.");
	}
	// add crossbar node to graph
	if (resources.crossbar_nodes.contains(
	        coordinate)) { // crossbar node already present, readd with new inputs
		auto& vertex = resources.crossbar_nodes.at(coordinate);
		if (inputs != get_inputs(graph, vertex)) { // only readd when inputs changed
			vertex = graph.add(vertex, instance, inputs);
		}
	} else { // crossbar node not added yet
		if (!connection_result.crossbar_nodes.contains(coordinate)) {
			std::stringstream ss;
			ss << "Trying to add crossbar node at CrossbarNodeOnDLS("
			   << coordinate.toCrossbarInputOnDLS() << ", " << coordinate.toCrossbarOutputOnDLS()
			   << ") for which no configuration from the connection result is available.";
			throw std::logic_error(ss.str());
		}
		auto const& config = connection_result.crossbar_nodes.at(coordinate);
		resources.crossbar_nodes[coordinate] =
		    graph.add(vertex::CrossbarNode(coordinate, config), instance, inputs);
	}
	LOG4CXX_TRACE(
	    m_logger, "add_crossbar_node(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_synapse_driver(
    Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::SynapseDriverOnDLS const& coordinate,
    RoutingResult const& connection_result,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get inputs (incoming padi bus)
	PADIBusOnDLS const padi_bus(
	    coordinate.toSynapseDriverOnSynapseDriverBlock().toPADIBusOnPADIBusBlock(),
	    coordinate.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS());
	std::vector<Input> const inputs = {resources.padi_busses.at(padi_bus)};
	if (resources.synapse_drivers.contains(
	        coordinate)) { // synapse driver already present, readd with new inputs
		auto& vertex = resources.synapse_drivers.at(coordinate);
		if (inputs !=
		    get_inputs(
		        graph, vertex)) { // only readd when inputs changed (FIXME: this never happens)
			vertex = graph.add(vertex, instance, inputs);
		}
	} else { // synapse driver not added yet
		vertex::SynapseDriver::Config::RowModes row_modes;
		for (auto const r : iter_all<SynapseRowOnSynapseDriver>()) {
			auto const synapse_row = SynapseRowOnDLS(
			    coordinate.toSynapseDriverOnSynapseDriverBlock().toSynapseRowOnSynram()[r],
			    coordinate.toSynapseDriverBlockOnDLS().toSynramOnDLS());
			if (!connection_result.synapse_row_modes.contains(synapse_row)) {
				throw std::runtime_error(
				    "Connection builder result does not contain the mode for the synapse row(" +
				    std::to_string(synapse_row) + ").");
			}
			row_modes[r] = connection_result.synapse_row_modes.at(synapse_row);
		}
		vertex::SynapseDriver synapse_driver(
		    coordinate, {connection_result.synapse_driver_compare_masks.at(coordinate), row_modes,
		                 true /* TODO: expose */});
		resources.synapse_drivers[coordinate] = graph.add(synapse_driver, instance, inputs);
	}
	LOG4CXX_TRACE(
	    m_logger, "add_synapse_driver(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_neuron_event_output(
    Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::NeuronEventOutputOnDLS const& coordinate,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get inputs (incoming neuron views)
	std::map<HemisphereOnDLS, std::vector<Input>> hemisphere_inputs;
	vertex::NeuronEventOutputView::Neurons neurons;
	for (auto const& [descriptor, pop] : resources.populations) {
		auto const is_outgoing_projection = [&](auto const& proj) {
			return proj.second.population_pre == descriptor;
		};
		// skip if the population neither has outgoing projections nor its spikes shall be recorded
		bool const has_outgoing_projection =
		    std::find_if(
		        m_network.projections.begin(), m_network.projections.end(),
		        is_outgoing_projection) != m_network.projections.end();
		auto const& population = std::get<Population>(m_network.populations.at(descriptor));
		bool const enable_record_spikes = std::any_of(
		    population.enable_record_spikes.begin(), population.enable_record_spikes.end(),
		    [](auto const c) { return c; });
		if (!has_outgoing_projection && !enable_record_spikes) {
			continue;
		}
		// get neuron locations of population from neuron view
		for (auto const& [hemisphere, nrns] : pop.neurons) {
			auto const& neuron_view = std::get<vertex::NeuronView>(graph.get_vertex_property(nrns));
			auto& ns = neurons[neuron_view.get_row()];
			size_t const size_before = ns.size();
			size_t min = 0;
			bool nothing_added = true;
			for (auto const& neuron : neuron_view.get_columns()) {
				if (neuron.toNeuronEventOutputOnDLS() != coordinate) {
					if (nothing_added) {
						min++;
					}
					continue;
				}
				nothing_added = false;
				ns.resize(hemisphere_inputs[hemisphere].size() + 1);
				ns.back().push_back(neuron);
			}
			// if the number of neurons increased, this population adds input to the hemisphere
			if (ns.size() > size_before) {
				PortRestriction port_restriction(min, min + (ns.back().size() - 1));
				if (port_restriction.size() == neuron_view.get_columns().size()) {
					hemisphere_inputs[hemisphere].push_back(nrns);
				} else {
					hemisphere_inputs[hemisphere].push_back({nrns, port_restriction});
				}
			}
		}
	}
	// get inputs sorted by hemispheres
	std::vector<Input> inputs;
	for (auto const& [_, ins] : hemisphere_inputs) {
		inputs.insert(inputs.end(), ins.begin(), ins.end());
	}
	// only add neuron event output, if there are inputs present
	if (inputs.empty()) {
		return;
	}
	resources.neuron_event_outputs[coordinate] =
	    graph.add(vertex::NeuronEventOutputView(std::move(neurons)), instance, inputs);
	LOG4CXX_TRACE(
	    m_logger,
	    "add_neuron_event_output(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_synapse_array_view_sparse(
    Graph& graph,
    Resources& resources,
    ProjectionDescriptor descriptor,
    RoutingResult const& connection_result,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get columns of post-synaptic population
	std::map<HemisphereOnDLS, vertex::SynapseArrayViewSparse::Columns> columns;
	auto const post_descriptor = m_network.projections.at(descriptor).population_post;
	if (resources.populations.contains(post_descriptor)) { // post population already present
		for (auto const& [coord, d] : resources.populations.at(post_descriptor).neurons) {
			auto const& neuron_view = std::get<vertex::NeuronView>(graph.get_vertex_property(d));
			auto& local_columns = columns[coord];
			for (auto const& c : neuron_view.get_columns()) {
				local_columns.push_back(c.toSynapseOnSynapseRow());
			}
		}
	} else { // post population not present yet
		auto const& population = std::get<Population>(m_network.populations.at(post_descriptor));
		for (auto const n : population.neurons) {
			columns[n.toNeuronRowOnDLS().toHemisphereOnDLS()].push_back(
			    n.toNeuronColumnOnDLS().toSynapseOnSynapseRow());
		}
	}
	// get used synapse drivers and synapse rows
	std::map<HemisphereOnDLS, std::set<SynapseDriverOnDLS>> used_synapse_drivers;
	std::map<HemisphereOnDLS, std::set<SynapseRowOnSynram>> used_synapse_rows;
	if (!connection_result.connections.contains(descriptor)) {
		throw std::runtime_error(
		    "Connection builder result does not contain connections for the projection(" +
		    std::to_string(descriptor) + ").");
	}
	for (auto const& placed_connections : connection_result.connections.at(descriptor)) {
		for (auto const& placed_connection : placed_connections) {
			auto const hemisphere =
			    placed_connection.synapse_row.toSynramOnDLS().toHemisphereOnDLS();
			used_synapse_drivers[hemisphere].insert(SynapseDriverOnDLS(
			    placed_connection.synapse_row.toSynapseRowOnSynram()
			        .toSynapseDriverOnSynapseDriverBlock(),
			    placed_connection.synapse_row.toSynramOnDLS().toSynapseDriverBlockOnDLS()));
			used_synapse_rows[hemisphere].insert(placed_connection.synapse_row);
		}
	}
	// get rows
	std::map<HemisphereOnDLS, vertex::SynapseArrayViewSparse::Rows> rows;
	for (auto const& [hemisphere, used_rows] : used_synapse_rows) {
		rows[hemisphere].assign(used_rows.begin(), used_rows.end());
	}
	// get reverse lookup of synapse indices
	std::map<HemisphereOnDLS, std::map<SynapseOnSynapseRow, size_t>> lookup_columns;
	for (auto const& [hemisphere, c] : columns) {
		auto& local_lookup = lookup_columns[hemisphere];
		size_t i = 0;
		for (auto const& cc : c) {
			local_lookup[cc] = i;
			i++;
		}
	}
	std::map<HemisphereOnDLS, std::map<SynapseRowOnSynram, size_t>> lookup_rows;
	for (auto const& [hemisphere, r] : rows) {
		auto& local_lookup = lookup_rows[hemisphere];
		size_t i = 0;
		for (auto const& rr : r) {
			local_lookup[rr] = i;
			i++;
		}
	}
	// get synapses
	std::map<HemisphereOnDLS, vertex::SynapseArrayViewSparse::Synapses> synapses;
	for (auto const& placed_connections : connection_result.connections.at(descriptor)) {
		for (auto const& placed_connection : placed_connections) {
			auto const hemisphere =
			    placed_connection.synapse_row.toSynramOnDLS().toHemisphereOnDLS();
			vertex::SynapseArrayViewSparse::Synapse synapse{
			    placed_connection.label, placed_connection.weight,
			    lookup_rows.at(hemisphere).at(placed_connection.synapse_row.toSynapseRowOnSynram()),
			    lookup_columns.at(hemisphere).at(placed_connection.synapse_on_row)};
			synapses[hemisphere].push_back(synapse);
		}
	}
	// get inputs
	std::map<HemisphereOnDLS, std::vector<Input>> inputs;
	for (auto const& [hemisphere, synapse_drivers] : used_synapse_drivers) {
		auto& local_inputs = inputs[hemisphere];
		for (auto const& synapse_driver : synapse_drivers) {
			local_inputs.push_back(resources.synapse_drivers.at(synapse_driver));
		}
	}
	// add vertices
	for (auto&& [hemisphere, cs] : columns) {
		auto&& rs = rows.at(hemisphere);
		auto&& syns = synapses.at(hemisphere);
		auto const synram = hemisphere.toSynramOnDLS();
		vertex::SynapseArrayViewSparse config(
		    synram, std::move(rs), std::move(cs), std::move(syns));

		assert(!resources.projections.contains(descriptor));
		resources.projections[descriptor].synapses[hemisphere] =
		    graph.add(config, instance, inputs.at(hemisphere));
	}
	LOG4CXX_TRACE(
	    m_logger, "add_synapse_array_view(): Added synapse array view for projection("
	                  << descriptor << ") in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_projection_from_external_input(
    Graph& graph,
    Resources& resources,
    ProjectionDescriptor const& descriptor,
    RoutingResult const& connection_result,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	auto const population_pre = m_network.projections.at(descriptor).population_pre;
	if (!std::holds_alternative<ExternalPopulation>(m_network.populations.at(population_pre))) {
		throw std::logic_error("Projection's presynaptic population is not external.");
	}
	// get used PADI busses, synapse drivers and spl1 addresses
	std::set<PADIBusOnDLS> used_padi_bus;
	std::set<SynapseDriverOnDLS> used_synapse_drivers;
	std::map<PADIBusOnDLS, std::set<SPL1Address>> used_spl1_addresses;
	if (!connection_result.external_spike_labels.contains(population_pre)) {
		throw std::runtime_error(
		    "Connection builder result does not contain spike labels for the population(" +
		    std::to_string(population_pre) + ") from external input.");
	}
	auto const& external_spike_labels = connection_result.external_spike_labels.at(population_pre);
	if (!connection_result.connections.contains(descriptor)) {
		throw std::runtime_error(
		    "Connection builder result does not contain connections the projection(" +
		    std::to_string(descriptor) + ").");
	}
	auto const num_connections_result = connection_result.connections.at(descriptor).size();
	auto const num_connections_projection = m_network.projections.at(descriptor).connections.size();
	if (num_connections_result != num_connections_projection) {
		throw std::runtime_error(
		    "Connection builder result connection number(" +
		    std::to_string(num_connections_result) +
		    ") is larger than expected number of connections by projection (" +
		    std::to_string(num_connections_projection) + ").");
	}
	size_t i = 0;
	for (auto const& placed_connections : connection_result.connections.at(descriptor)) {
		for (auto const& placed_connection : placed_connections) {
			auto const padi_bus_block =
			    placed_connection.synapse_row.toSynramOnDLS().toPADIBusBlockOnDLS();
			auto const padi_bus_on_block = placed_connection.synapse_row.toSynapseRowOnSynram()
			                                   .toSynapseDriverOnSynapseDriverBlock()
			                                   .toPADIBusOnPADIBusBlock();
			PADIBusOnDLS padi_bus(padi_bus_on_block, padi_bus_block);
			used_padi_bus.insert(padi_bus);
			used_synapse_drivers.insert(SynapseDriverOnDLS(
			    placed_connection.synapse_row.toSynapseRowOnSynram()
			        .toSynapseDriverOnSynapseDriverBlock(),
			    placed_connection.synapse_row.toSynramOnDLS().toSynapseDriverBlockOnDLS()));
			auto const index_pre = m_network.projections.at(descriptor).connections.at(i).index_pre;
			for (auto label : external_spike_labels.at(index_pre)) {
				used_spl1_addresses[padi_bus].insert(label.get_spl1_address());
			}
		}
		i++;
	}
	// add crossbar nodes from L2 input to PADI busses
	for (auto const& padi_bus : used_padi_bus) {
		for (auto const spl1_address : used_spl1_addresses.at(padi_bus)) {
			CrossbarNodeOnDLS coord(
			    padi_bus.toCrossbarOutputOnDLS(), spl1_address.toCrossbarInputOnDLS());
			add_crossbar_node(graph, resources, coord, connection_result, instance);
		}
	}
	// add PADI busses
	for (auto const padi_bus : used_padi_bus) {
		add_padi_bus(graph, resources, padi_bus, instance);
	}
	// add synapse drivers
	for (auto const synapse_driver : used_synapse_drivers) {
		add_synapse_driver(graph, resources, synapse_driver, connection_result, instance);
	}
	// add synapse array views
	add_synapse_array_view_sparse(graph, resources, descriptor, connection_result, instance);
	// add population
	std::map<HemisphereOnDLS, Input> inputs(
	    resources.projections.at(descriptor).synapses.begin(),
	    resources.projections.at(descriptor).synapses.end());
	add_population(
	    graph, resources, inputs, m_network.projections.at(descriptor).population_post,
	    connection_result, instance);
	LOG4CXX_TRACE(
	    m_logger, "add_projection_from_external(): Added projection(" << descriptor << ") in "
	                                                                  << timer.print() << ".");
}

void NetworkGraphBuilder::add_projection_from_internal_input(
    Graph& graph,
    Resources& resources,
    ProjectionDescriptor const& descriptor,
    RoutingResult const& connection_result,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	if (!std::holds_alternative<Population>(
	        m_network.populations.at(m_network.projections.at(descriptor).population_pre))) {
		throw std::logic_error("Projection's presynaptic population is not internal.");
	}
	if (!connection_result.connections.contains(descriptor)) {
		throw std::runtime_error(
		    "Connection builder result does not contain connections the projection(" +
		    std::to_string(descriptor) + ").");
	}
	// get possibly used neuron event outputs
	auto const& population = std::get<Population>(
	    m_network.populations.at(m_network.projections.at(descriptor).population_pre));
	std::set<NeuronEventOutputOnDLS> used_neuron_event_outputs;
	for (auto const& neuron : population.neurons) {
		used_neuron_event_outputs.insert(neuron.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
	}
	// get used PADI busses and synapse drivers
	std::set<PADIBusOnDLS> used_padi_bus;
	std::set<SynapseDriverOnDLS> used_synapse_drivers;
	for (auto const& placed_connections : connection_result.connections.at(descriptor)) {
		for (auto const& placed_connection : placed_connections) {
			auto const padi_bus_block =
			    placed_connection.synapse_row.toSynramOnDLS().toPADIBusBlockOnDLS();
			auto const padi_bus_on_block = placed_connection.synapse_row.toSynapseRowOnSynram()
			                                   .toSynapseDriverOnSynapseDriverBlock()
			                                   .toPADIBusOnPADIBusBlock();
			used_padi_bus.insert(PADIBusOnDLS(padi_bus_on_block, padi_bus_block));
			used_synapse_drivers.insert(SynapseDriverOnDLS(
			    placed_connection.synapse_row.toSynapseRowOnSynram()
			        .toSynapseDriverOnSynapseDriverBlock(),
			    placed_connection.synapse_row.toSynramOnDLS().toSynapseDriverBlockOnDLS()));
		}
	}
	// add crossbar nodes from neuron event outputs to PADI busses
	for (auto const& padi_bus : used_padi_bus) {
		for (auto const neuron_event_output : used_neuron_event_outputs) {
			static_assert(
			    PADIBusOnPADIBusBlock::size == NeuronEventOutputOnNeuronBackendBlock::size);
			if ((padi_bus.toEnum() % PADIBusOnPADIBusBlock::size) !=
			    (neuron_event_output.toEnum() % NeuronEventOutputOnNeuronBackendBlock::size)) {
				continue;
			}
			CrossbarNodeOnDLS coord(
			    padi_bus.toCrossbarOutputOnDLS(), neuron_event_output.toCrossbarInputOnDLS());
			add_crossbar_node(graph, resources, coord, connection_result, instance);
		}
	}
	// add PADI busses
	for (auto const padi_bus : used_padi_bus) {
		add_padi_bus(graph, resources, padi_bus, instance);
	}
	// add synapse drivers
	for (auto const synapse_driver : used_synapse_drivers) {
		add_synapse_driver(graph, resources, synapse_driver, connection_result, instance);
	}
	// add synapse array views
	add_synapse_array_view_sparse(graph, resources, descriptor, connection_result, instance);
	// add population
	std::map<HemisphereOnDLS, Input> inputs(
	    resources.projections.at(descriptor).synapses.begin(),
	    resources.projections.at(descriptor).synapses.end());
	add_population(
	    graph, resources, inputs, m_network.projections.at(descriptor).population_post,
	    connection_result, instance);
	LOG4CXX_TRACE(
	    m_logger, "add_projection_from_internal(): Added projection(" << descriptor << ") in "
	                                                                  << timer.print() << ".");
}

void NetworkGraphBuilder::add_populations(
    Graph& graph,
    Resources& resources,
    RoutingResult const& connection_result,
    coordinate::ExecutionInstance const& instance) const
{
	// place all populations without input
	for (auto const& [descriptor, population] : m_network.populations) {
		if (std::holds_alternative<ExternalPopulation>(population)) {
			continue;
		}
		add_population(graph, resources, {}, descriptor, connection_result, instance);
	}
}

void NetworkGraphBuilder::add_neuron_event_outputs(
    Graph& graph, Resources& resources, coordinate::ExecutionInstance const& instance) const
{
	for (auto const coord : iter_all<NeuronEventOutputOnDLS>()) {
		add_neuron_event_output(graph, resources, coord, instance);
	}
}

void NetworkGraphBuilder::add_external_output(
    Graph& graph,
    Resources& resources,
    RoutingResult const& connection_result,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get set of neuron event outputs from to be recorded populations
	std::set<NeuronEventOutputOnDLS> neuron_event_outputs;
	for (auto const& [descriptor, pop] : m_network.populations) {
		if (!std::holds_alternative<Population>(pop)) {
			continue;
		}
		auto const& population = std::get<Population>(pop);
		if (std::any_of(
		        population.enable_record_spikes.begin(), population.enable_record_spikes.end(),
		        [](auto const c) { return c; })) {
			for (auto const& neuron : population.neurons) {
				neuron_event_outputs.insert(
				    neuron.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
			}
		}
	}
	if (neuron_event_outputs.empty()) {
		return;
	}
	// add crossbar nodes from neuron event outputs
	std::vector<Input> crossbar_l2_output_inputs;
	for (auto const neuron_event_output : neuron_event_outputs) {
		CrossbarL2OutputOnDLS crossbar_l2_output(
		    neuron_event_output.toNeuronEventOutputOnNeuronBackendBlock());
		CrossbarNodeOnDLS coord(
		    crossbar_l2_output.toCrossbarOutputOnDLS(), neuron_event_output.toCrossbarInputOnDLS());
		add_crossbar_node(graph, resources, coord, connection_result, instance);
		crossbar_l2_output_inputs.push_back(resources.crossbar_nodes.at(coord));
	}
	// add crossbar l2 output
	auto const crossbar_l2_output =
	    graph.add(vertex::CrossbarL2Output(), instance, crossbar_l2_output_inputs);
	resources.external_output = graph.add(
	    vertex::DataOutput(ConnectionType::TimedSpikeFromChipSequence, 1), instance,
	    {crossbar_l2_output});
	LOG4CXX_TRACE(
	    m_logger, "add_external_output(): Added external output in " << timer.print() << ".");
}

NetworkGraph::SpikeLabels NetworkGraphBuilder::get_spike_labels(
    RoutingResult const& connection_result)
{
	hate::Timer timer;
	NetworkGraph::SpikeLabels spike_labels = connection_result.external_spike_labels;
	for (auto const& [descriptor, pop] : m_network.populations) {
		if (std::holds_alternative<ExternalPopulation>(pop) && !spike_labels.contains(descriptor)) {
			throw std::runtime_error(
			    "Connection builder result does not contain spike labels for the population(" +
			    std::to_string(descriptor) + ") from external input.");
		}
		if (!std::holds_alternative<Population>(pop)) {
			continue;
		}
		auto const& population = std::get<Population>(pop);
		if (!std::any_of(
		        population.enable_record_spikes.begin(), population.enable_record_spikes.end(),
		        [](auto const c) { return c; })) {
			continue;
		}
		auto& local_spike_labels = spike_labels[descriptor];
		if (!connection_result.internal_neuron_labels.contains(descriptor)) {
			throw std::runtime_error(
			    "Connection builder result does not contain neuron labels for the population(" +
			    std::to_string(descriptor) + ").");
		}
		auto const& local_neuron_labels = connection_result.internal_neuron_labels.at(descriptor);
		for (size_t i = 0; i < population.neurons.size(); ++i) {
			haldls::vx::v2::SpikeLabel spike_label;
			spike_label.set_neuron_event_output(
			    population.neurons.at(i).toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
			spike_label.set_spl1_address(SPL1Address(
			    population.neurons.at(i).toNeuronColumnOnDLS().toNeuronEventOutputOnDLS() %
			    SPL1Address::size));
			spike_label.set_neuron_backend_address_out(local_neuron_labels.at(i));
			local_spike_labels.push_back({spike_label});
		}
	}
	LOG4CXX_TRACE(
	    m_logger, "get_spike_labels(): Calculated spike labels in " << timer.print() << ".");
	return spike_labels;
}

void NetworkGraphBuilder::add_madc_recording(
    Graph& graph,
    Resources& resources,
    MADCRecording const& madc_recording,
    coordinate::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	auto const& population =
	    std::get<Population>(m_network.populations.at(madc_recording.population));
	auto const neuron = population.neurons.at(madc_recording.index);
	vertex::MADCReadoutView madc_readout(neuron, madc_recording.source);
	auto const neuron_vertex_descriptor =
	    resources.populations.at(madc_recording.population)
	        .neurons.at(neuron.toNeuronRowOnDLS().toHemisphereOnDLS());
	auto const neuron_vertex =
	    std::get<vertex::NeuronView>(graph.get_vertex_property(neuron_vertex_descriptor));
	auto const& columns = neuron_vertex.get_columns();
	auto const in_view_location = static_cast<size_t>(std::distance(
	    columns.begin(), std::find(columns.begin(), columns.end(), neuron.toNeuronColumnOnDLS())));
	assert(in_view_location < columns.size());
	auto const madc_vertex = graph.add(
	    madc_readout, instance, {{neuron_vertex_descriptor, {in_view_location, in_view_location}}});
	resources.madc_output = graph.add(
	    vertex::DataOutput(ConnectionType::TimedMADCSampleFromChipSequence, 1), instance,
	    {madc_vertex});
	LOG4CXX_TRACE(
	    m_logger, "add_madc_recording(): Added MADC recording in " << timer.print() << ".");
}

} // namespace grenade::vx::network
