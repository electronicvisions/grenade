#include "grenade/vx/network/placed_logical/network_graph_builder.h"

#include "grenade/vx/network/placed_logical/build_connection_routing.h"
#include "grenade/vx/network/placed_logical/build_connection_weight_split.h"
#include "grenade/vx/network/placed_logical/exception.h"
#include "grenade/vx/network/placed_logical/requires_routing.h"
#include "hate/math.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <map>
#include <log4cxx/logger.h>

namespace grenade::vx::network::placed_logical {

using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;


NetworkGraph build_network_graph(
    std::shared_ptr<Network> const& network,
    RoutingResult const& routing_result,
    signal_flow::ExecutionInstance const& instance)
{
	hate::Timer timer;
	assert(network);
	log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("grenade.build_network_graph");

	NetworkGraph result;
	result.m_routing_duration = routing_result.timing_statistics.routing;
	result.m_network = network;

	NetworkGraphBuilder::Resources resources;

	NetworkGraphBuilder builder(*network);

	// add external populations input
	builder.add_external_input(result.m_graph, resources, instance);

	// add background spike sources
	builder.add_background_spike_sources(result.m_graph, resources, instance, routing_result);

	// add on-chip populations without input
	builder.add_populations(result.m_graph, resources, routing_result, instance);

	// add MADC recording
	if (network->madc_recording) {
		builder.add_madc_recording(result.m_graph, resources, *(network->madc_recording), instance);
	}

	// add CADC recording
	if (network->cadc_recording) {
		builder.add_cadc_recording(result.m_graph, resources, *(network->cadc_recording), instance);
	}

	// add neuron event outputs
	builder.add_neuron_event_outputs(result.m_graph, resources, instance);

	// add external output
	builder.add_external_output(result.m_graph, resources, routing_result, instance);

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
		std::map<PopulationDescriptor, std::map<HemisphereOnDLS, std::vector<signal_flow::Input>>>
		    inputs;
		for (auto const descriptor : unplaced_projections) {
			auto const descriptor_pre = network->projections.at(descriptor).population_pre;
			// skip projection if presynaptic population is not yet present
			if (!resources.populations.contains(descriptor_pre) &&
			    std::holds_alternative<Population>(network->populations.at(descriptor_pre))) {
				continue;
			}
			auto const population_inputs = std::visit(
			    hate::overloaded(
			        [&](ExternalPopulation const&) {
				        return builder.add_projection_from_external_input(
				            result.m_graph, resources, descriptor, routing_result, instance);
			        },
			        [&](BackgroundSpikeSourcePopulation const&) {
				        return builder.add_projection_from_background_spike_source(
				            result.m_graph, resources, descriptor, routing_result, instance);
			        },
			        [&](Population const&) {
				        return builder.add_projection_from_internal_input(
				            result.m_graph, resources, descriptor, routing_result, instance);
			        }),
			    network->populations.at(descriptor_pre));
			newly_placed_projections.insert(descriptor);
			auto& local_population_inputs =
			    inputs[network->projections.at(descriptor).population_post];
			for (auto const& [hemisphere, input] : population_inputs) {
				local_population_inputs[hemisphere].push_back(input);
			}
		}
		// update post-populations
		for (auto const& [population_post, population_inputs] : inputs) {
			builder.add_population(
			    result.m_graph, resources, population_inputs, population_post, routing_result,
			    instance);
		}
		for (auto const descriptor : newly_placed_projections) {
			unplaced_projections.erase(descriptor);
		}
	}
	assert(unplaced_projections.empty());

	// add plasticity rules
	builder.add_plasticity_rules(result.m_graph, resources, instance);

	result.m_spike_labels = builder.get_spike_labels(routing_result);
	result.m_event_input_vertex = resources.external_input;
	result.m_event_output_vertex = resources.external_output;
	result.m_madc_sample_output_vertex = resources.madc_output;
	result.m_cadc_sample_output_vertex = resources.cadc_output;
	for (auto const& [d, p] : resources.projections) {
		result.m_synapse_vertices[d] = p.synapses;
	}
	for (auto const& [d, p] : resources.populations) {
		result.m_neuron_vertices[d] = p.neurons;
	}
	for (auto const& [d, p] : resources.background_spike_sources) {
		result.m_background_spike_source_vertices[d] = p;
	}
	result.m_plasticity_rule_vertices = resources.plasticity_rules;
	result.m_plasticity_rule_output_vertices = resources.plasticity_rules_output;
	result.m_graph_translation = resources.graph_translation;

	LOG4CXX_TRACE(
	    logger, "Built hardware graph representation of network in " << timer.print() << ".");
	result.m_construction_duration = std::chrono::microseconds(timer.get_us());

	hate::Timer verification_timer;
	if (!result.valid()) {
		LOG4CXX_ERROR(logger, "Built network graph is not valid.");
		throw InvalidNetworkGraph("Built network graph is not valid.");
	}
	result.m_verification_duration = std::chrono::microseconds(verification_timer.get_us());

	return result;
}


void update_network_graph(NetworkGraph& network_graph, std::shared_ptr<Network> const& network)
{
	hate::Timer timer;
	log4cxx::LoggerPtr logger =
	    log4cxx::Logger::getLogger("grenade.placed_logical.update_network_graph");

	if (requires_routing(network, network_graph)) {
		throw std::runtime_error(
		    "Network graph can only be updated if no new routing is required.");
	}

	// get whether projection requires weight changes
	auto const requires_change = [](Projection const& projection,
	                                Projection const& old_projection) {
		for (size_t i = 0; i < projection.connections.size(); ++i) {
			auto const& connection = projection.connections.at(i);
			auto const& old_connection = old_projection.connections.at(i);
			if (connection.weight != old_connection.weight) {
				return true;
			}
		}
		return false;
	};

	// update synapse array view in hardware graph corresponding to projection
	auto const update_weights = [](NetworkGraph& network_graph,
	                               std::shared_ptr<Network> const& network,
	                               ProjectionDescriptor const descriptor) {
		std::map<
		    signal_flow::Graph::vertex_descriptor,
		    signal_flow::vertex::SynapseArrayViewSparse::Synapses>
		    synapses;
		auto const synapse_vertices = network_graph.get_synapse_vertices();
		for (auto const [_, vertex_descriptor] : synapse_vertices.at(descriptor)) {
			auto const& old_synapse_array_view =
			    std::get<signal_flow::vertex::SynapseArrayViewSparse>(
			        network_graph.get_graph().get_vertex_property(vertex_descriptor));
			auto const old_synapses = old_synapse_array_view.get_synapses();
			synapses[vertex_descriptor] = signal_flow::vertex::SynapseArrayViewSparse::Synapses{
			    old_synapses.begin(), old_synapses.end()};
		}
		auto const& old_projection = network_graph.get_network()->projections.at(descriptor);
		auto const& projection = network->projections.at(descriptor);
		auto const& translation = network_graph.get_graph_translation().projections;
		for (size_t i = 0; i < old_projection.connections.size(); ++i) {
			auto const& connection = projection.connections.at(i);
			auto const& old_connection = old_projection.connections.at(i);
			if (connection.weight != old_connection.weight) {
				auto const& synapse_indices = translation.at(descriptor).at(i);
				auto const weight_split =
				    build_connection_weight_split(connection.weight, synapse_indices.size());
				for (size_t s = 0; s < synapse_indices.size(); ++s) {
					synapses.at(synapse_indices.at(s).first)
					    .at(synapse_indices.at(s).second)
					    .weight = weight_split.at(s);
				}
			}
		}
		for (auto const [_, vertex_descriptor] : synapse_vertices.at(descriptor)) {
			auto const& old_synapse_array_view =
			    std::get<signal_flow::vertex::SynapseArrayViewSparse>(
			        network_graph.get_graph().get_vertex_property(vertex_descriptor));
			auto const old_rows = old_synapse_array_view.get_rows();
			auto rows =
			    signal_flow::vertex::SynapseArrayViewSparse::Rows{old_rows.begin(), old_rows.end()};
			auto const old_columns = old_synapse_array_view.get_columns();
			auto columns = signal_flow::vertex::SynapseArrayViewSparse::Columns{
			    old_columns.begin(), old_columns.end()};
			signal_flow::vertex::SynapseArrayViewSparse synapse_array_view(
			    old_synapse_array_view.get_synram(), std::move(rows), std::move(columns),
			    std::move(synapses.at(vertex_descriptor)));
			network_graph.m_graph.update(vertex_descriptor, std::move(synapse_array_view));
		}
	};

	// update MADC in graph such that it resembles the configuration from network to update towards
	auto const update_madc = [](NetworkGraph& network_graph, Network const& network) {
		if (!network.madc_recording || !network_graph.m_network->madc_recording) {
			throw std::runtime_error("Updating network graph only possible, if MADC recording is "
			                         "neither added nor removed.");
		}
		auto const& population =
		    std::get<Population>(network.populations.at(network.madc_recording->population));
		auto const neuron = population.neurons.at(network.madc_recording->neuron_on_population)
		                        .coordinate.get_placed_compartments()
		                        .at(network.madc_recording->compartment_on_neuron)
		                        .at(network.madc_recording->atomic_neuron_on_compartment);
		signal_flow::vertex::MADCReadoutView madc_readout(neuron, network.madc_recording->source);
		auto const neuron_vertex_descriptor =
		    network_graph.get_neuron_vertices()
		        .at(network.madc_recording->population)
		        .at(neuron.toNeuronRowOnDLS().toHemisphereOnDLS());
		auto const& neuron_vertex = std::get<signal_flow::vertex::NeuronView>(
		    network_graph.get_graph().get_vertex_property(neuron_vertex_descriptor));
		auto const& columns = neuron_vertex.get_columns();
		auto const in_view_location = static_cast<size_t>(std::distance(
		    columns.begin(),
		    std::find(columns.begin(), columns.end(), neuron.toNeuronColumnOnDLS())));
		assert(in_view_location < columns.size());
		std::vector<signal_flow::Input> inputs{
		    {neuron_vertex_descriptor, {in_view_location, in_view_location}}};
		assert(network_graph.get_madc_sample_output_vertex());
		assert(
		    boost::in_degree(
		        *(network_graph.get_madc_sample_output_vertex()),
		        network_graph.get_graph().get_graph()) == 1);
		auto const edges = boost::in_edges(
		    *(network_graph.get_madc_sample_output_vertex()),
		    network_graph.get_graph().get_graph());
		auto const madc_vertex =
		    boost::source(*(edges.first), network_graph.get_graph().get_graph());
		network_graph.m_graph.update_and_relocate(madc_vertex, std::move(madc_readout), inputs);
	};

	// update plasticity rule in graph such that it resembles the configuration from network to
	// update towards
	auto const update_plasticity_rule = [](NetworkGraph& network_graph,
	                                       PlasticityRule const& new_rule,
	                                       PlasticityRuleDescriptor descriptor) {
		if (!network_graph.m_network->plasticity_rules.contains(descriptor)) {
			throw std::runtime_error("Updating network graph only possible, if plasticity rule is "
			                         "neither added nor removed.");
		}
		if (network_graph.m_network->plasticity_rules.at(descriptor).recording !=
		    new_rule.recording) {
			throw std::runtime_error("Updating network graph only possible, if plasticity rule "
			                         "recording is not changed.");
		}
		// TODO (Issue #3991): merge impl. from here and add_plasticity_rule below, requires rework
		// of resources in order to align interface to NetworkGraph.
		auto const vertex_descriptor = network_graph.get_plasticity_rule_vertices().at(descriptor);
		std::vector<signal_flow::Input> inputs;
		auto const synapse_vertices = network_graph.get_synapse_vertices();
		for (auto const& d : new_rule.projections) {
			for (auto const& [_, p] : synapse_vertices.at(d)) {
				inputs.push_back({p});
			}
		}
		auto const neuron_vertices = network_graph.get_neuron_vertices();
		for (auto const& d : new_rule.populations) {
			for (auto const& [_, p] : neuron_vertices.at(d.descriptor)) {
				inputs.push_back({p});
			}
		}
		auto const& old_rule = std::get<signal_flow::vertex::PlasticityRule>(
		    network_graph.get_graph().get_vertex_property(vertex_descriptor));
		signal_flow::vertex::PlasticityRule vertex(
		    new_rule.kernel,
		    signal_flow::vertex::PlasticityRule::Timer{
		        signal_flow::vertex::PlasticityRule::Timer::Value(new_rule.timer.start.value()),
		        signal_flow::vertex::PlasticityRule::Timer::Value(new_rule.timer.period.value()),
		        new_rule.timer.num_periods},
		    old_rule.get_synapse_view_shapes(), old_rule.get_neuron_view_shapes(),
		    new_rule.recording);
		network_graph.m_graph.update_and_relocate(vertex_descriptor, std::move(vertex), inputs);
	};

	assert(network);
	assert(network_graph.m_network);

	for (auto const& [descriptor, projection] : network->projections) {
		auto const& old_projection = network_graph.m_network->projections.at(descriptor);
		if (!requires_change(projection, old_projection)) {
			continue;
		}
		update_weights(network_graph, network, descriptor);
	}
	if (network_graph.m_network->madc_recording != network->madc_recording) {
		update_madc(network_graph, *network);
	}
	if (network_graph.m_network->plasticity_rules != network->plasticity_rules) {
		for (auto const& [descriptor, new_rule] : network->plasticity_rules) {
			update_plasticity_rule(network_graph, new_rule, descriptor);
		}
	}
	network_graph.m_network = network;

	LOG4CXX_TRACE(
	    logger, "Updated hardware graph representation of network in " << timer.print() << ".");
	network_graph.m_construction_duration += std::chrono::microseconds(timer.get_us());
}


NetworkGraphBuilder::NetworkGraphBuilder(Network const& network) :
    m_network(network), m_logger(log4cxx::Logger::getLogger("grenade.NetworkGraphBuilder"))
{}

std::vector<signal_flow::Input> NetworkGraphBuilder::get_inputs(
    signal_flow::Graph const& graph, signal_flow::Graph::vertex_descriptor const descriptor)
{
	auto const& g = graph.get_graph();
	auto const edges = boost::make_iterator_range(boost::in_edges(descriptor, g));
	std::vector<signal_flow::Input> inputs;
	for (auto const& edge : edges) {
		auto const source = boost::source(edge, g);
		auto const port_restriction = graph.get_edge_property_map().at(edge);
		auto const input = port_restriction ? signal_flow::Input(source, *port_restriction)
		                                    : signal_flow::Input(source);
		inputs.push_back(input);
	}
	return inputs;
}

void NetworkGraphBuilder::add_external_input(
    signal_flow::Graph& graph,
    Resources& resources,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// only continue if any external population exists
	if (std::none_of(m_network.populations.begin(), m_network.populations.end(), [](auto const& p) {
		    return std::holds_alternative<ExternalPopulation>(p.second);
	    })) {
		return;
	}

	// add external input for spikes
	signal_flow::vertex::ExternalInput external_input(
	    signal_flow::ConnectionType::DataTimedSpikeToChipSequence, 1);
	auto const event_input_vertex = graph.add(external_input, instance, {});

	// add local execution instance data input for spikes
	signal_flow::vertex::DataInput data_input(
	    signal_flow::ConnectionType::TimedSpikeToChipSequence, 1);
	auto const data_input_vertex = graph.add(data_input, instance, {event_input_vertex});

	// add crossbar l2 input from spike data
	signal_flow::vertex::CrossbarL2Input crossbar_l2_input;
	auto const crossbar_l2_input_vertex =
	    graph.add(crossbar_l2_input, instance, {data_input_vertex});
	resources.external_input = event_input_vertex;
	resources.crossbar_l2_input = crossbar_l2_input_vertex;
	LOG4CXX_TRACE(
	    m_logger, "add_external_input(): Added external input in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_background_spike_sources(
    signal_flow::Graph& graph,
    Resources& resources,
    signal_flow::ExecutionInstance const& instance,
    RoutingResult const& routing_result) const
{
	hate::Timer timer;
	for (auto const& [descriptor, population] : m_network.populations) {
		if (!std::holds_alternative<BackgroundSpikeSourcePopulation>(population)) {
			continue;
		}
		auto const& pop = std::get<BackgroundSpikeSourcePopulation>(population);
		if (!routing_result.background_spike_source_labels.contains(descriptor)) {
			throw std::runtime_error(
			    "Connection builder result does not contain spike labels for the population(" +
			    std::to_string(descriptor) + ").");
		}
		auto const& label = routing_result.background_spike_source_labels.at(descriptor);
		haldls::vx::v3::BackgroundSpikeSource config;
		config.set_period(pop.config.period);
		config.set_rate(pop.config.rate);
		config.set_seed(pop.config.seed);
		config.set_enable(true);
		config.set_enable_random(pop.config.enable_random);
		if (pop.config.enable_random) {
			assert(!(pop.size == 0) && !(pop.size & (pop.size - 1)));
			config.set_mask(haldls::vx::v3::BackgroundSpikeSource::Mask(pop.size - 1));
		} else {
			assert(pop.size == 1);
		}
		for (auto const& [hemisphere, bus] : pop.coordinate) {
			config.set_neuron_label(label.at(hemisphere));
			signal_flow::vertex::BackgroundSpikeSource background_spike_source(
			    config, BackgroundSpikeSourceOnDLS(
			                bus.value() + hemisphere.value() * PADIBusOnPADIBusBlock::size));
			auto const background_spike_source_vertex =
			    graph.add(background_spike_source, instance, {});
			resources.background_spike_sources[descriptor][hemisphere] =
			    background_spike_source_vertex;
		}
	}
	LOG4CXX_TRACE(
	    m_logger,
	    "add_external_input(): Added background spike sources in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_population(
    signal_flow::Graph& graph,
    Resources& resources,
    std::map<HemisphereOnDLS, std::vector<signal_flow::Input>> const& input,
    PopulationDescriptor const& descriptor,
    RoutingResult const& connection_result,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	if (resources.populations.contains(
	        descriptor)) { // population already added, readd by data-reference with new input
		for (auto const& [hemisphere, new_input] : input) {
			auto const old_neuron_view_vertex =
			    resources.populations.at(descriptor).neurons.at(hemisphere);
			// use all former inputs
			auto inputs = get_inputs(graph, old_neuron_view_vertex);
			// append new input
			inputs.insert(inputs.end(), new_input.begin(), new_input.end());
			// add by using present data from former vertex with new inputs
			auto const neuron_view_vertex = graph.add(old_neuron_view_vertex, instance, inputs);
			// update graph translation and resources
			resources.populations.at(descriptor).neurons.at(hemisphere) = neuron_view_vertex;
			auto& local_graph_translation = resources.graph_translation.populations.at(descriptor);
			for (auto& neuron : local_graph_translation) {
				for (auto& [_, ans] : neuron) {
					for (auto& an : ans) {
						if (an.first == old_neuron_view_vertex) {
							an.first = neuron_view_vertex;
						}
					}
				}
			}
		}
	} else { // population not added before
		auto const& population = std::get<Population>(m_network.populations.at(descriptor));
		// split neuron locations into hemispheres
		std::map<
		    NeuronRowOnDLS, std::vector<std::tuple<
		                        signal_flow::vertex::NeuronView::Columns::value_type, size_t,
		                        CompartmentOnLogicalNeuron, size_t>>>
		    neurons;
		for (size_t n = 0; n < population.neurons.size(); ++n) {
			for (auto const& [c, ans] :
			     population.neurons.at(n).coordinate.get_placed_compartments()) {
				for (size_t an = 0; an < ans.size(); ++an) {
					neurons[ans.at(an).toNeuronRowOnDLS()].push_back(
					    std::tuple{ans.at(an).toNeuronColumnOnDLS(), n, c, an});
				}
			}
		}
		// NeuronEventOutputView requires sorted locations
		for (auto& [row, col] : neurons) {
			std::sort(col.begin(), col.end(), [](auto const& a, auto const& b) {
				return std::get<0>(a) < std::get<0>(b);
			});
		}
		auto& local_graph_translation = resources.graph_translation.populations[descriptor];
		local_graph_translation.resize(population.neurons.size());
		// add a neuron view per hemisphere
		for (auto&& [row, nrn] : neurons) {
			signal_flow::vertex::NeuronView::Configs configs;
			if (!connection_result.internal_neuron_labels.contains(descriptor)) {
				std::stringstream ss;
				ss << "Connection builder result does not contain the neuron labels for "
				   << descriptor << ".";
				throw std::runtime_error(ss.str());
			}
			signal_flow::vertex::NeuronView::Columns columns;
			for (
			    auto const& [column, neuron_on_population, compartment_on_neuron, neuron_on_compartment] :
			    nrn) {
				if (neuron_on_population >=
				    connection_result.internal_neuron_labels.at(descriptor).size()) {
					std::stringstream ss;
					ss << "Connection builder result does not contain the neuron label for "
					      "population ("
					   << descriptor << "), neuron (" << neuron_on_population << ").";
					throw std::runtime_error(ss.str());
				}
				if (!connection_result.internal_neuron_labels.at(descriptor)
				         .at(neuron_on_population)
				         .contains(compartment_on_neuron)) {
					std::stringstream ss;
					ss << "Connection builder result does not contain the neuron label for "
					      "population ("
					   << descriptor << "), neuron (" << neuron_on_population << "), compartment ("
					   << compartment_on_neuron << ").";
					throw std::runtime_error(ss.str());
				}
				if (neuron_on_compartment >= connection_result.internal_neuron_labels.at(descriptor)
				                                 .at(neuron_on_population)
				                                 .at(compartment_on_neuron)
				                                 .size()) {
					std::stringstream ss;
					ss << "Connection builder result does not contain the neuron label for "
					      "population ("
					   << descriptor << "), neuron (" << neuron_on_population << "), compartment ("
					   << compartment_on_neuron << "), neuron on compartment ("
					   << neuron_on_compartment << ").";
					throw std::runtime_error(ss.str());
				}
				signal_flow::vertex::NeuronView::Config config{
				    connection_result.internal_neuron_labels.at(descriptor)
				        .at(neuron_on_population)
				        .at(compartment_on_neuron)
				        .at(neuron_on_compartment),
				    false /* TODO: expose reset */};
				configs.push_back(config);
				columns.push_back(column);
			}
			signal_flow::vertex::NeuronView neuron_view(
			    std::move(columns), std::move(configs), row);
			auto const hemisphere = row.toHemisphereOnDLS();
			// use input for specific hemisphere
			std::vector<signal_flow::Input> inputs;
			if (input.contains(hemisphere)) {
				inputs.insert(
				    inputs.end(), input.at(hemisphere).begin(), input.at(hemisphere).end());
			}
			// add neuron view to graph and vertex descriptor to resources
			auto const neuron_view_vertex = graph.add(neuron_view, instance, inputs);
			resources.populations[descriptor].neurons[row.toHemisphereOnDLS()] = neuron_view_vertex;
			for (
			    size_t c = 0;
			    auto const& [_, neuron_on_population, compartment_on_neuron, neuron_on_compartment] :
			    nrn) {
				auto& compartment_local_graph_translation =
				    local_graph_translation.at(neuron_on_population)[compartment_on_neuron];
				compartment_local_graph_translation.resize(
				    population.neurons.at(neuron_on_population)
				        .coordinate.get_placed_compartments()
				        .at(compartment_on_neuron)
				        .size());
				compartment_local_graph_translation.at(neuron_on_compartment) =
				    std::pair{neuron_view_vertex, c};
				c++;
			}
		}
	}
	LOG4CXX_TRACE(
	    m_logger,
	    "add_population(): Added population(" << descriptor << ") in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_padi_bus(
    signal_flow::Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::PADIBusOnDLS const& coordinate,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get incoming crossbar nodes via lookup in the resources
	std::vector<signal_flow::Input> inputs;
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
		    graph.add(signal_flow::vertex::PADIBus(coordinate), instance, inputs);
	}
	LOG4CXX_TRACE(
	    m_logger, "add_padi_bus(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_crossbar_node(
    signal_flow::Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::CrossbarNodeOnDLS const& coordinate,
    RoutingResult const& connection_result,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get inputs
	std::vector<signal_flow::Input> inputs;
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
		for (auto const& [dd, d] : resources.background_spike_sources) {
			for (auto const& [hemisphere, bus] :
			     std::get<BackgroundSpikeSourcePopulation>(m_network.populations.at(dd))
			         .coordinate) {
				BackgroundSpikeSourceOnDLS source(
				    bus.value() + hemisphere.value() * PADIBusOnPADIBusBlock::size);
				if (source.toCrossbarInputOnDLS() == coordinate.toCrossbarInputOnDLS()) {
					inputs.push_back(d.at(hemisphere));
				}
			}
		}
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
		    graph.add(signal_flow::vertex::CrossbarNode(coordinate, config), instance, inputs);
	}
	LOG4CXX_TRACE(
	    m_logger, "add_crossbar_node(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_synapse_driver(
    signal_flow::Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::SynapseDriverOnDLS const& coordinate,
    RoutingResult const& connection_result,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get inputs (incoming padi bus)
	PADIBusOnDLS const padi_bus(
	    coordinate.toSynapseDriverOnSynapseDriverBlock().toPADIBusOnPADIBusBlock(),
	    coordinate.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS());
	std::vector<signal_flow::Input> const inputs = {resources.padi_busses.at(padi_bus)};
	if (resources.synapse_drivers.contains(
	        coordinate)) { // synapse driver already present, readd with new inputs
		auto& vertex = resources.synapse_drivers.at(coordinate);
		if (inputs !=
		    get_inputs(
		        graph, vertex)) { // only readd when inputs changed (FIXME: this never happens)
			vertex = graph.add(vertex, instance, inputs);
		}
	} else { // synapse driver not added yet
		signal_flow::vertex::SynapseDriver::Config::RowModes row_modes;
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
		signal_flow::vertex::SynapseDriver synapse_driver(
		    coordinate, {connection_result.synapse_driver_compare_masks.at(coordinate), row_modes,
		                 true /* TODO: expose */});
		resources.synapse_drivers[coordinate] = graph.add(synapse_driver, instance, inputs);
	}
	LOG4CXX_TRACE(
	    m_logger, "add_synapse_driver(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_neuron_event_output(
    signal_flow::Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::NeuronEventOutputOnDLS const& coordinate,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get inputs (incoming neuron views)
	std::map<HemisphereOnDLS, std::vector<signal_flow::Input>> hemisphere_inputs;
	signal_flow::vertex::NeuronEventOutputView::Neurons neurons;
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
		bool const enable_record_spikes =
		    std::any_of(population.neurons.begin(), population.neurons.end(), [](auto const nrn) {
			    for (auto const& [_, compartment] : nrn.compartments) {
				    if (compartment.spike_master &&
				        compartment.spike_master->enable_record_spikes) {
					    return true;
				    }
			    }
			    return false;
		    });
		if (!has_outgoing_projection && !enable_record_spikes) {
			continue;
		}
		// get neuron locations of population from neuron view
		for (auto const& [hemisphere, nrns] : pop.neurons) {
			auto const& neuron_view =
			    std::get<signal_flow::vertex::NeuronView>(graph.get_vertex_property(nrns));
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
				signal_flow::PortRestriction port_restriction(min, min + (ns.back().size() - 1));
				if (port_restriction.size() == neuron_view.get_columns().size()) {
					hemisphere_inputs[hemisphere].push_back(nrns);
				} else {
					hemisphere_inputs[hemisphere].push_back({nrns, port_restriction});
				}
			}
		}
	}
	// get inputs sorted by hemispheres
	std::vector<signal_flow::Input> inputs;
	for (auto const& [_, ins] : hemisphere_inputs) {
		inputs.insert(inputs.end(), ins.begin(), ins.end());
	}
	// only add neuron event output, if there are inputs present
	if (inputs.empty()) {
		return;
	}
	resources.neuron_event_outputs[coordinate] =
	    graph.add(signal_flow::vertex::NeuronEventOutputView(std::move(neurons)), instance, inputs);
	LOG4CXX_TRACE(
	    m_logger,
	    "add_neuron_event_output(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_synapse_array_view_sparse(
    signal_flow::Graph& graph,
    Resources& resources,
    ProjectionDescriptor descriptor,
    RoutingResult const& connection_result,
    signal_flow::ExecutionInstance const& instance) const
{
	if (connection_result.connections.at(descriptor).empty()) {
		resources.projections[descriptor] = {};
		resources.graph_translation.projections[descriptor] = {};
		return;
	}
	hate::Timer timer;
	// get columns of post-synaptic population
	std::map<HemisphereOnDLS, signal_flow::vertex::SynapseArrayViewSparse::Columns> columns;
	auto const post_descriptor = m_network.projections.at(descriptor).population_post;
	if (resources.populations.contains(post_descriptor)) { // post population already present
		for (auto const& [coord, d] : resources.populations.at(post_descriptor).neurons) {
			auto const& neuron_view =
			    std::get<signal_flow::vertex::NeuronView>(graph.get_vertex_property(d));
			auto& local_columns = columns[coord];
			for (auto const& c : neuron_view.get_columns()) {
				local_columns.push_back(c.toSynapseOnSynapseRow());
			}
		}
	} else { // post population not present yet
		auto const& population = std::get<Population>(m_network.populations.at(post_descriptor));
		// NeuronEventOutputView requires sorted locations
		std::vector<AtomicNeuronOnDLS> neurons;
		for (auto const& nrn : population.neurons) {
			for (auto const& an : nrn.coordinate.get_atomic_neurons()) {
				neurons.push_back(an);
			}
		}
		std::sort(neurons.begin(), neurons.end());
		for (auto const& n : neurons) {
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
	std::map<HemisphereOnDLS, signal_flow::vertex::SynapseArrayViewSparse::Rows> rows;
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
	std::map<HemisphereOnDLS, signal_flow::vertex::SynapseArrayViewSparse::Synapses> synapses;
	for (auto const& placed_connections : connection_result.connections.at(descriptor)) {
		for (auto const& placed_connection : placed_connections) {
			auto const hemisphere =
			    placed_connection.synapse_row.toSynramOnDLS().toHemisphereOnDLS();
			if (!lookup_columns.contains(hemisphere) ||
			    !lookup_columns.at(hemisphere).contains(placed_connection.synapse_on_row)) {
				std::stringstream ss;
				ss << "Placed connection post-neuron(" << hemisphere << ", "
				   << placed_connection.synapse_on_row << ") not present in post-population.";
				throw std::runtime_error(ss.str());
			}
			signal_flow::vertex::SynapseArrayViewSparse::Synapse synapse{
			    placed_connection.label, placed_connection.weight,
			    lookup_rows.at(hemisphere).at(placed_connection.synapse_row.toSynapseRowOnSynram()),
			    lookup_columns.at(hemisphere).at(placed_connection.synapse_on_row)};
			synapses[hemisphere].push_back(synapse);
		}
	}
	// get inputs
	std::map<HemisphereOnDLS, std::vector<signal_flow::Input>> inputs;
	for (auto const& [hemisphere, synapse_drivers] : used_synapse_drivers) {
		auto& local_inputs = inputs[hemisphere];
		for (auto const& synapse_driver : synapse_drivers) {
			if (!resources.synapse_drivers.contains(synapse_driver)) {
				std::stringstream ss;
				ss << synapse_driver
				   << " required by synapse array view not present in network graph builder "
				      "resources.";
				throw std::runtime_error(ss.str());
			}
			local_inputs.push_back(resources.synapse_drivers.at(synapse_driver));
		}
	}
	// add vertices
	for (auto&& [hemisphere, cs] : columns) {
		// The population might be spread over more hemispheres than the projection is.
		// If this is the case no used rows and synapses will be present, therefore we skip here.
		if (!rows.contains(hemisphere)) {
			assert(!synapses.contains(hemisphere));
			continue;
		}
		auto&& rs = rows.at(hemisphere);
		auto&& syns = synapses.at(hemisphere);
		auto const synram = hemisphere.toSynramOnDLS();
		signal_flow::vertex::SynapseArrayViewSparse config(
		    synram, std::move(rs), std::move(cs), std::move(syns));

		assert(
		    !resources.projections.contains(descriptor) ||
		    !resources.projections.at(descriptor).synapses.contains(hemisphere));
		resources.projections[descriptor].synapses[hemisphere] =
		    graph.add(config, instance, inputs.at(hemisphere));
	}
	// add translation
	auto& local_graph_translation = resources.graph_translation.projections[descriptor];
	local_graph_translation.resize(m_network.projections.at(descriptor).connections.size());
	std::map<HemisphereOnDLS, size_t> synapse_index;
	for (size_t c = 0;
	     auto const& placed_connections : connection_result.connections.at(descriptor)) {
		for (auto const& placed_connection : placed_connections) {
			auto const hemisphere =
			    placed_connection.synapse_row.toSynramOnDLS().toHemisphereOnDLS();
			if (!synapse_index.contains(hemisphere)) {
				synapse_index[hemisphere] = 0;
			}
			local_graph_translation.at(c).push_back(std::pair{
			    resources.projections.at(descriptor).synapses.at(hemisphere),
			    synapse_index.at(hemisphere)});
			synapse_index.at(hemisphere)++;
		}
		c++;
	}
	LOG4CXX_TRACE(
	    m_logger, "add_synapse_array_view(): Added synapse array view for projection("
	                  << descriptor << ") in " << timer.print() << ".");
}

std::map<HemisphereOnDLS, signal_flow::Input>
NetworkGraphBuilder::add_projection_from_external_input(
    signal_flow::Graph& graph,
    Resources& resources,
    ProjectionDescriptor const& descriptor,
    RoutingResult const& connection_result,
    signal_flow::ExecutionInstance const& instance) const
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
		    "Connection builder result does not contain connections for the projection(" +
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
			auto const index_pre =
			    m_network.projections.at(descriptor).connections.at(i).index_pre.first;
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
	for (auto const& synapse_driver : used_synapse_drivers) {
		add_synapse_driver(graph, resources, synapse_driver, connection_result, instance);
	}
	// add synapse array views
	add_synapse_array_view_sparse(graph, resources, descriptor, connection_result, instance);
	// add population
	std::map<HemisphereOnDLS, signal_flow::Input> inputs(
	    resources.projections.at(descriptor).synapses.begin(),
	    resources.projections.at(descriptor).synapses.end());
	LOG4CXX_TRACE(
	    m_logger, "add_projection_from_external(): Added projection(" << descriptor << ") in "
	                                                                  << timer.print() << ".");
	return inputs;
}

std::map<HemisphereOnDLS, signal_flow::Input>
NetworkGraphBuilder::add_projection_from_background_spike_source(
    signal_flow::Graph& graph,
    Resources& resources,
    ProjectionDescriptor const& descriptor,
    RoutingResult const& connection_result,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	auto const population_pre = m_network.projections.at(descriptor).population_pre;
	if (!std::holds_alternative<BackgroundSpikeSourcePopulation>(
	        m_network.populations.at(population_pre))) {
		throw std::logic_error(
		    "Projection's presynaptic population is not a background spike source.");
	}
	// get used PADI busses and synapse drivers
	std::set<PADIBusOnDLS> used_padi_bus;
	std::set<SynapseDriverOnDLS> used_synapse_drivers;
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
		}
		i++;
	}
	// add crossbar nodes from source to PADI busses
	for (auto const& [hemisphere, bus] :
	     std::get<BackgroundSpikeSourcePopulation>(m_network.populations.at(population_pre))
	         .coordinate) {
		auto const crossbar_input =
		    BackgroundSpikeSourceOnDLS(
		        bus.value() + hemisphere.value() * PADIBusOnPADIBusBlock::size)
		        .toCrossbarInputOnDLS();
		for (auto const& padi_bus : used_padi_bus) {
			if (padi_bus.toPADIBusBlockOnDLS().toHemisphereOnDLS() == hemisphere) {
				CrossbarNodeOnDLS coord(padi_bus.toCrossbarOutputOnDLS(), crossbar_input);
				add_crossbar_node(graph, resources, coord, connection_result, instance);
			}
		}
	}
	// add PADI busses
	for (auto const padi_bus : used_padi_bus) {
		add_padi_bus(graph, resources, padi_bus, instance);
	}
	// add synapse drivers
	for (auto const& synapse_driver : used_synapse_drivers) {
		add_synapse_driver(graph, resources, synapse_driver, connection_result, instance);
	}
	// add synapse array views
	add_synapse_array_view_sparse(graph, resources, descriptor, connection_result, instance);
	// add population
	std::map<HemisphereOnDLS, signal_flow::Input> inputs(
	    resources.projections.at(descriptor).synapses.begin(),
	    resources.projections.at(descriptor).synapses.end());
	LOG4CXX_TRACE(
	    m_logger, "add_projection_from_background_spike_source(): Added projection("
	                  << descriptor << ") in " << timer.print() << ".");
	return inputs;
}

std::map<HemisphereOnDLS, signal_flow::Input>
NetworkGraphBuilder::add_projection_from_internal_input(
    signal_flow::Graph& graph,
    Resources& resources,
    ProjectionDescriptor const& descriptor,
    RoutingResult const& connection_result,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	if (!std::holds_alternative<Population>(
	        m_network.populations.at(m_network.projections.at(descriptor).population_pre))) {
		throw std::logic_error("Projection's presynaptic population is not internal.");
	}
	if (!connection_result.connections.contains(descriptor)) {
		throw std::runtime_error(
		    "Connection builder result does not contain connections for the projection(" +
		    std::to_string(descriptor) + ").");
	}
	// get possibly used neuron event outputs
	auto const& population = std::get<Population>(
	    m_network.populations.at(m_network.projections.at(descriptor).population_pre));
	std::set<NeuronEventOutputOnDLS> used_neuron_event_outputs;
	for (auto const& neuron : population.neurons) {
		for (auto const& an : neuron.coordinate.get_atomic_neurons()) {
			used_neuron_event_outputs.insert(an.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
		}
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
		for (auto const& neuron_event_output : used_neuron_event_outputs) {
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
	for (auto const& synapse_driver : used_synapse_drivers) {
		add_synapse_driver(graph, resources, synapse_driver, connection_result, instance);
	}
	// add synapse array views
	add_synapse_array_view_sparse(graph, resources, descriptor, connection_result, instance);
	// add population
	std::map<HemisphereOnDLS, signal_flow::Input> inputs(
	    resources.projections.at(descriptor).synapses.begin(),
	    resources.projections.at(descriptor).synapses.end());
	LOG4CXX_TRACE(
	    m_logger, "add_projection_from_internal(): Added projection(" << descriptor << ") in "
	                                                                  << timer.print() << ".");
	return inputs;
}

void NetworkGraphBuilder::add_populations(
    signal_flow::Graph& graph,
    Resources& resources,
    RoutingResult const& connection_result,
    signal_flow::ExecutionInstance const& instance) const
{
	// place all populations without input
	for (auto const& [descriptor, population] : m_network.populations) {
		if (!std::holds_alternative<Population>(population)) {
			continue;
		}
		add_population(graph, resources, {}, descriptor, connection_result, instance);
	}
}

void NetworkGraphBuilder::add_neuron_event_outputs(
    signal_flow::Graph& graph,
    Resources& resources,
    signal_flow::ExecutionInstance const& instance) const
{
	for (auto const coord : iter_all<NeuronEventOutputOnDLS>()) {
		add_neuron_event_output(graph, resources, coord, instance);
	}
}

void NetworkGraphBuilder::add_external_output(
    signal_flow::Graph& graph,
    Resources& resources,
    RoutingResult const& connection_result,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	// get set of neuron event outputs from to be recorded populations
	std::set<NeuronEventOutputOnDLS> neuron_event_outputs;
	for (auto const& [descriptor, pop] : m_network.populations) {
		if (!std::holds_alternative<Population>(pop)) {
			continue;
		}
		auto const& population = std::get<Population>(pop);
		if (std::any_of(population.neurons.begin(), population.neurons.end(), [](auto const nrn) {
			    for (auto const& [_, compartment] : nrn.compartments) {
				    if (compartment.spike_master &&
				        compartment.spike_master->enable_record_spikes) {
					    return true;
				    }
			    }
			    return false;
		    })) {
			for (auto const& neuron : population.neurons) {
				for (auto const& an : neuron.coordinate.get_atomic_neurons()) {
					neuron_event_outputs.insert(
					    an.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
				}
			}
		}
	}
	if (neuron_event_outputs.empty()) {
		return;
	}
	// add crossbar nodes from neuron event outputs
	std::vector<signal_flow::Input> crossbar_l2_output_inputs;
	for (auto const& neuron_event_output : neuron_event_outputs) {
		CrossbarL2OutputOnDLS crossbar_l2_output(
		    neuron_event_output.toNeuronEventOutputOnNeuronBackendBlock());
		CrossbarNodeOnDLS coord(
		    crossbar_l2_output.toCrossbarOutputOnDLS(), neuron_event_output.toCrossbarInputOnDLS());
		add_crossbar_node(graph, resources, coord, connection_result, instance);
		crossbar_l2_output_inputs.push_back(resources.crossbar_nodes.at(coord));
	}
	// add crossbar l2 output
	auto const crossbar_l2_output =
	    graph.add(signal_flow::vertex::CrossbarL2Output(), instance, crossbar_l2_output_inputs);
	resources.external_output = graph.add(
	    signal_flow::vertex::DataOutput(signal_flow::ConnectionType::TimedSpikeFromChipSequence, 1),
	    instance, {crossbar_l2_output});
	LOG4CXX_TRACE(
	    m_logger, "add_external_output(): Added external output in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_plasticity_rules(
    signal_flow::Graph& graph,
    Resources& resources,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	for (auto const& [descriptor, plasticity_rule] : m_network.plasticity_rules) {
		std::vector<signal_flow::Input> inputs;
		std::vector<signal_flow::vertex::PlasticityRule::SynapseViewShape> synapse_view_shapes;
		for (auto const& d : plasticity_rule.projections) {
			for (auto const& [_, p] : resources.projections.at(d).synapses) {
				inputs.push_back({p});
				auto const& synapse_view = std::get<signal_flow::vertex::SynapseArrayViewSparse>(
				    graph.get_vertex_property(p));
				synapse_view_shapes.push_back(signal_flow::vertex::PlasticityRule::SynapseViewShape{
				    synapse_view.get_rows().size(),
				    std::vector<halco::hicann_dls::vx::v3::SynapseOnSynapseRow>{
				        synapse_view.get_columns().begin(), synapse_view.get_columns().end()},
				    synapse_view.get_synram().toHemisphereOnDLS()});
			}
		}
		std::vector<signal_flow::vertex::PlasticityRule::NeuronViewShape> neuron_view_shapes;
		for (auto const& d : plasticity_rule.populations) {
			std::map<
			    signal_flow::Graph::vertex_descriptor,
			    std::vector<std::optional<
			        signal_flow::vertex::PlasticityRule::NeuronViewShape::NeuronReadoutSource>>>
			    neuron_readout_sources;
			for (auto const& [_, p] : resources.populations.at(d.descriptor).neurons) {
				auto const& neuron_view =
				    std::get<signal_flow::vertex::NeuronView>(graph.get_vertex_property(p));
				neuron_readout_sources[p].resize(neuron_view.get_columns().size());
			}
			for (size_t i = 0;
			     auto const& nrn : resources.graph_translation.populations.at(d.descriptor)) {
				for (auto const& [compartment, ans] : nrn) {
					for (size_t an = 0; an < ans.size(); ++an) {
						neuron_readout_sources.at(ans.at(an).first).at(ans.at(an).second) =
						    d.neuron_readout_sources.at(i).at(compartment).at(an);
					}
				}
				++i;
			}
			for (auto const& [_, p] : resources.populations.at(d.descriptor).neurons) {
				inputs.push_back({p});
				auto const& neuron_view =
				    std::get<signal_flow::vertex::NeuronView>(graph.get_vertex_property(p));
				neuron_view_shapes.push_back(signal_flow::vertex::PlasticityRule::NeuronViewShape{
				    neuron_view.get_columns(), neuron_view.get_row(),
				    neuron_readout_sources.at(p)});
			}
		}
		signal_flow::vertex::PlasticityRule vertex(
		    plasticity_rule.kernel,
		    signal_flow::vertex::PlasticityRule::Timer{
		        signal_flow::vertex::PlasticityRule::Timer::Value(
		            plasticity_rule.timer.start.value()),
		        signal_flow::vertex::PlasticityRule::Timer::Value(
		            plasticity_rule.timer.period.value()),
		        plasticity_rule.timer.num_periods},
		    std::move(synapse_view_shapes), std::move(neuron_view_shapes),
		    plasticity_rule.recording);
		auto const output = vertex.output();
		auto const plasticity_rule_descriptor = graph.add(std::move(vertex), instance, inputs);
		resources.plasticity_rules[descriptor] = plasticity_rule_descriptor;
		if (plasticity_rule.recording) {
			resources.plasticity_rules_output[descriptor] = graph.add(
			    signal_flow::vertex::DataOutput(output.type, output.size), instance,
			    {plasticity_rule_descriptor});
		}
	}
	LOG4CXX_TRACE(
	    m_logger, "add_plasticity_rules(): Added plasticity rules in " << timer.print() << ".");
}

NetworkGraph::SpikeLabels NetworkGraphBuilder::get_spike_labels(
    RoutingResult const& connection_result)
{
	hate::Timer timer;
	NetworkGraph::SpikeLabels spike_labels;
	for (auto const& [descriptor, labels] : connection_result.external_spike_labels) {
		auto& local = spike_labels[descriptor];
		size_t i = 0;
		for (auto const& ll : labels) {
			local.push_back({});
			local.at(i)[CompartmentOnLogicalNeuron()] = {};
			for (auto const& label : ll) {
				local.at(i).at(CompartmentOnLogicalNeuron()).push_back(label);
			}
			i++;
		}
	}
	for (auto const& [descriptor, pop] : m_network.populations) {
		if (std::holds_alternative<Population>(pop)) {
			auto const& population = std::get<Population>(pop);
			auto& local_spike_labels = spike_labels[descriptor];
			local_spike_labels.resize(population.neurons.size());
			if (!connection_result.internal_neuron_labels.contains(descriptor)) {
				throw std::runtime_error(
				    "Connection builder result does not contain neuron labels for the population(" +
				    std::to_string(descriptor) + ").");
			}

			auto const& local_neuron_labels =
			    connection_result.internal_neuron_labels.at(descriptor);

			if (local_neuron_labels.size() != population.neurons.size()) {
				std::stringstream ss;
				ss << "Connection builder result does not contain the neuron labels for population "
				      "("
				   << descriptor << ").";
				throw std::runtime_error(ss.str());
			}
			for (size_t i = 0; i < population.neurons.size(); ++i) {
				for (auto const& [compartment_on_neuron, ans] :
				     population.neurons.at(i).coordinate.get_placed_compartments()) {
					if (!local_neuron_labels.at(i).contains(compartment_on_neuron)) {
						std::stringstream ss;
						ss << "Connection builder result does not contain the neuron label for "
						      "population ("
						   << descriptor << "), neuron (" << i << "), compartment ("
						   << compartment_on_neuron << ").";
						throw std::runtime_error(ss.str());
					}
					if (ans.size() != local_neuron_labels.at(i).at(compartment_on_neuron).size()) {
						std::stringstream ss;
						ss << "Connection builder result does not contain the neuron labels for "
						      "population ("
						   << descriptor << "), neuron (" << i << "), compartment ("
						   << compartment_on_neuron << ").";
						throw std::runtime_error(ss.str());
					}
					auto& compartment_local_spike_labels =
					    local_spike_labels.at(i)[compartment_on_neuron];
					for (size_t an = 0; an < ans.size(); ++an) {
						if (local_neuron_labels.at(i).at(compartment_on_neuron).at(an)) {
							halco::hicann_dls::vx::v3::SpikeLabel spike_label;
							spike_label.set_neuron_event_output(
							    ans.at(an).toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
							spike_label.set_spl1_address(SPL1Address(
							    ans.at(an).toNeuronColumnOnDLS().toNeuronEventOutputOnDLS() %
							    SPL1Address::size));
							spike_label.set_neuron_backend_address_out(
							    *(local_neuron_labels.at(i).at(compartment_on_neuron).at(an)));
							compartment_local_spike_labels.push_back(spike_label);
						} else {
							compartment_local_spike_labels.push_back(std::nullopt);
						}
					}
				}
			}
		} else if (std::holds_alternative<BackgroundSpikeSourcePopulation>(pop)) {
			auto const& population = std::get<BackgroundSpikeSourcePopulation>(pop);
			auto& local_spike_labels = spike_labels[descriptor];
			if (!connection_result.background_spike_source_labels.contains(descriptor)) {
				throw std::runtime_error(
				    "Connection builder result does not contain neuron labels for the "
				    "population(" +
				    std::to_string(descriptor) + ").");
			}
			local_spike_labels.resize(population.size);
			auto const& local_labels =
			    connection_result.background_spike_source_labels.at(descriptor);
			for (auto const& [hemisphere, base_label] : local_labels) {
				for (size_t k = 0; k < population.size; ++k) {
					halco::hicann_dls::vx::v3::SpikeLabel label;
					label.set_neuron_label(NeuronLabel(base_label + k));
					local_spike_labels.at(k)[CompartmentOnLogicalNeuron()].push_back(label);
				}
			}
		} else if (std::holds_alternative<ExternalPopulation>(pop)) {
			if (!spike_labels.contains(descriptor)) {
				throw std::runtime_error(
				    "Connection builder result does not contain spike labels for the "
				    "population(" +
				    std::to_string(descriptor) + ").");
			}
		} else {
			throw std::logic_error("Population type not supported.");
		}
	}
	LOG4CXX_TRACE(
	    m_logger, "get_spike_labels(): Calculated spike labels in " << timer.print() << ".");
	return spike_labels;
}

void NetworkGraphBuilder::add_madc_recording(
    signal_flow::Graph& graph,
    Resources& resources,
    MADCRecording const& madc_recording,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	auto const& population =
	    std::get<Population>(m_network.populations.at(madc_recording.population));
	auto const neuron = population.neurons.at(madc_recording.neuron_on_population);
	auto const atomic_neuron = neuron.coordinate.get_placed_compartments()
	                               .at(madc_recording.compartment_on_neuron)
	                               .at(madc_recording.atomic_neuron_on_compartment);
	signal_flow::vertex::MADCReadoutView madc_readout(atomic_neuron, madc_recording.source);
	auto const neuron_vertex_descriptor =
	    resources.populations.at(madc_recording.population)
	        .neurons.at(atomic_neuron.toNeuronRowOnDLS().toHemisphereOnDLS());
	auto const neuron_vertex = std::get<signal_flow::vertex::NeuronView>(
	    graph.get_vertex_property(neuron_vertex_descriptor));
	auto const& columns = neuron_vertex.get_columns();
	auto const in_view_location = static_cast<size_t>(std::distance(
	    columns.begin(),
	    std::find(columns.begin(), columns.end(), atomic_neuron.toNeuronColumnOnDLS())));
	assert(in_view_location < columns.size());
	auto const madc_vertex = graph.add(
	    madc_readout, instance, {{neuron_vertex_descriptor, {in_view_location, in_view_location}}});
	resources.madc_output = graph.add(
	    signal_flow::vertex::DataOutput(
	        signal_flow::ConnectionType::TimedMADCSampleFromChipSequence, 1),
	    instance, {madc_vertex});
	LOG4CXX_TRACE(
	    m_logger, "add_madc_recording(): Added MADC recording in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_cadc_recording(
    signal_flow::Graph& graph,
    Resources& resources,
    CADCRecording const& cadc_recording,
    signal_flow::ExecutionInstance const& instance) const
{
	hate::Timer timer;
	halco::common::typed_array<
	    std::vector<
	        std::tuple<NeuronColumnOnDLS, signal_flow::Input, CADCRecording::Neuron::Source>>,
	    NeuronRowOnDLS>
	    neurons;
	for (auto const& neuron : cadc_recording.neurons) {
		auto const& population = std::get<Population>(m_network.populations.at(neuron.population));
		auto const an = population.neurons.at(neuron.neuron_on_population)
		                    .coordinate.get_placed_compartments()
		                    .at(neuron.compartment_on_neuron)
		                    .at(neuron.atomic_neuron_on_compartment);
		std::vector<AtomicNeuronOnDLS> sorted_neurons;
		for (auto const& nrn : population.neurons) {
			for (auto const& other_an : nrn.coordinate.get_atomic_neurons()) {
				if (an.toNeuronRowOnDLS() == other_an.toNeuronRowOnDLS()) {
					sorted_neurons.push_back(other_an);
				}
			}
		}
		std::sort(sorted_neurons.begin(), sorted_neurons.end());
		size_t const sorted_index = std::distance(
		    sorted_neurons.begin(), std::find(sorted_neurons.begin(), sorted_neurons.end(), an));
		assert(sorted_index < sorted_neurons.size());
		signal_flow::PortRestriction const port_restriction(sorted_index, sorted_index);
		signal_flow::Input const input(
		    resources.populations.at(neuron.population)
		        .neurons.at(an.toNeuronRowOnDLS().toHemisphereOnDLS()),
		    port_restriction);
		neurons[an.toNeuronRowOnDLS()].push_back({an.toNeuronColumnOnDLS(), input, neuron.source});
	}
	for (auto const row : iter_all<NeuronRowOnDLS>()) {
		if (neurons.at(row).empty()) {
			continue;
		}
		signal_flow::vertex::CADCMembraneReadoutView::Columns columns;
		signal_flow::vertex::CADCMembraneReadoutView::Sources sources;
		std::vector<signal_flow::Input> inputs;
		for (auto const& [c, i, s] : neurons.at(row)) {
			columns.push_back({c.toSynapseOnSynapseRow()});
			sources.push_back({s});
			inputs.push_back(i);
		}
		// TODO (Issue #3986): support source selection in vertex
		signal_flow::vertex::CADCMembraneReadoutView vertex(
		    std::move(columns), row.toSynramOnDLS(),
		    signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic, std::move(sources));
		signal_flow::vertex::DataOutput data_output(
		    signal_flow::ConnectionType::Int8, vertex.output().size);
		auto const cv = graph.add(std::move(vertex), instance, inputs);
		resources.cadc_output.push_back(graph.add(data_output, instance, {cv}));
	}
	LOG4CXX_TRACE(
	    m_logger, "add_cadc_recording(): Added CADC recording in " << timer.print() << ".");
}


} // namespace grenade::vx::network::placed_logical
