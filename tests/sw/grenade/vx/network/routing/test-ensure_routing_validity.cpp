#include <gtest/gtest.h>

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/projection_connector/sequence.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/execution/backend/initialized_connection.h"
#include "grenade/vx/execution/backend/stateful_connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/abstract/calibration/fixture.h"
#include "grenade/vx/network/abstract/mapper/greedy.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/recorder/spike.h"
#include "grenade/vx/network/exception.h"
#include "hate/indent.h"
#include "hate/join.h"
#include "hate/math.h"
#include "hxcomm/vx/zeromockconnection.h"
#include <iterator>
#include <memory>
#include <random>
#include <ranges>

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;
using namespace haldls::vx::v3;


struct RandomNetworkGenerator
{
	RandomNetworkGenerator(uintmax_t seed) : m_rng(seed) {}

	enum class PopulationType
	{
		internal,
		external,
		background
	};

	std::shared_ptr<grenade::common::Topology> operator()()
	{
		auto topology = std::make_shared<grenade::common::Topology>();

		auto const external_populations = get_external_populations();
		auto const internal_populations = get_internal_populations();
		auto const background_populations = get_background_populations();

		std::vector<std::pair<PopulationType, size_t>> population_add_order;
		for (size_t i = 0; i < external_populations.size(); ++i) {
			population_add_order.push_back(std::pair{PopulationType::external, i});
		}
		for (size_t i = 0; i < internal_populations.size(); ++i) {
			population_add_order.push_back(std::pair{PopulationType::internal, i});
		}
		for (size_t i = 0; i < background_populations.size(); ++i) {
			population_add_order.push_back(std::pair{PopulationType::background, i});
		}
		std::shuffle(population_add_order.begin(), population_add_order.end(), m_rng);

		std::map<size_t, grenade::common::VertexOnTopology> external_population_descriptors;
		std::map<size_t, grenade::common::VertexOnTopology> internal_population_descriptors;
		std::map<size_t, grenade::common::VertexOnTopology> background_population_descriptors;
		for (auto const& [type, index] : population_add_order) {
			switch (type) {
				case PopulationType::external: {
					external_population_descriptors[index] =
					    topology->add_vertex(external_populations.at(index));
					auto const recorder = get_recorder(external_populations.at(index));
					if (!recorder.get_shape().size()) {
						break;
					}
					auto const recorder_descriptor = topology->add_vertex(recorder);
					topology->add_edge(
					    external_population_descriptors.at(index), recorder_descriptor,
					    grenade::common::Edge(
					        recorder.get_input_ports().at(0).get_channels(),
					        recorder.get_input_ports().at(0).get_channels()));
					break;
				}
				case PopulationType::internal: {
					internal_population_descriptors[index] =
					    topology->add_vertex(internal_populations.at(index));
					auto const recorder = get_recorder(internal_populations.at(index));
					if (!recorder.get_shape().size()) {
						break;
					}
					auto const recorder_descriptor = topology->add_vertex(recorder);
					topology->add_edge(
					    internal_population_descriptors.at(index), recorder_descriptor,
					    grenade::common::Edge(
					        recorder.get_input_ports().at(0).get_channels(),
					        recorder.get_input_ports().at(0).get_channels()));
					break;
				}
				case PopulationType::background: {
					background_population_descriptors[index] =
					    topology->add_vertex(background_populations.at(index));
					auto const recorder = get_recorder(background_populations.at(index));
					if (!recorder.get_shape().size()) {
						break;
					}
					auto const recorder_descriptor = topology->add_vertex(recorder);
					topology->add_edge(
					    background_population_descriptors.at(index), recorder_descriptor,
					    grenade::common::Edge(
					        recorder.get_input_ports().at(0).get_channels(),
					        recorder.get_input_ports().at(0).get_channels()));
					break;
				}
				default: {
					throw std::logic_error("PopulationType not supported.");
				}
			}
		}

		auto const projections = get_projections(
		    external_population_descriptors, internal_population_descriptors,
		    background_population_descriptors, external_populations, internal_populations,
		    background_populations);
		for (auto const& [proj, source, target, receptor] : projections) {
			// don't add empty projections
			if (proj.size() == 0) {
				continue;
			}
			auto const proj_descriptor = topology->add_vertex(proj);

			auto const target_pop_section =
			    topology->get(target).get_input_ports().at(0).get_channels().subset_restriction(
			        *topology->get(target)
			             .get_input_ports()
			             .at(0)
			             .get_channels()
			             .projection({0, 1})
			             ->cartesian_product(grenade::common::ListMultiIndexSequence(
			                 {grenade::common::MultiIndex({receptor.value()})},
			                 {grenade::common::ReceptorOnCompartmentDimensionUnit()})));

			topology->add_edge(
			    source, proj_descriptor,
			    grenade::common::Edge(
			        topology->get(source).get_output_ports().at(0).get_channels(),
			        proj.get_input_ports().at(0).get_channels()));
			topology->add_edge(
			    proj_descriptor, target,
			    grenade::common::Edge(
			        proj.get_output_ports().at(0).get_channels(), *target_pop_section));
		}

		return topology;
	}

	size_t get_num_external_sources()
	{
		// at most 8192 sources are safely supported
		// to explore different orders of magnitude, we sample approx. logarithmically
		std::uniform_int_distribution<size_t> d_exponent(0, 12);
		auto const exponent = d_exponent(m_rng);
		std::uniform_int_distribution<size_t> d_fine(0, hate::math::pow(2, exponent) - 1);
		auto const result = hate::math::pow(2, exponent) + d_fine(m_rng);
		assert(result < 8192);
		return result;
	}

	size_t get_num_external_populations(size_t num_sources)
	{
		// at most as much populations as sources can exist
		// to explore different orders of magnitude, we sample approx. logarithmically
		std::uniform_int_distribution<size_t> d_exponent(0, hate::math::log2(num_sources));
		auto const exponent = d_exponent(m_rng);
		std::uniform_int_distribution<size_t> d_fine(
		    0,
		    std::min(hate::math::pow(2, exponent) - 1, num_sources - hate::math::pow(2, exponent)));
		auto const result = hate::math::pow(2, exponent) + d_fine(m_rng);
		assert(result <= num_sources);
		return result;
	}

	std::vector<grenade::common::Population> get_external_populations()
	{
		auto const num_sources = get_num_external_sources();
		auto const num_populations = get_num_external_populations(num_sources);
		assert(num_sources >= num_populations);
		std::vector<grenade::common::Population> populations;
		size_t num_unassigned_sources = num_sources;
		for (size_t pop = 0; pop < num_populations - 1; ++pop) {
			std::uniform_int_distribution<size_t> d(
			    0, num_unassigned_sources - (num_populations - pop));
			auto const size = d(m_rng) + 1;
			num_unassigned_sources -= size;
			grenade::common::Population external_source_population(
			    abstract::ExternalSourceNeuron(),
			    grenade::common::CuboidMultiIndexSequence(
			        {size}, {grenade::common::CellOnPopulationDimensionUnit()}),
			    abstract::ExternalSourceNeuron::ParameterSpace(size),
			    grenade::common::TimeDomainOnTopology());
			populations.push_back(external_source_population);
		}
		grenade::common::Population external_source_population(
		    abstract::ExternalSourceNeuron(),
		    grenade::common::CuboidMultiIndexSequence(
		        {num_unassigned_sources}, {grenade::common::CellOnPopulationDimensionUnit()}),
		    abstract::ExternalSourceNeuron::ParameterSpace(num_unassigned_sources),
		    grenade::common::TimeDomainOnTopology());
		populations.push_back(external_source_population);
		std::shuffle(populations.begin(), populations.end(), m_rng);
		assert(
		    std::accumulate(
		        populations.begin(), populations.end(), static_cast<size_t>(0),
		        [](size_t const p, auto const& q) { return p + q.size(); }) == num_sources);
		return populations;
	}

	size_t get_num_internal_sources()
	{
		std::uniform_int_distribution<size_t> d(0, AtomicNeuronOnDLS::size - 1);
		return d(m_rng);
	}

	size_t get_num_internal_populations(size_t num_sources)
	{
		if (num_sources == 0) {
			return 0;
		}
		// at most as much populations as sources can exist
		// to explore different orders of magnitude, we sample approx. logarithmically
		std::uniform_int_distribution<size_t> d_exponent(0, hate::math::log2(num_sources));
		auto const exponent = d_exponent(m_rng);
		std::uniform_int_distribution<size_t> d_fine(
		    0,
		    std::min(hate::math::pow(2, exponent) - 1, num_sources - hate::math::pow(2, exponent)));
		auto const result = hate::math::pow(2, exponent) + d_fine(m_rng);
		assert(result <= num_sources);
		return result;
	}

	std::vector<grenade::common::Population> get_internal_populations()
	{
		// generate number of neurons and populations
		auto const num_sources = get_num_internal_sources();
		auto const num_populations = get_num_internal_populations(num_sources);
		assert(num_sources >= num_populations);
		if (num_populations == 0) {
			return {};
		}

		// generate randomized neuron locations and record enables
		std::vector<AtomicNeuronOnDLS> neurons;
		std::vector<bool> enable_record_spikes;
		std::bernoulli_distribution enable_record_spikes_distribution(
		    std::uniform_real_distribution<double>{}(m_rng));
		for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
			neurons.push_back(neuron);
			enable_record_spikes.push_back(enable_record_spikes_distribution(m_rng));
		}
		std::shuffle(neurons.begin(), neurons.end(), m_rng);

		std::vector<grenade::common::Population> populations;
		size_t num_unassigned_sources = num_sources;
		for (size_t pop = 0; pop < num_populations - 1; ++pop) {
			std::uniform_int_distribution<size_t> d(
			    0, num_unassigned_sources - (num_populations - pop));
			auto const size = d(m_rng) + 1;
			grenade::common::Population population{
			    abstract::UncalibratedNeuron{
			        abstract::UncalibratedNeuron::Compartments{
			            {grenade::common::CompartmentOnNeuron(),
			             abstract::UncalibratedNeuron::Compartment{
			                 abstract::UncalibratedNeuron::Compartment::SpikeMaster(0),
			                 {{{grenade::common::ReceptorOnCompartment(0),
			                    Receptor::Type::excitatory},
			                   {grenade::common::ReceptorOnCompartment(1),
			                    Receptor::Type::inhibitory}}}}}},
			        LogicalNeuronCompartments(
			            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
			    grenade::common::CuboidMultiIndexSequence(
			        {size}, grenade::common::MultiIndex({0}),
			        {grenade::common::CellOnPopulationDimensionUnit()}),
			    abstract::UncalibratedNeuron::ParameterSpace(
			        size, {{grenade::common::CompartmentOnNeuron(), 1}}),
			    grenade::common::TimeDomainOnTopology()};
			populations.push_back(population);
			num_unassigned_sources -= size;
		}
		if (num_unassigned_sources > 0) {
			grenade::common::Population population{
			    abstract::UncalibratedNeuron{
			        abstract::UncalibratedNeuron::Compartments{
			            {grenade::common::CompartmentOnNeuron(),
			             abstract::UncalibratedNeuron::Compartment{
			                 abstract::UncalibratedNeuron::Compartment::SpikeMaster(0),
			                 {{{grenade::common::ReceptorOnCompartment(0),
			                    Receptor::Type::excitatory},
			                   {grenade::common::ReceptorOnCompartment(1),
			                    Receptor::Type::inhibitory}}}}}},
			        LogicalNeuronCompartments(
			            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
			    grenade::common::CuboidMultiIndexSequence(
			        {num_unassigned_sources}, grenade::common::MultiIndex({0}),
			        {grenade::common::CellOnPopulationDimensionUnit()}),
			    abstract::UncalibratedNeuron::ParameterSpace(
			        num_unassigned_sources, {{grenade::common::CompartmentOnNeuron(), 1}}),
			    grenade::common::TimeDomainOnTopology()};
			populations.push_back(population);
		}
		std::shuffle(populations.begin(), populations.end(), m_rng);
		assert(
		    std::accumulate(
		        populations.begin(), populations.end(), static_cast<size_t>(0),
		        [](size_t const p, auto const& q) { return p + q.size(); }) == num_sources);
		return populations;
	}

	size_t get_num_background_populations()
	{
		std::uniform_int_distribution<size_t> d(
		    0,
		    (BackgroundSpikeSourceOnDLS::size / 2 /* we always place top and bottom currently */) -
		        1);
		return d(m_rng);
	}

	std::vector<grenade::common::Population> get_background_populations()
	{
		std::vector<grenade::common::Population> populations;
		for (size_t i = 0; i < get_num_background_populations(); ++i) {
			std::uniform_int_distribution<size_t> d_size(0, 8);
			size_t const size = hate::math::pow(2, d_size(m_rng));
			grenade::common::Population poisson_source_population(
			    abstract::PoissonSourceNeuron(),
			    grenade::common::CuboidMultiIndexSequence(
			        {size}, {grenade::common::CellOnPopulationDimensionUnit()}),
			    abstract::PoissonSourceNeuron::ParameterSpace(size),
			    grenade::common::TimeDomainOnTopology());
			populations.push_back(poisson_source_population);
		}
		std::shuffle(populations.begin(), populations.end(), m_rng);
		return populations;
	}

	std::vector<size_t> get_max_num_projections(
	    std::vector<grenade::common::Population> const& populations_post) const
	{
		std::vector<size_t> num;
		for (auto const& pop : populations_post) {
			num.push_back(
			    pop.size() *
			    LogicalNeuronOnDLS(
			        dynamic_cast<abstract::UncalibratedNeuron const&>(pop.get_cell()).shape,
			        AtomicNeuronOnDLS())
			        .get_atomic_neurons()
			        .size() *
			    SynapseRowOnSynram::size);
		}
		return num;
	}

	size_t get_num_projections(size_t max_num_projections)
	{
		// to explore different orders of magnitude, we sample approx. logarithmically
		std::uniform_int_distribution<size_t> d_exponent(
		    0, hate::math::log2(std::min(static_cast<size_t>(256), max_num_projections)));
		auto const exponent = d_exponent(m_rng);
		std::uniform_int_distribution<size_t> d_fine(
		    0, std::min(
		           hate::math::pow(2, exponent) - 1,
		           max_num_projections - hate::math::pow(2, exponent)));
		auto const result = hate::math::pow(2, exponent) + d_fine(m_rng);
		assert(result <= max_num_projections);
		return result;
	}

	std::vector<std::pair<PopulationType, size_t>> get_projection_partners(
	    size_t num_projections,
	    size_t num_neurons_post,
	    std::vector<std::pair<PopulationType, size_t>> const& all_populations_pre)
	{
		if (all_populations_pre.empty()) {
			return {};
		}
		std::vector<std::pair<PopulationType, size_t>> populations_pre;
		std::map<std::pair<PopulationType, size_t>, size_t> out_degree;
		for (size_t i = 0; i < num_projections; ++i) {
			std::uniform_int_distribution<size_t> d(0, all_populations_pre.size() - 1);
			auto population_pre = all_populations_pre.at(d(m_rng));
			while (out_degree[population_pre] == num_neurons_post * SynapseDriverOnPADIBus::size *
			                                         SynapseRowOnSynapseDriver::size) {
				population_pre = all_populations_pre.at(d(m_rng));
			}
			populations_pre.push_back(population_pre);
			out_degree[population_pre]++;
		}
		return populations_pre;
	}

	std::vector<std::tuple<
	    grenade::common::Projection,
	    grenade::common::VertexOnTopology,
	    grenade::common::VertexOnTopology,
	    grenade::common::ReceptorOnCompartment>>
	get_projections(
	    std::map<size_t, grenade::common::VertexOnTopology> const& external_population_descriptors,
	    std::map<size_t, grenade::common::VertexOnTopology> const& internal_population_descriptors,
	    std::map<size_t, grenade::common::VertexOnTopology> const&
	        background_population_descriptors,
	    std::vector<grenade::common::Population> const& external_populations,
	    std::vector<grenade::common::Population> const& internal_populations,
	    std::vector<grenade::common::Population> const& background_populations)
	{
		auto const max_num_projections = get_max_num_projections(internal_populations);

		std::vector<size_t> num_projections;
		for (auto const& max : max_num_projections) {
			num_projections.push_back(get_num_projections(max));
		}

		std::vector<std::pair<PopulationType, size_t>> all_populations_pre;
		for (size_t i = 0; i < external_populations.size(); ++i) {
			all_populations_pre.push_back(std::pair{PopulationType::external, i});
		}
		for (size_t i = 0; i < internal_populations.size(); ++i) {
			all_populations_pre.push_back(std::pair{PopulationType::internal, i});
		}
		for (size_t i = 0; i < background_populations.size(); ++i) {
			all_populations_pre.push_back(std::pair{PopulationType::background, i});
		}
		std::shuffle(all_populations_pre.begin(), all_populations_pre.end(), m_rng);

		std::vector<std::vector<std::pair<PopulationType, size_t>>> populations_pre;
		for (size_t i = 0; i < internal_populations.size(); ++i) {
			populations_pre.push_back(get_projection_partners(
			    num_projections.at(i), internal_populations.at(i).size(), all_populations_pre));
		}

		std::vector<std::tuple<
		    grenade::common::Projection, grenade::common::VertexOnTopology,
		    grenade::common::VertexOnTopology, grenade::common::ReceptorOnCompartment>>
		    projections;

		auto const num_connections = get_num_connections(
		    std::accumulate(max_num_projections.begin(), max_num_projections.end(), 0));
		double const num_connections_per_projection =
		    static_cast<double>(num_connections) / static_cast<double>(projections.size());

		auto const population_sizes = get_population_sizes(
		    external_population_descriptors, internal_population_descriptors,
		    background_population_descriptors, external_populations, internal_populations,
		    background_populations);

		for (size_t i = 0; i < internal_populations.size(); ++i) {
			for (auto const& [type, index] : populations_pre.at(i)) {
				grenade::common::VertexOnTopology population_pre;
				switch (type) {
					case PopulationType::external: {
						population_pre = external_population_descriptors.at(index);
						break;
					}
					case PopulationType::internal: {
						population_pre = internal_population_descriptors.at(index);
						break;
					}
					case PopulationType::background: {
						population_pre = background_population_descriptors.at(index);
						break;
					}
					default: {
						throw std::logic_error("PopulationType not supported.");
					}
				}
				auto const population_post = internal_population_descriptors.at(i);
				auto const max_num_connections =
				    population_sizes.at(population_pre) * population_sizes.at(population_post);
				auto const num_connections = std::min(
				    static_cast<size_t>(std::max(num_connections_per_projection, 1.)),
				    max_num_connections);
				std::vector<size_t> locations(num_connections);
				std::ranges::sample(
				    std::views::iota(static_cast<size_t>(0), max_num_connections),
				    locations.begin(), num_connections, m_rng);
				std::shuffle(locations.begin(), locations.end(), m_rng);

				grenade::common::ReceptorOnCompartment receptor(
				    std::bernoulli_distribution{}(m_rng));

				std::vector<grenade::common::MultiIndex> connections;
				for (auto const location : locations) {
					size_t const index_pre = location / population_sizes.at(population_post);
					size_t const index_post = location % population_sizes.at(population_post);
					connections.push_back(grenade::common::MultiIndex({index_pre, index_post}));
				}
				projections.push_back(std::tuple{
				    grenade::common::Projection(
				        abstract::UncalibratedSynapse(),
				        abstract::UncalibratedSynapse::ParameterSpace(
				            std::vector<abstract::UncalibratedSynapse::Weight>(
				                connections.size(), abstract::UncalibratedSynapse::Weight(63))),
				        grenade::common::SequenceConnector(
				            grenade::common::CuboidMultiIndexSequence(
				                {population_sizes.at(population_pre)}),
				            grenade::common::CuboidMultiIndexSequence(
				                {population_sizes.at(population_post)}),
				            grenade::common::ListMultiIndexSequence(
				                connections,
				                grenade::common::ListMultiIndexSequence::DimensionUnits(2))),
				        grenade::common::TimeDomainOnTopology()),
				    population_pre, population_post, receptor});
			}
		}

		return projections;
	}

	std::map<grenade::common::VertexOnTopology, size_t> get_population_sizes(
	    std::map<size_t, grenade::common::VertexOnTopology> const& external_population_descriptors,
	    std::map<size_t, grenade::common::VertexOnTopology> const& internal_population_descriptors,
	    std::map<size_t, grenade::common::VertexOnTopology> const&
	        background_population_descriptors,
	    std::vector<grenade::common::Population> const& external_populations,
	    std::vector<grenade::common::Population> const& internal_populations,
	    std::vector<grenade::common::Population> const& background_populations)
	{
		std::map<grenade::common::VertexOnTopology, size_t> population_sizes;
		for (auto const& [index, descriptor] : external_population_descriptors) {
			population_sizes[descriptor] = external_populations.at(index).size();
		}
		for (auto const& [index, descriptor] : internal_population_descriptors) {
			population_sizes[descriptor] = internal_populations.at(index).size();
		}
		for (auto const& [index, descriptor] : background_population_descriptors) {
			population_sizes[descriptor] = background_populations.at(index).size();
		}
		return population_sizes;
	}

	size_t get_num_connections(size_t max_num_projections)
	{
		if (max_num_projections == 0) {
			return 0;
		}
		// to explore different orders of magnitude, we sample approx. logarithmically
		std::uniform_int_distribution<size_t> d_exponent(0, hate::math::log2(max_num_projections));
		auto const exponent = d_exponent(m_rng);
		std::uniform_int_distribution<size_t> d_fine(
		    0, std::min(
		           hate::math::pow(2, exponent) - 1,
		           max_num_projections - hate::math::pow(2, exponent)));
		auto const result = hate::math::pow(2, exponent) + d_fine(m_rng);
		assert(result <= max_num_projections);
		return result;
	}

	abstract::SpikeRecorder get_recorder(grenade::common::Population const& population)
	{
		if (population.size() == 0) {
			return abstract::SpikeRecorder(
			    grenade::common::ListMultiIndexSequence(
			        {}, population.get_output_ports().at(0).get_channels().get_dimension_units()),
			    grenade::common::TimeDomainOnTopology());
		}
		auto const population_elements =
		    population.get_output_ports().at(0).get_channels().get_elements();
		std::vector<grenade::common::MultiIndex> recorder_elements;
		std::sample(
		    population_elements.begin(), population_elements.end(),
		    std::back_inserter(recorder_elements),
		    std::uniform_int_distribution{
		        static_cast<size_t>(0), population_elements.size() - 1}(m_rng),
		    m_rng);
		return abstract::SpikeRecorder(
		    grenade::common::ListMultiIndexSequence(
		        std::move(recorder_elements),
		        population.get_output_ports().at(0).get_channels().get_dimension_units()),
		    grenade::common::TimeDomainOnTopology());
	}

private:
	std::mt19937 m_rng;
};


/**
 * Tests routing with GreedyRouter.
 */
void test_greedy_router(std::shared_ptr<grenade::common::Topology> const& network)
{
	try {
		// constructs routing
		using namespace std::literals::chrono_literals;
		routing::GreedyRouter::Options options;
		abstract::GreedyMapper router;
		routing::GreedyRouter::Options router_options;
		router_options.synapse_driver_allocation_policy =
		    routing::GreedyRouter::Options::AllocationPolicyGreedy();
		router_options.synapse_driver_allocation_timeout = 100ms;
		router.set_router(std::make_shared<routing::GreedyRouter>(router_options));
		abstract::FixtureCalibration calibration;
		std::map<
		    grenade::common::ConnectionOnExecutor,
		    grenade::vx::execution::backend::StatefulConnection>
		    connections;
		connections.emplace(
		    grenade::common::ConnectionOnExecutor(),
		    grenade::vx::execution::backend::StatefulConnection(
		        grenade::vx::execution::backend::InitializedConnection(
		            hxcomm::MultiConnection<hxcomm::vx::ZeroMockConnection>()),
		        {{true}}));
		grenade::vx::execution::JITGraphExecutor executor(std::move(connections));
		[[maybe_unused]] auto const mapped_topology = router(network, calibration, executor);
	} catch (UnsuccessfulRouting const&) {
	}
}


TEST(GreedyRouter, EnsureRoutingValidity_RandomNetwork)
{
	constexpr size_t num = 100;

	for (size_t i = 0; i < num; ++i) {
		auto const seed = std::random_device{}();
		RandomNetworkGenerator network_generator(seed);
		auto network = network_generator();
		EXPECT_NO_THROW(test_greedy_router(network)) << "Seed: " << seed;
	}
}
