#include "grenade/vx/network/network_graph_builder.h"

#include "grenade/vx/network/build_connection_routing.h"
#include "grenade/vx/network/build_connection_weight_split.h"
#include "grenade/vx/network/exception.h"
#include "grenade/vx/network/requires_routing.h"
#include "grenade/vx/network/vertex/transformation/external_source_merger.h"
#include "hate/math.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <map>
#include <log4cxx/logger.h>

namespace grenade::vx::network {

using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;


NetworkGraph build_network_graph(
    std::shared_ptr<Network> const& network, RoutingResult const& routing_result)
{
	hate::Timer timer;
	assert(network);
	log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("grenade.build_network_graph");

	NetworkGraph result;
	result.m_routing_duration = routing_result.timing_statistics.routing;
	result.m_network = network;

	NetworkGraphBuilder::Resources resources;

	NetworkGraphBuilder builder(*network);

	for (auto const& instance : network->topologically_sorted_execution_instance_ids) {
		resources.execution_instances.emplace(
		    instance, NetworkGraphBuilder::Resources::ExecutionInstance{});
	}

	// spike labels needed for constructions on inter-execution-instance projections below
	for (auto const& instance : network->topologically_sorted_execution_instance_ids) {
		builder.calculate_spike_labels(resources, routing_result, instance);
	}

	for (auto const& instance : network->topologically_sorted_execution_instance_ids) {
		// add external populations input
		// For inter-execution-instance sources we use the above-calculated spike-labels
		// to construct the translation here.
		builder.add_external_input(result.m_graph, resources, instance);

		// add background spike sources
		builder.add_background_spike_sources(result.m_graph, resources, instance, routing_result);

		// add on-chip populations without input
		builder.add_populations(result.m_graph, resources, routing_result, instance);

		// add MADC recording
		if (network->execution_instances.at(instance).madc_recording) {
			builder.add_madc_recording(
			    result.m_graph, resources,
			    *(network->execution_instances.at(instance).madc_recording), instance);
		}

		// add CADC recording
		if (network->execution_instances.at(instance).cadc_recording) {
			builder.add_cadc_recording(
			    result.m_graph, resources,
			    *(network->execution_instances.at(instance).cadc_recording), instance);
		}

		// add pad recording
		if (network->execution_instances.at(instance).pad_recording) {
			builder.add_pad_recording(
			    result.m_graph, resources,
			    *(network->execution_instances.at(instance).pad_recording), instance);
		}

		// add neuron event outputs
		builder.add_neuron_event_outputs(result.m_graph, resources, instance);

		// add external output
		builder.add_external_output(result.m_graph, resources, routing_result, instance);

		// add projections iteratively
		// keep track of unplaced projections
		// each iteration at least a single projection can be placed and therefore size(projections)
		// iterations are needed at most
		std::set<ProjectionOnExecutionInstance> unplaced_projections;
		for (auto const& [descriptor, _] : network->execution_instances.at(instance).projections) {
			unplaced_projections.insert(descriptor);
		}
		for (size_t iteration = 0;
		     iteration < network->execution_instances.at(instance).projections.size();
		     ++iteration) {
			std::set<ProjectionOnExecutionInstance> newly_placed_projections;
			std::map<
			    PopulationOnExecutionInstance,
			    std::map<HemisphereOnDLS, std::vector<signal_flow::Input>>>
			    inputs;
			for (auto const descriptor : unplaced_projections) {
				auto const descriptor_pre = network->execution_instances.at(instance)
				                                .projections.at(descriptor)
				                                .population_pre;
				// skip projection if presynaptic population is not yet present
				if (!resources.execution_instances.at(instance).populations.contains(
				        descriptor_pre) &&
				    std::holds_alternative<Population>(
				        network->execution_instances.at(instance).populations.at(descriptor_pre))) {
					continue;
				}
				auto const population_inputs = std::visit(
				    hate::overloaded(
				        [&](ExternalSourcePopulation const&) {
					        return builder.add_projection_from_external_input(
					            result.m_graph, resources, descriptor, routing_result, instance);
				        },
				        [&](BackgroundSourcePopulation const&) {
					        return builder.add_projection_from_background_spike_source(
					            result.m_graph, resources, descriptor, routing_result, instance);
				        },
				        [&](Population const&) {
					        return builder.add_projection_from_internal_input(
					            result.m_graph, resources, descriptor, routing_result, instance);
				        }),
				    network->execution_instances.at(instance).populations.at(descriptor_pre));
				newly_placed_projections.insert(descriptor);
				auto& local_population_inputs = inputs[network->execution_instances.at(instance)
				                                           .projections.at(descriptor)
				                                           .population_post];
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

		for (auto const& [d, p] : resources.execution_instances.at(instance).projections) {
			resources.execution_instances.at(instance).graph_translation.synapse_vertices[d] =
			    p.synapses;
		}
		for (auto const& [d, p] : resources.execution_instances.at(instance).populations) {
			resources.execution_instances.at(instance).graph_translation.neuron_vertices[d] =
			    p.neurons;
		}
	}
	for (auto const& instance : network->topologically_sorted_execution_instance_ids) {
		result.m_graph_translation.execution_instances[instance] =
		    resources.execution_instances.at(instance).graph_translation;
	}
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
	log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("grenade.update_network_graph");

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
	                               ProjectionOnExecutionInstance const descriptor,
	                               common::ExecutionInstanceID const id) {
		std::map<
		    signal_flow::Graph::vertex_descriptor,
		    signal_flow::vertex::SynapseArrayViewSparse::Synapses>
		    synapses;
		auto const synapse_vertices =
		    network_graph.get_graph_translation().execution_instances.at(id).synapse_vertices;
		for (auto const [_, vertex_descriptor] : synapse_vertices.at(descriptor)) {
			auto const& old_synapse_array_view =
			    std::get<signal_flow::vertex::SynapseArrayViewSparse>(
			        network_graph.get_graph().get_vertex_property(vertex_descriptor));
			auto const old_synapses = old_synapse_array_view.get_synapses();
			synapses[vertex_descriptor] = signal_flow::vertex::SynapseArrayViewSparse::Synapses{
			    old_synapses.begin(), old_synapses.end()};
		}
		auto const& old_projection =
		    network_graph.get_network()->execution_instances.at(id).projections.at(descriptor);
		auto const& projection = network->execution_instances.at(id).projections.at(descriptor);
		auto const& translation =
		    network_graph.get_graph_translation().execution_instances.at(id).projections;
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

	// update plasticity rule in graph such that it resembles the configuration from network to
	// update towards
	auto const update_plasticity_rule = [](NetworkGraph& network_graph,
	                                       PlasticityRule const& new_rule,
	                                       PlasticityRuleOnExecutionInstance descriptor,
	                                       common::ExecutionInstanceID const& id) {
		if (!network_graph.m_network->execution_instances.at(id).plasticity_rules.contains(
		        descriptor)) {
			throw std::runtime_error("Updating network graph only possible, if plasticity rule is "
			                         "neither added nor removed.");
		}
		if (network_graph.m_network->execution_instances.at(id)
		        .plasticity_rules.at(descriptor)
		        .recording != new_rule.recording) {
			throw std::runtime_error("Updating network graph only possible, if plasticity rule "
			                         "recording is not changed.");
		}
		// TODO (Issue #3991): merge impl. from here and add_plasticity_rule below, requires rework
		// of resources in order to align interface to NetworkGraph.
		auto const vertex_descriptor = network_graph.get_graph_translation()
		                                   .execution_instances.at(id)
		                                   .plasticity_rule_vertices.at(descriptor);
		std::vector<signal_flow::Input> inputs;
		auto const synapse_vertices =
		    network_graph.get_graph_translation().execution_instances.at(id).synapse_vertices;
		for (auto const& d : new_rule.projections) {
			for (auto const& [_, p] : synapse_vertices.at(d)) {
				inputs.push_back({p});
			}
		}
		auto const neuron_vertices =
		    network_graph.get_graph_translation().execution_instances.at(id).neuron_vertices;
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

	for (auto const& [id, execution_instance] : network->execution_instances) {
		for (auto const& [descriptor, projection] :
		     network->execution_instances.at(id).projections) {
			auto const& old_projection =
			    network_graph.m_network->execution_instances.at(id).projections.at(descriptor);
			if (!requires_change(projection, old_projection)) {
				continue;
			}
			update_weights(network_graph, network, descriptor, id);
		}
		if (network_graph.m_network->execution_instances.at(id).plasticity_rules !=
		    network->execution_instances.at(id).plasticity_rules) {
			for (auto const& [descriptor, new_rule] :
			     network->execution_instances.at(id).plasticity_rules) {
				update_plasticity_rule(network_graph, new_rule, descriptor, id);
			}
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
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	// only continue if any external population exists
	if (std::none_of(
	        m_network.execution_instances.at(instance).populations.begin(),
	        m_network.execution_instances.at(instance).populations.end(), [](auto const& p) {
		        return std::holds_alternative<ExternalSourcePopulation>(p.second);
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

	// extract inter-execution-instance projection spike label translations
	// [pre, translation]
	std::map<
	    common::ExecutionInstanceID,
	    vertex::transformation::ExternalSourceMerger::InterExecutionInstanceInput>
	    inter_execution_instance_translations;
	for (auto const& [_, projection] : m_network.inter_execution_instance_projections) {
		if (projection.population_post.toExecutionInstanceID() != instance) {
			continue;
		}
		auto& local_translations =
		    inter_execution_instance_translations[projection.population_pre
		                                              .toExecutionInstanceID()];
		for (auto const& connection : projection.connections) {
			auto const& spike_labels_pre =
			    resources.execution_instances.at(projection.population_pre.toExecutionInstanceID())
			        .graph_translation.spike_labels
			        .at(projection.population_pre.toPopulationOnExecutionInstance())
			        .at(connection.index_pre.first)
			        .at(connection.index_pre.second);
			assert(spike_labels_pre.size() == 1);
			auto const& spike_label_pre = spike_labels_pre.at(0);
			assert(spike_label_pre);

			auto const& maybe_spike_labels_post =
			    resources.execution_instances.at(instance)
			        .graph_translation.spike_labels
			        .at(projection.population_post.toPopulationOnExecutionInstance())
			        .at(connection.index_post.first)
			        .at(connection.index_post.second);
			std::vector<halco::hicann_dls::vx::v3::SpikeLabel> spike_labels_post;
			for (auto const& maybe_spike_label_post : maybe_spike_labels_post) {
				assert(maybe_spike_label_post);
				spike_labels_post.push_back(*maybe_spike_label_post);
			}

			local_translations.translation[*spike_label_pre] = spike_labels_post;
		}
	}

	// add inter-execution-instance data input(s) for spikes
	std::vector<signal_flow::Input> external_source_merger_inputs;
	external_source_merger_inputs.push_back(data_input_vertex);
	for (auto const& [execution_instance_pre, _] : inter_execution_instance_translations) {
		auto const& data_vertex_pre = resources.execution_instances.at(execution_instance_pre)
		                                  .graph_translation.event_output_vertex;
		assert(data_vertex_pre);
		signal_flow::vertex::DataInput data_input(
		    signal_flow::ConnectionType::TimedSpikeFromChipSequence, 1);
		external_source_merger_inputs.push_back(
		    graph.add(data_input, instance, {*data_vertex_pre}));
	}

	// add external source merger
	std::vector<vertex::transformation::ExternalSourceMerger::Input>
	    external_source_merger_translations;
	external_source_merger_translations.push_back(
	    vertex::transformation::ExternalSourceMerger::ExternalInput{});
	for (auto const& [execution_instance_pre, local_translations] :
	     inter_execution_instance_translations) {
		external_source_merger_translations.push_back(local_translations);
	}
	auto external_source_merger = std::make_unique<vertex::transformation::ExternalSourceMerger>(
	    external_source_merger_translations);
	signal_flow::Vertex external_source_merger_transformation(
	    std::move(signal_flow::vertex::Transformation(std::move(external_source_merger))));
	auto const external_source_merger_vertex = graph.add(
	    std::move(external_source_merger_transformation), instance, external_source_merger_inputs);

	// add crossbar l2 input from spike data
	signal_flow::vertex::CrossbarL2Input crossbar_l2_input(get_chip_coordinate(instance));
	auto const crossbar_l2_input_vertex =
	    graph.add(crossbar_l2_input, instance, {external_source_merger_vertex});
	resources.execution_instances.at(instance).graph_translation.event_input_vertex =
	    event_input_vertex;
	resources.execution_instances.at(instance).crossbar_l2_input = crossbar_l2_input_vertex;
	LOG4CXX_TRACE(
	    m_logger, "add_external_input(): Added external input in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_background_spike_sources(
    signal_flow::Graph& graph,
    Resources& resources,
    common::ExecutionInstanceID const& instance,
    RoutingResult const& routing_result) const
{
	hate::Timer timer;
	for (auto const& [descriptor, population] :
	     m_network.execution_instances.at(instance).populations) {
		if (!std::holds_alternative<BackgroundSourcePopulation>(population)) {
			continue;
		}
		auto const& pop = std::get<BackgroundSourcePopulation>(population);
		if (!routing_result.execution_instances.at(instance)
		         .background_spike_source_labels.contains(descriptor)) {
			throw std::runtime_error(
			    "Connection builder result does not contain spike labels for the population(" +
			    std::to_string(descriptor) + ").");
		}
		auto const& label =
		    routing_result.execution_instances.at(instance).background_spike_source_labels.at(
		        descriptor);
		if (pop.config.enable_random) {
			assert(!(pop.size == 0) && !(pop.size & (pop.size - 1)));
			if (!routing_result.execution_instances.at(instance)
			         .background_spike_source_masks.contains(descriptor)) {
				throw std::runtime_error(
				    "Connection builder result does not contain mask for the population(" +
				    std::to_string(descriptor) + ").");
			}
		} else {
			assert(pop.size == 1);
		}
		haldls::vx::v3::BackgroundSpikeSource config;
		config.set_period(pop.config.period);
		config.set_rate(pop.config.rate);
		config.set_seed(pop.config.seed);
		config.set_enable(true);
		config.set_enable_random(pop.config.enable_random);
		for (auto const& [hemisphere, bus] : pop.coordinate) {
			if (label.at(hemisphere).size() != pop.size) {
				throw std::runtime_error(
				    "Connection builder result does not contain as many spike labels as are "
				    "neurons in the population(" +
				    std::to_string(descriptor) + ").");
			}
			if (pop.config.enable_random) {
				auto const& mask = routing_result.execution_instances.at(instance)
				                       .background_spike_source_masks.at(descriptor);
				config.set_mask(mask.at(hemisphere));
			}
			// we choose the first label here, which is arbitrary since all other labels are
			// randomnly chosen or there exists exactly one (enable_random = false)
			config.set_neuron_label(label.at(hemisphere).at(0).get_neuron_label());
			signal_flow::vertex::BackgroundSpikeSource background_spike_source(
			    config,
			    BackgroundSpikeSourceOnDLS(
			        bus.value() + hemisphere.value() * PADIBusOnPADIBusBlock::size),
			    get_chip_coordinate(instance));
			auto const background_spike_source_vertex =
			    graph.add(background_spike_source, instance, {});
			resources.execution_instances.at(instance)
			    .graph_translation.background_spike_source_vertices[descriptor][hemisphere] =
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
    PopulationOnExecutionInstance const& descriptor,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	if (resources.execution_instances.at(instance).populations.contains(
	        descriptor)) { // population already added, readd by data-reference with new input
		for (auto const& [hemisphere, new_input] : input) {
			auto const old_neuron_view_vertex = resources.execution_instances.at(instance)
			                                        .populations.at(descriptor)
			                                        .neurons.at(hemisphere);
			// use all former inputs
			auto inputs = get_inputs(graph, old_neuron_view_vertex);
			// append new input
			inputs.insert(inputs.end(), new_input.begin(), new_input.end());
			// add by using present data from former vertex with new inputs
			auto const neuron_view_vertex = graph.add(old_neuron_view_vertex, instance, inputs);
			// update graph translation and resources
			resources.execution_instances.at(instance)
			    .populations.at(descriptor)
			    .neurons.at(hemisphere) = neuron_view_vertex;
			auto& local_graph_translation =
			    resources.execution_instances.at(instance).graph_translation.populations.at(
			        descriptor);
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
		auto const& population = std::get<Population>(
		    m_network.execution_instances.at(instance).populations.at(descriptor));
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
		auto& local_graph_translation =
		    resources.execution_instances.at(instance).graph_translation.populations[descriptor];
		local_graph_translation.resize(population.neurons.size());
		// add a neuron view per hemisphere
		for (auto&& [row, nrn] : neurons) {
			signal_flow::vertex::NeuronView::Configs configs;
			signal_flow::vertex::NeuronView::Columns columns;
			for (
			    auto const& [column, neuron_on_population, compartment_on_neuron, neuron_on_compartment] :
			    nrn) {
				std::optional<signal_flow::vertex::NeuronView::Config::Label> label;
				if (connection_result.execution_instances.at(instance)
				        .internal_neuron_labels.contains(descriptor) &&
				    (neuron_on_population < connection_result.execution_instances.at(instance)
				                                .internal_neuron_labels.at(descriptor)
				                                .size()) &&
				    connection_result.execution_instances.at(instance)
				        .internal_neuron_labels.at(descriptor)
				        .at(neuron_on_population)
				        .contains(compartment_on_neuron) &&
				    (neuron_on_compartment < connection_result.execution_instances.at(instance)
				                                 .internal_neuron_labels.at(descriptor)
				                                 .at(neuron_on_population)
				                                 .at(compartment_on_neuron)
				                                 .size())) {
					label = connection_result.execution_instances.at(instance)
					            .internal_neuron_labels.at(descriptor)
					            .at(neuron_on_population)
					            .at(compartment_on_neuron)
					            .at(neuron_on_compartment);
				}
				signal_flow::vertex::NeuronView::Config config{
				    label, false /* TODO: expose reset */};
				configs.push_back(config);
				columns.push_back(column);
			}
			signal_flow::vertex::NeuronView neuron_view(
			    std::move(columns), std::move(configs), row, get_chip_coordinate(instance));
			auto const hemisphere = row.toHemisphereOnDLS();
			// use input for specific hemisphere
			std::vector<signal_flow::Input> inputs;
			if (input.contains(hemisphere)) {
				inputs.insert(
				    inputs.end(), input.at(hemisphere).begin(), input.at(hemisphere).end());
			}
			// add neuron view to graph and vertex descriptor to resources
			auto const neuron_view_vertex = graph.add(neuron_view, instance, inputs);
			resources.execution_instances.at(instance)
			    .populations[descriptor]
			    .neurons[row.toHemisphereOnDLS()] = neuron_view_vertex;
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
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	// get incoming crossbar nodes via lookup in the resources
	std::vector<signal_flow::Input> inputs;
	for (auto const& [c, d] : resources.execution_instances.at(instance).crossbar_nodes) {
		auto const local_padi_bus = c.toCrossbarOutputOnDLS().toPADIBusOnDLS();
		if (local_padi_bus && (*local_padi_bus == coordinate)) {
			inputs.push_back(d);
		}
	}
	if (resources.execution_instances.at(instance).padi_busses.contains(
	        coordinate)) { // padi bus already added, readd with new inputs
		auto& vertex = resources.execution_instances.at(instance).padi_busses.at(coordinate);
		if (inputs != get_inputs(graph, vertex)) { // only readd when inputs changed
			vertex = graph.add(vertex, instance, inputs);
		}
	} else { // padi bus not added yet
		resources.execution_instances.at(instance).padi_busses[coordinate] = graph.add(
		    signal_flow::vertex::PADIBus(coordinate, get_chip_coordinate(instance)), instance,
		    inputs);
	}
	LOG4CXX_TRACE(
	    m_logger, "add_padi_bus(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_crossbar_node(
    signal_flow::Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::CrossbarNodeOnDLS const& coordinate,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	// get inputs
	std::vector<signal_flow::Input> inputs;
	if (coordinate.toCrossbarInputOnDLS().toSPL1Address()) { // from crossbar l2 input
		if (!resources.execution_instances.at(instance).crossbar_l2_input) {
			throw std::logic_error("Trying to add crossbar node from L2 without l2 input vertex.");
		}
		inputs.push_back(*resources.execution_instances.at(instance).crossbar_l2_input);
	} else if (coordinate.toCrossbarInputOnDLS()
	               .toNeuronEventOutputOnDLS()) { // from neuron event output
		auto const neuron_event_output =
		    *(coordinate.toCrossbarInputOnDLS().toNeuronEventOutputOnDLS());
		inputs.push_back(resources.execution_instances.at(instance).neuron_event_outputs.at(
		    neuron_event_output));
	} else { // other (currently only background sources)
		for (auto const& [dd, d] : resources.execution_instances.at(instance)
		                               .graph_translation.background_spike_source_vertices) {
			for (auto const& [hemisphere, bus] :
			     std::get<BackgroundSourcePopulation>(
			         m_network.execution_instances.at(instance).populations.at(dd))
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
	if (resources.execution_instances.at(instance).crossbar_nodes.contains(
	        coordinate)) { // crossbar node already present, readd with new inputs
		auto& vertex = resources.execution_instances.at(instance).crossbar_nodes.at(coordinate);
		if (inputs != get_inputs(graph, vertex)) { // only readd when inputs changed
			vertex = graph.add(vertex, instance, inputs);
		}
	} else { // crossbar node not added yet
		auto const config =
		    connection_result.execution_instances.at(instance).crossbar_nodes.contains(coordinate)
		        ? connection_result.execution_instances.at(instance).crossbar_nodes.at(coordinate)
		        : haldls::vx::v3::CrossbarNode::drop_all;
		resources.execution_instances.at(instance).crossbar_nodes[coordinate] = graph.add(
		    signal_flow::vertex::CrossbarNode(coordinate, config, get_chip_coordinate(instance)),
		    instance, inputs);
	}
	LOG4CXX_TRACE(
	    m_logger, "add_crossbar_node(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_synapse_driver(
    signal_flow::Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::SynapseDriverOnDLS const& coordinate,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	// get inputs (incoming padi bus)
	PADIBusOnDLS const padi_bus(
	    coordinate.toSynapseDriverOnSynapseDriverBlock().toPADIBusOnPADIBusBlock(),
	    coordinate.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS());
	std::vector<signal_flow::Input> const inputs = {
	    resources.execution_instances.at(instance).padi_busses.at(padi_bus)};
	if (resources.execution_instances.at(instance).synapse_drivers.contains(
	        coordinate)) { // synapse driver already present, readd with new inputs
		auto& vertex = resources.execution_instances.at(instance).synapse_drivers.at(coordinate);
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
			if (!connection_result.execution_instances.at(instance).synapse_row_modes.contains(
			        synapse_row)) {
				throw std::runtime_error(
				    "Connection builder result does not contain the mode for the synapse row(" +
				    std::to_string(synapse_row) + ").");
			}
			row_modes[r] = connection_result.execution_instances.at(instance).synapse_row_modes.at(
			    synapse_row);
		}
		signal_flow::vertex::SynapseDriver synapse_driver(
		    coordinate,
		    {connection_result.execution_instances.at(instance).synapse_driver_compare_masks.at(
		         coordinate),
		     row_modes, true /* TODO: expose */},
		    get_chip_coordinate(instance));
		resources.execution_instances.at(instance).synapse_drivers[coordinate] =
		    graph.add(synapse_driver, instance, inputs);
	}
	LOG4CXX_TRACE(
	    m_logger, "add_synapse_driver(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_neuron_event_output(
    signal_flow::Graph& graph,
    Resources& resources,
    halco::hicann_dls::vx::NeuronEventOutputOnDLS const& coordinate,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	// get inputs (incoming neuron views)
	std::map<HemisphereOnDLS, std::vector<signal_flow::Input>> hemisphere_inputs;
	signal_flow::vertex::NeuronEventOutputView::Neurons neurons;
	for (auto const& [descriptor, pop] : resources.execution_instances.at(instance).populations) {
		auto const is_outgoing_projection = [&](auto const& proj) {
			return proj.second.population_pre == descriptor;
		};
		// skip if the population neither has outgoing projections nor its spikes shall be recorded
		bool const has_outgoing_projection =
		    std::find_if(
		        m_network.execution_instances.at(instance).projections.begin(),
		        m_network.execution_instances.at(instance).projections.end(),
		        is_outgoing_projection) !=
		    m_network.execution_instances.at(instance).projections.end();
		auto const& population = std::get<Population>(
		    m_network.execution_instances.at(instance).populations.at(descriptor));
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
	resources.execution_instances.at(instance).neuron_event_outputs[coordinate] = graph.add(
	    signal_flow::vertex::NeuronEventOutputView(
	        std::move(neurons), get_chip_coordinate(instance)),
	    instance, inputs);
	LOG4CXX_TRACE(
	    m_logger,
	    "add_neuron_event_output(): Added " << coordinate << " in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_synapse_array_view_sparse(
    signal_flow::Graph& graph,
    Resources& resources,
    ProjectionOnExecutionInstance descriptor,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	if (!connection_result.execution_instances.at(instance).connections.contains(descriptor) ||
	    connection_result.execution_instances.at(instance).connections.at(descriptor).empty()) {
		resources.execution_instances.at(instance).projections[descriptor] = {};
		resources.execution_instances.at(instance).graph_translation.projections[descriptor] = {};
		return;
	}
	hate::Timer timer;
	// get columns of post-synaptic population
	std::map<HemisphereOnDLS, signal_flow::vertex::SynapseArrayViewSparse::Columns> columns;
	auto const post_descriptor =
	    m_network.execution_instances.at(instance).projections.at(descriptor).population_post;
	if (resources.execution_instances.at(instance).populations.contains(
	        post_descriptor)) { // post population already present
		for (auto const& [coord, d] :
		     resources.execution_instances.at(instance).populations.at(post_descriptor).neurons) {
			auto const& neuron_view =
			    std::get<signal_flow::vertex::NeuronView>(graph.get_vertex_property(d));
			auto& local_columns = columns[coord];
			for (auto const& c : neuron_view.get_columns()) {
				local_columns.push_back(c.toSynapseOnSynapseRow());
			}
		}
	} else { // post population not present yet
		auto const& population = std::get<Population>(
		    m_network.execution_instances.at(instance).populations.at(post_descriptor));
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
	if (!connection_result.execution_instances.at(instance).connections.contains(descriptor)) {
		throw std::runtime_error(
		    "Connection builder result does not contain connections for the projection(" +
		    std::to_string(descriptor) + ").");
	}
	for (auto const& placed_connections :
	     connection_result.execution_instances.at(instance).connections.at(descriptor)) {
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
	for (auto const& placed_connections :
	     connection_result.execution_instances.at(instance).connections.at(descriptor)) {
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
			if (!resources.execution_instances.at(instance).synapse_drivers.contains(
			        synapse_driver)) {
				std::stringstream ss;
				ss << synapse_driver
				   << " required by synapse array view not present in network graph builder "
				      "resources.execution_instances.at(instance).";
				throw std::runtime_error(ss.str());
			}
			local_inputs.push_back(
			    resources.execution_instances.at(instance).synapse_drivers.at(synapse_driver));
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
		    synram, std::move(rs), std::move(cs), std::move(syns), get_chip_coordinate(instance));

		assert(
		    !resources.execution_instances.at(instance).projections.contains(descriptor) ||
		    !resources.execution_instances.at(instance)
		         .projections.at(descriptor)
		         .synapses.contains(hemisphere));
		resources.execution_instances.at(instance).projections[descriptor].synapses[hemisphere] =
		    graph.add(config, instance, inputs.at(hemisphere));
	}
	// add translation
	auto& local_graph_translation =
	    resources.execution_instances.at(instance).graph_translation.projections[descriptor];
	local_graph_translation.resize(
	    m_network.execution_instances.at(instance).projections.at(descriptor).connections.size());
	std::map<HemisphereOnDLS, size_t> synapse_index;
	for (size_t c = 0;
	     auto const& placed_connections :
	     connection_result.execution_instances.at(instance).connections.at(descriptor)) {
		for (auto const& placed_connection : placed_connections) {
			auto const hemisphere =
			    placed_connection.synapse_row.toSynramOnDLS().toHemisphereOnDLS();
			if (!synapse_index.contains(hemisphere)) {
				synapse_index[hemisphere] = 0;
			}
			local_graph_translation.at(c).push_back(std::pair{
			    resources.execution_instances.at(instance)
			        .projections.at(descriptor)
			        .synapses.at(hemisphere),
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
    ProjectionOnExecutionInstance const& descriptor,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	auto const population_pre =
	    m_network.execution_instances.at(instance).projections.at(descriptor).population_pre;
	if (!std::holds_alternative<ExternalSourcePopulation>(
	        m_network.execution_instances.at(instance).populations.at(population_pre))) {
		throw std::logic_error("Projection's presynaptic population is not external.");
	}
	// get used PADI busses, synapse drivers and spl1 addresses
	std::set<PADIBusOnDLS> used_padi_bus;
	std::set<SynapseDriverOnDLS> used_synapse_drivers;
	std::map<PADIBusOnDLS, std::set<SPL1Address>> used_spl1_addresses;
	if (!connection_result.execution_instances.at(instance).external_spike_labels.contains(
	        population_pre)) {
		throw std::runtime_error(
		    "Connection builder result does not contain spike labels for the population(" +
		    std::to_string(population_pre) + ") from external input.");
	}
	auto const& external_spike_labels =
	    connection_result.execution_instances.at(instance).external_spike_labels.at(population_pre);
	auto const num_connections_projection =
	    m_network.execution_instances.at(instance).projections.at(descriptor).connections.size();
	if (!connection_result.execution_instances.at(instance).connections.contains(descriptor)) {
		if (num_connections_projection != 0) {
			throw std::runtime_error(
			    "Connection builder result does not contain connections for the projection(" +
			    std::to_string(descriptor) + ").");
		}
	} else {
		auto const num_connections_result =
		    connection_result.execution_instances.at(instance).connections.at(descriptor).size();
		if (num_connections_result != num_connections_projection) {
			throw std::runtime_error(
			    "Connection builder result connection number(" +
			    std::to_string(num_connections_result) +
			    ") is larger than expected number of connections by projection (" +
			    std::to_string(num_connections_projection) + ").");
		}
	}
	for (size_t i = 0;
	     i <
	     m_network.execution_instances.at(instance).projections.at(descriptor).connections.size();
	     ++i) {
		auto const& placed_connections =
		    connection_result.execution_instances.at(instance).connections.at(descriptor).at(i);
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
			auto const index_pre = m_network.execution_instances.at(instance)
			                           .projections.at(descriptor)
			                           .connections.at(i)
			                           .index_pre.first;
			for (auto label : external_spike_labels.at(index_pre)) {
				used_spl1_addresses[padi_bus].insert(label.get_spl1_address());
			}
		}
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
	    resources.execution_instances.at(instance).projections.at(descriptor).synapses.begin(),
	    resources.execution_instances.at(instance).projections.at(descriptor).synapses.end());
	LOG4CXX_TRACE(
	    m_logger, "add_projection_from_external(): Added projection(" << descriptor << ") in "
	                                                                  << timer.print() << ".");
	return inputs;
}

std::map<HemisphereOnDLS, signal_flow::Input>
NetworkGraphBuilder::add_projection_from_background_spike_source(
    signal_flow::Graph& graph,
    Resources& resources,
    ProjectionOnExecutionInstance const& descriptor,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	auto const population_pre =
	    m_network.execution_instances.at(instance).projections.at(descriptor).population_pre;
	if (!std::holds_alternative<BackgroundSourcePopulation>(
	        m_network.execution_instances.at(instance).populations.at(population_pre))) {
		throw std::logic_error(
		    "Projection's presynaptic population is not a background spike source.");
	}
	// get used PADI busses and synapse drivers
	std::set<PADIBusOnDLS> used_padi_bus;
	std::set<SynapseDriverOnDLS> used_synapse_drivers;
	if (!connection_result.execution_instances.at(instance).connections.contains(descriptor)) {
		throw std::runtime_error(
		    "Connection builder result does not contain connections the projection(" +
		    std::to_string(descriptor) + ").");
	}
	auto const num_connections_result =
	    connection_result.execution_instances.at(instance).connections.at(descriptor).size();
	auto const num_connections_projection =
	    m_network.execution_instances.at(instance).projections.at(descriptor).connections.size();
	if (num_connections_result != num_connections_projection) {
		throw std::runtime_error(
		    "Connection builder result connection number(" +
		    std::to_string(num_connections_result) +
		    ") is larger than expected number of connections by projection (" +
		    std::to_string(num_connections_projection) + ").");
	}
	size_t i = 0;
	for (auto const& placed_connections :
	     connection_result.execution_instances.at(instance).connections.at(descriptor)) {
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
	     std::get<BackgroundSourcePopulation>(
	         m_network.execution_instances.at(instance).populations.at(population_pre))
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
	    resources.execution_instances.at(instance).projections.at(descriptor).synapses.begin(),
	    resources.execution_instances.at(instance).projections.at(descriptor).synapses.end());
	LOG4CXX_TRACE(
	    m_logger, "add_projection_from_background_spike_source(): Added projection("
	                  << descriptor << ") in " << timer.print() << ".");
	return inputs;
}

std::map<HemisphereOnDLS, signal_flow::Input>
NetworkGraphBuilder::add_projection_from_internal_input(
    signal_flow::Graph& graph,
    Resources& resources,
    ProjectionOnExecutionInstance const& descriptor,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	if (!std::holds_alternative<Population>(
	        m_network.execution_instances.at(instance).populations.at(
	            m_network.execution_instances.at(instance)
	                .projections.at(descriptor)
	                .population_pre))) {
		throw std::logic_error("Projection's presynaptic population is not internal.");
	}
	if (!connection_result.execution_instances.at(instance).connections.contains(descriptor)) {
		throw std::runtime_error(
		    "Connection builder result does not contain connections for the projection(" +
		    std::to_string(descriptor) + ").");
	}
	// get possibly used neuron event outputs
	auto const& population =
	    std::get<Population>(m_network.execution_instances.at(instance).populations.at(
	        m_network.execution_instances.at(instance).projections.at(descriptor).population_pre));
	std::set<NeuronEventOutputOnDLS> used_neuron_event_outputs;
	for (auto const& neuron : population.neurons) {
		for (auto const& an : neuron.coordinate.get_atomic_neurons()) {
			used_neuron_event_outputs.insert(an.toNeuronColumnOnDLS().toNeuronEventOutputOnDLS());
		}
	}
	// get used PADI busses and synapse drivers
	std::set<PADIBusOnDLS> used_padi_bus;
	std::set<SynapseDriverOnDLS> used_synapse_drivers;
	for (auto const& placed_connections :
	     connection_result.execution_instances.at(instance).connections.at(descriptor)) {
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
	    resources.execution_instances.at(instance).projections.at(descriptor).synapses.begin(),
	    resources.execution_instances.at(instance).projections.at(descriptor).synapses.end());
	LOG4CXX_TRACE(
	    m_logger, "add_projection_from_internal(): Added projection(" << descriptor << ") in "
	                                                                  << timer.print() << ".");
	return inputs;
}

void NetworkGraphBuilder::add_populations(
    signal_flow::Graph& graph,
    Resources& resources,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	// place all populations without input
	for (auto const& [descriptor, population] :
	     m_network.execution_instances.at(instance).populations) {
		if (!std::holds_alternative<Population>(population)) {
			continue;
		}
		add_population(graph, resources, {}, descriptor, connection_result, instance);
	}
}

void NetworkGraphBuilder::add_neuron_event_outputs(
    signal_flow::Graph& graph,
    Resources& resources,
    common::ExecutionInstanceID const& instance) const
{
	for (auto const coord : iter_all<NeuronEventOutputOnDLS>()) {
		add_neuron_event_output(graph, resources, coord, instance);
	}
}

void NetworkGraphBuilder::add_external_output(
    signal_flow::Graph& graph,
    Resources& resources,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	// get set of neuron event outputs from to be recorded populations
	std::set<NeuronEventOutputOnDLS> neuron_event_outputs;
	for (auto const& [descriptor, pop] : m_network.execution_instances.at(instance).populations) {
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
		crossbar_l2_output_inputs.push_back(
		    resources.execution_instances.at(instance).crossbar_nodes.at(coord));
	}
	// add crossbar l2 output
	auto const crossbar_l2_output = graph.add(
	    signal_flow::vertex::CrossbarL2Output(get_chip_coordinate(instance)), instance,
	    crossbar_l2_output_inputs);
	resources.execution_instances.at(instance).graph_translation.event_output_vertex = graph.add(
	    signal_flow::vertex::DataOutput(signal_flow::ConnectionType::TimedSpikeFromChipSequence, 1),
	    instance, {crossbar_l2_output});
	LOG4CXX_TRACE(
	    m_logger, "add_external_output(): Added external output in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_plasticity_rules(
    signal_flow::Graph& graph,
    Resources& resources,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	for (auto const& [descriptor, plasticity_rule] :
	     m_network.execution_instances.at(instance).plasticity_rules) {
		std::vector<signal_flow::Input> inputs;
		std::vector<signal_flow::vertex::PlasticityRule::SynapseViewShape> synapse_view_shapes;
		for (auto const& d : plasticity_rule.projections) {
			for (auto const& [_, p] :
			     resources.execution_instances.at(instance).projections.at(d).synapses) {
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
			for (auto const& [_, p] :
			     resources.execution_instances.at(instance).populations.at(d.descriptor).neurons) {
				auto const& neuron_view =
				    std::get<signal_flow::vertex::NeuronView>(graph.get_vertex_property(p));
				neuron_readout_sources[p].resize(neuron_view.get_columns().size());
			}
			for (size_t i = 0;
			     auto const& nrn :
			     resources.execution_instances.at(instance).graph_translation.populations.at(
			         d.descriptor)) {
				for (auto const& [compartment, ans] : nrn) {
					for (size_t an = 0; an < ans.size(); ++an) {
						neuron_readout_sources.at(ans.at(an).first).at(ans.at(an).second) =
						    d.neuron_readout_sources.at(i).at(compartment).at(an);
					}
				}
				++i;
			}
			for (auto const& [_, p] :
			     resources.execution_instances.at(instance).populations.at(d.descriptor).neurons) {
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
		    plasticity_rule.recording, get_chip_coordinate(instance));
		auto const output = vertex.output();
		auto const plasticity_rule_descriptor = graph.add(std::move(vertex), instance, inputs);
		resources.execution_instances.at(instance)
		    .graph_translation.plasticity_rule_vertices[descriptor] = plasticity_rule_descriptor;
		if (plasticity_rule.recording) {
			resources.execution_instances.at(instance)
			    .graph_translation.plasticity_rule_output_vertices[descriptor] = graph.add(
			    signal_flow::vertex::DataOutput(output.type, output.size), instance,
			    {plasticity_rule_descriptor});
		}
	}
	LOG4CXX_TRACE(
	    m_logger, "add_plasticity_rules(): Added plasticity rules in " << timer.print() << ".");
}

void NetworkGraphBuilder::calculate_spike_labels(
    Resources& resources,
    RoutingResult const& connection_result,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	auto& spike_labels = resources.execution_instances.at(instance).graph_translation.spike_labels;
	for (auto const& [descriptor, labels] :
	     connection_result.execution_instances.at(instance).external_spike_labels) {
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
	for (auto const& [descriptor, pop] : m_network.execution_instances.at(instance).populations) {
		if (std::holds_alternative<Population>(pop)) {
			auto const& population = std::get<Population>(pop);
			auto& local_spike_labels = spike_labels[descriptor];
			local_spike_labels.resize(population.neurons.size());
			if (!connection_result.execution_instances.at(instance).internal_neuron_labels.contains(
			        descriptor)) {
				throw std::runtime_error(
				    "Connection builder result does not contain neuron labels for the "
				    "population(" +
				    std::to_string(descriptor) + ").");
			}

			auto const& local_neuron_labels =
			    connection_result.execution_instances.at(instance).internal_neuron_labels.at(
			        descriptor);

			if (local_neuron_labels.size() != population.neurons.size()) {
				std::stringstream ss;
				ss << "Connection builder result does not contain the neuron labels for "
				      "population "
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
						ss << "Connection builder result does not contain the neuron labels "
						      "for "
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
		} else if (std::holds_alternative<BackgroundSourcePopulation>(pop)) {
			auto const& population = std::get<BackgroundSourcePopulation>(pop);
			auto& local_spike_labels = spike_labels[descriptor];
			if (!connection_result.execution_instances.at(instance)
			         .background_spike_source_labels.contains(descriptor)) {
				throw std::runtime_error(
				    "Connection builder result does not contain neuron labels for the "
				    "population(" +
				    std::to_string(descriptor) + ").");
			}
			auto const& local_labels = connection_result.execution_instances.at(instance)
			                               .background_spike_source_labels.at(descriptor);
			local_spike_labels.resize(population.size);
			for (auto const& [hemisphere, base_label] : local_labels) {
				if (local_labels.at(hemisphere).size() != population.size) {
					throw std::runtime_error(
					    "Connection builder result does not contain as many spike labels as "
					    "are "
					    "neurons in the population(" +
					    std::to_string(descriptor) + ").");
				}
				for (size_t k = 0; k < population.size; ++k) {
					local_spike_labels.at(k)[CompartmentOnLogicalNeuron()].push_back(
					    local_labels.at(hemisphere).at(k));
				}
			}
		} else if (std::holds_alternative<ExternalSourcePopulation>(pop)) {
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
	    m_logger, "get_spike_labels(): Calculated spike labels for " << instance << " in "
	                                                                 << timer.print() << ".");
}

void NetworkGraphBuilder::add_madc_recording(
    signal_flow::Graph& graph,
    Resources& resources,
    MADCRecording const& madc_recording,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	std::vector<signal_flow::Input> inputs;
	auto const& population =
	    std::get<Population>(m_network.execution_instances.at(instance).populations.at(
	        madc_recording.neurons.at(0).coordinate.population));
	auto const neuron =
	    population.neurons.at(madc_recording.neurons.at(0).coordinate.neuron_on_population);
	auto const atomic_neuron =
	    neuron.coordinate.get_placed_compartments()
	        .at(madc_recording.neurons.at(0).coordinate.compartment_on_neuron)
	        .at(madc_recording.neurons.at(0).coordinate.atomic_neuron_on_compartment);
	signal_flow::vertex::MADCReadoutView::Source first_source{
	    atomic_neuron, madc_recording.neurons.at(0).source};

	auto const neuron_vertex_descriptor =
	    resources.execution_instances.at(instance)
	        .populations.at(madc_recording.neurons.at(0).coordinate.population)
	        .neurons.at(atomic_neuron.toNeuronRowOnDLS().toHemisphereOnDLS());
	auto const neuron_vertex = std::get<signal_flow::vertex::NeuronView>(
	    graph.get_vertex_property(neuron_vertex_descriptor));
	auto const& columns = neuron_vertex.get_columns();
	auto const in_view_location = static_cast<size_t>(std::distance(
	    columns.begin(),
	    std::find(columns.begin(), columns.end(), atomic_neuron.toNeuronColumnOnDLS())));
	assert(in_view_location < columns.size());
	inputs.push_back(
	    signal_flow::Input(neuron_vertex_descriptor, {in_view_location, in_view_location}));

	std::optional<signal_flow::vertex::MADCReadoutView::Source> second_source;
	signal_flow::vertex::MADCReadoutView::SourceSelection source_selection;
	if (madc_recording.neurons.size() == 2) {
		auto const& population =
		    std::get<Population>(m_network.execution_instances.at(instance).populations.at(
		        madc_recording.neurons.at(1).coordinate.population));
		auto const neuron =
		    population.neurons.at(madc_recording.neurons.at(1).coordinate.neuron_on_population);
		auto const atomic_neuron =
		    neuron.coordinate.get_placed_compartments()
		        .at(madc_recording.neurons.at(1).coordinate.compartment_on_neuron)
		        .at(madc_recording.neurons.at(1).coordinate.atomic_neuron_on_compartment);
		second_source = signal_flow::vertex::MADCReadoutView::Source{
		    atomic_neuron, madc_recording.neurons.at(1).source};
		source_selection.period = haldls::vx::v3::MADCConfig::ActiveMuxInputSelectLength(1);

		auto const neuron_vertex_descriptor =
		    resources.execution_instances.at(instance)
		        .populations.at(madc_recording.neurons.at(1).coordinate.population)
		        .neurons.at(atomic_neuron.toNeuronRowOnDLS().toHemisphereOnDLS());
		auto const neuron_vertex = std::get<signal_flow::vertex::NeuronView>(
		    graph.get_vertex_property(neuron_vertex_descriptor));
		auto const& columns = neuron_vertex.get_columns();
		auto const in_view_location = static_cast<size_t>(std::distance(
		    columns.begin(),
		    std::find(columns.begin(), columns.end(), atomic_neuron.toNeuronColumnOnDLS())));
		assert(in_view_location < columns.size());
		inputs.push_back(
		    signal_flow::Input(neuron_vertex_descriptor, {in_view_location, in_view_location}));
	}

	signal_flow::vertex::MADCReadoutView madc_readout(
	    first_source, second_source, source_selection, get_chip_coordinate(instance));
	auto const madc_vertex = graph.add(madc_readout, instance, inputs);
	resources.execution_instances.at(instance).graph_translation.madc_sample_output_vertex =
	    graph.add(
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
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	halco::common::typed_array<
	    std::vector<
	        std::tuple<NeuronColumnOnDLS, signal_flow::Input, CADCRecording::Neuron::Source>>,
	    NeuronRowOnDLS>
	    neurons;
	for (auto const& neuron : cadc_recording.neurons) {
		auto const& population =
		    std::get<Population>(m_network.execution_instances.at(instance).populations.at(
		        neuron.coordinate.population));
		auto const an = population.neurons.at(neuron.coordinate.neuron_on_population)
		                    .coordinate.get_placed_compartments()
		                    .at(neuron.coordinate.compartment_on_neuron)
		                    .at(neuron.coordinate.atomic_neuron_on_compartment);
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
		    resources.execution_instances.at(instance)
		        .populations.at(neuron.coordinate.population)
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
		    signal_flow::vertex::CADCMembraneReadoutView::Mode::periodic, std::move(sources),
		    get_chip_coordinate(instance));
		signal_flow::vertex::DataOutput data_output(
		    signal_flow::ConnectionType::Int8, vertex.output().size);
		auto const cv = graph.add(std::move(vertex), instance, inputs);
		resources.execution_instances.at(instance)
		    .graph_translation.cadc_sample_output_vertex.push_back(
		        graph.add(data_output, instance, {cv}));
	}
	LOG4CXX_TRACE(
	    m_logger, "add_cadc_recording(): Added CADC recording in " << timer.print() << ".");
}

void NetworkGraphBuilder::add_pad_recording(
    signal_flow::Graph& graph,
    Resources const& resources,
    PadRecording const& pad_recording,
    common::ExecutionInstanceID const& instance) const
{
	hate::Timer timer;
	for (auto const& [pad, pad_recording_source] : pad_recording.recordings) {
		auto const& pad_recording_neuron = pad_recording_source.neuron;
		auto const& population =
		    std::get<Population>(m_network.execution_instances.at(instance).populations.at(
		        pad_recording_neuron.coordinate.population));
		auto const& neuron =
		    population.neurons.at(pad_recording_neuron.coordinate.neuron_on_population);
		auto const atomic_neuron =
		    neuron.coordinate.get_placed_compartments()
		        .at(pad_recording_neuron.coordinate.compartment_on_neuron)
		        .at(pad_recording_neuron.coordinate.atomic_neuron_on_compartment);
		signal_flow::vertex::PadReadoutView::Source vertex_source{
		    atomic_neuron, pad_recording_neuron.source, pad_recording_source.enable_buffered};

		auto const neuron_vertex_descriptor =
		    resources.execution_instances.at(instance)
		        .populations.at(pad_recording_neuron.coordinate.population)
		        .neurons.at(atomic_neuron.toNeuronRowOnDLS().toHemisphereOnDLS());
		auto const neuron_vertex = std::get<signal_flow::vertex::NeuronView>(
		    graph.get_vertex_property(neuron_vertex_descriptor));
		auto const& columns = neuron_vertex.get_columns();
		auto const in_view_location = static_cast<size_t>(std::distance(
		    columns.begin(),
		    std::find(columns.begin(), columns.end(), atomic_neuron.toNeuronColumnOnDLS())));
		assert(in_view_location < columns.size());
		signal_flow::Input const input(
		    signal_flow::Input(neuron_vertex_descriptor, {in_view_location, in_view_location}));

		signal_flow::vertex::PadReadoutView pad_readout(
		    vertex_source, pad, get_chip_coordinate(instance));
		graph.add(pad_readout, instance, {input});
	}
	LOG4CXX_TRACE(m_logger, "add_pad_recording(): Added pad recording in " << timer.print() << ".");
}

common::EntityOnChip::ChipCoordinate NetworkGraphBuilder::get_chip_coordinate(
    common::ExecutionInstanceID const& instance) const
{
	std::set<common::EntityOnChip::ChipCoordinate> chip_coordinates;
	auto const& execution_instance = m_network.execution_instances.at(instance);
	for (auto const& [_, pop] : execution_instance.populations) {
		chip_coordinates.insert(
		    std::visit([](auto const& population) { return population.chip_coordinate; }, pop));
	}
	for (auto const& [_, proj] : execution_instance.projections) {
		chip_coordinates.insert(proj.chip_coordinate);
	}
	if (execution_instance.madc_recording) {
		chip_coordinates.insert(execution_instance.madc_recording->chip_coordinate);
	}
	if (execution_instance.cadc_recording) {
		chip_coordinates.insert(execution_instance.cadc_recording->chip_coordinate);
	}
	if (execution_instance.pad_recording) {
		chip_coordinates.insert(execution_instance.pad_recording->chip_coordinate);
	}
	assert(chip_coordinates.size() > 0);
	if (chip_coordinates.size() > 1) {
		throw std::logic_error("Multiple chips in single execution instance not supported (yet).");
	}
	return *chip_coordinates.begin();
}

} // namespace grenade::vx::network
