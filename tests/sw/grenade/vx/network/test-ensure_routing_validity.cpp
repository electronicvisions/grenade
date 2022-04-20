#include <gtest/gtest.h>

#include "grenade/vx/network/exception.h"
#include "grenade/vx/network/network_builder.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/routing_builder.h"
#include "hate/indent.h"
#include "hate/join.h"
#include "hate/math.h"
#include <memory>
#include <random>
#include <ranges>

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;
using namespace haldls::vx::v3;


struct RandomNetworkGenerator
{
	RandomNetworkGenerator(uintmax_t seed) : m_seed(seed), m_rng(seed) {}

	enum class PopulationType
	{
		internal,
		external,
		background
	};

	std::shared_ptr<Network> operator()()
	{
		NetworkBuilder builder;

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

		std::map<size_t, PopulationDescriptor> external_population_descriptors;
		std::map<size_t, PopulationDescriptor> internal_population_descriptors;
		std::map<size_t, PopulationDescriptor> background_population_descriptors;
		for (auto const& [type, index] : population_add_order) {
			switch (type) {
				case PopulationType::external: {
					external_population_descriptors[index] =
					    builder.add(external_populations.at(index));
					break;
				}
				case PopulationType::internal: {
					internal_population_descriptors[index] =
					    builder.add(internal_populations.at(index));
					break;
				}
				case PopulationType::background: {
					background_population_descriptors[index] =
					    builder.add(background_populations.at(index));
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
		for (auto const& proj : projections) {
			builder.add(proj);
		}

		return builder.done();
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

	std::vector<ExternalPopulation> get_external_populations()
	{
		auto const num_sources = get_num_external_sources();
		auto const num_populations = get_num_external_populations(num_sources);
		assert(num_sources >= num_populations);
		std::vector<ExternalPopulation> populations;
		size_t num_unassigned_sources = num_sources;
		for (size_t pop = 0; pop < num_populations - 1; ++pop) {
			std::uniform_int_distribution<size_t> d(
			    0, num_unassigned_sources - (num_populations - pop));
			auto const size = d(m_rng) + 1;
			num_unassigned_sources -= size;
			populations.push_back(ExternalPopulation(size));
		}
		populations.push_back(ExternalPopulation(num_unassigned_sources));
		std::shuffle(populations.begin(), populations.end(), m_rng);
		assert(
		    std::accumulate(
		        populations.begin(), populations.end(), static_cast<size_t>(0),
		        [](size_t const p, auto const& q) { return p + q.size; }) == num_sources);
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

	std::vector<Population> get_internal_populations()
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

		std::vector<Population> populations;
		size_t num_unassigned_sources = num_sources;
		for (size_t pop = 0; pop < num_populations - 1; ++pop) {
			std::uniform_int_distribution<size_t> d(
			    0, num_unassigned_sources - (num_populations - pop));
			auto const size = d(m_rng) + 1;
			std::vector<AtomicNeuronOnDLS> local_neurons;
			std::vector<bool> local_enable_record_spikes;
			for (size_t i = 0; i < size; ++i) {
				auto const index = num_sources - num_unassigned_sources + i;
				local_neurons.push_back(neurons.at(index));
				local_enable_record_spikes.push_back(enable_record_spikes.at(index));
			}
			populations.push_back(Population(local_neurons, local_enable_record_spikes));
			num_unassigned_sources -= size;
		}
		std::vector<AtomicNeuronOnDLS> local_neurons;
		std::vector<bool> local_enable_record_spikes;
		for (size_t i = 0; i < num_unassigned_sources; ++i) {
			auto const index = num_sources - num_unassigned_sources + i;
			local_neurons.push_back(neurons.at(index));
			local_enable_record_spikes.push_back(enable_record_spikes.at(index));
		}
		populations.push_back(Population(local_neurons, local_enable_record_spikes));
		std::shuffle(populations.begin(), populations.end(), m_rng);
		assert(
		    std::accumulate(
		        populations.begin(), populations.end(), static_cast<size_t>(0),
		        [](size_t const p, auto const& q) { return p + q.neurons.size(); }) == num_sources);
		return populations;
	}

	size_t get_num_background_populations()
	{
		std::uniform_int_distribution<size_t> d(0, BackgroundSpikeSourceOnDLS::size - 1);
		return d(m_rng);
	}

	std::vector<size_t> get_num_background_sources(size_t const num_populations)
	{
		size_t num_unassigned_sources = BackgroundSpikeSourceOnDLS::size;
		std::vector<size_t> sizes;
		if (num_populations == 0) {
			return sizes;
		}
		for (size_t pop = 0; pop < num_populations - 1; ++pop) {
			std::uniform_int_distribution<size_t> d(
			    static_cast<size_t>(0),
			    std::min(static_cast<size_t>(1), num_unassigned_sources - (num_populations - pop)));
			auto const size = d(m_rng) + 1;
			sizes.push_back(size);
			num_unassigned_sources -= size;
		}
		assert(num_unassigned_sources >= 1);
		std::uniform_int_distribution<size_t> d(
		    static_cast<size_t>(1), std::min(static_cast<size_t>(2), num_unassigned_sources));
		sizes.push_back(d(m_rng));
		std::sort(sizes.begin(), sizes.end(), std::greater{});
		assert(
		    std::accumulate(sizes.begin(), sizes.end(), static_cast<size_t>(0)) <=
		    BackgroundSpikeSourceOnDLS::size);
		return sizes;
	}

	std::vector<std::map<HemisphereOnDLS, PADIBusOnPADIBusBlock>> get_background_source_locations()
	{
		auto const num_populations = get_num_background_populations();
		auto const num_sources = get_num_background_sources(num_populations);
		std::vector<std::map<HemisphereOnDLS, PADIBusOnPADIBusBlock>> locations;

		std::map<HemisphereOnDLS, std::vector<PADIBusOnPADIBusBlock>> all_locations;
		std::map<HemisphereOnDLS, size_t> used_locations;
		for (auto const hemisphere : iter_all<HemisphereOnDLS>()) {
			for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
				all_locations[hemisphere].push_back(padi_bus);
			}
			std::shuffle(
			    all_locations.at(hemisphere).begin(), all_locations.at(hemisphere).end(), m_rng);
			used_locations[hemisphere] = 0;
		}

		std::bernoulli_distribution hemisphere_distribution;
		for (auto const& num : num_sources) {
			if (num == 2) {
				std::map<HemisphereOnDLS, PADIBusOnPADIBusBlock> local;
				for (auto const hemisphere : iter_all<HemisphereOnDLS>()) {
					local[hemisphere] =
					    all_locations.at(hemisphere).at(used_locations.at(hemisphere));
					used_locations.at(hemisphere)++;
				}
				locations.push_back(local);
			} else {
				assert(num == 1);
				HemisphereOnDLS hemisphere(hemisphere_distribution(m_rng));
				while (used_locations.at(hemisphere) == PADIBusOnPADIBusBlock::size) {
					hemisphere = HemisphereOnDLS(hemisphere_distribution(m_rng));
				}
				std::map<HemisphereOnDLS, PADIBusOnPADIBusBlock> local;
				local[hemisphere] = all_locations.at(hemisphere).at(used_locations.at(hemisphere));
				used_locations.at(hemisphere)++;
				locations.push_back(local);
			}
		}
		return locations;
	}

	std::vector<BackgroundSpikeSourcePopulation> get_background_populations()
	{
		auto const locations = get_background_source_locations();

		std::bernoulli_distribution enable_random_distribution(
		    std::uniform_real_distribution<double>{}(m_rng));
		auto const get_config = [&enable_random_distribution, this]() {
			BackgroundSpikeSourcePopulation::Config config;
			config.enable_random = enable_random_distribution(m_rng);
			if (config.enable_random) {
				std::uniform_int_distribution<BackgroundSpikeSource::Rate::value_type> d_rate(
				    BackgroundSpikeSource::Rate::min, BackgroundSpikeSource::Rate::max);
				config.rate = BackgroundSpikeSource::Rate(d_rate(m_rng));
				std::uniform_int_distribution<BackgroundSpikeSource::Seed::value_type> d_seed(
				    BackgroundSpikeSource::Seed::min, BackgroundSpikeSource::Seed::max);
				config.seed = BackgroundSpikeSource::Seed(d_seed(m_rng));
			}
			std::uniform_int_distribution<BackgroundSpikeSource::Period::value_type> d_period(
			    BackgroundSpikeSource::Period::min, BackgroundSpikeSource::Period::max);
			config.period = BackgroundSpikeSource::Period(d_period(m_rng));
			return config;
		};

		std::vector<BackgroundSpikeSourcePopulation> populations;
		for (auto const& location : locations) {
			auto const config = get_config();
			size_t size = 1;
			if (config.enable_random) {
				std::uniform_int_distribution<size_t> d_size(0, 8);
				size = hate::math::pow(2, d_size(m_rng));
			}
			populations.push_back(BackgroundSpikeSourcePopulation(size, location, config));
		}
		std::shuffle(populations.begin(), populations.end(), m_rng);
		return populations;
	}

	std::vector<size_t> get_max_num_projections(
	    std::vector<Population> const& populations_post) const
	{
		std::vector<size_t> num;
		for (auto const& pop : populations_post) {
			num.push_back(pop.neurons.size() * SynapseRowOnSynram::size);
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

	std::vector<Projection> get_projections(
	    std::map<size_t, PopulationDescriptor> const& external_population_descriptors,
	    std::map<size_t, PopulationDescriptor> const& internal_population_descriptors,
	    std::map<size_t, PopulationDescriptor> const& background_population_descriptors,
	    std::vector<ExternalPopulation> const& external_populations,
	    std::vector<Population> const& internal_populations,
	    std::vector<BackgroundSpikeSourcePopulation> const& background_populations)
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
			    num_projections.at(i), internal_populations.at(i).neurons.size(),
			    all_populations_pre));
		}

		std::vector<Projection> projections;

		for (size_t i = 0; i < internal_populations.size(); ++i) {
			for (auto const& [type, index] : populations_pre.at(i)) {
				PopulationDescriptor population_pre;
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
				projections.push_back(Projection(
				    Projection::ReceptorType(std::bernoulli_distribution{}(m_rng)),
				    Projection::Connections(), population_pre,
				    internal_population_descriptors.at(i)));
			}
		}

		auto ret = add_connections(
		    std::move(projections), external_population_descriptors,
		    internal_population_descriptors, background_population_descriptors,
		    external_populations, internal_populations, background_populations);
		return ret;
	}

	typed_array<bool, HemisphereOnDLS> supports_connection(Population const& /*pre*/) const
	{
		return {true, true};
	}

	typed_array<bool, HemisphereOnDLS> supports_connection(ExternalPopulation const& /*pre*/) const
	{
		return {true, true};
	}

	typed_array<bool, HemisphereOnDLS> supports_connection(
	    BackgroundSpikeSourcePopulation const& pre) const
	{
		typed_array<bool, HemisphereOnDLS> ret;
		ret.fill(false);
		for (auto const& [hemisphere, _] : pre.coordinate) {
			ret[hemisphere] = true;
		}
		return ret;
	}

	bool supports_connection(
	    std::map<PopulationDescriptor, typed_array<bool, HemisphereOnDLS>> const& support,
	    PopulationDescriptor const& pre_descriptor,
	    HemisphereOnDLS const& post_hemisphere)
	{
		return support.at(pre_descriptor)[post_hemisphere];
	}

	std::map<PopulationDescriptor, typed_array<bool, HemisphereOnDLS>> get_supports_connection(
	    std::map<size_t, PopulationDescriptor> const& external_population_descriptors,
	    std::map<size_t, PopulationDescriptor> const& internal_population_descriptors,
	    std::map<size_t, PopulationDescriptor> const& background_population_descriptors,
	    std::vector<ExternalPopulation> const& external_populations,
	    std::vector<Population> const& internal_populations,
	    std::vector<BackgroundSpikeSourcePopulation> const& background_populations)
	{
		std::map<PopulationDescriptor, typed_array<bool, HemisphereOnDLS>> ret;
		for (auto const& [index, descriptor] : external_population_descriptors) {
			ret[descriptor] = supports_connection(external_populations.at(index));
		}
		for (auto const& [index, descriptor] : internal_population_descriptors) {
			ret[descriptor] = supports_connection(internal_populations.at(index));
		}
		for (auto const& [index, descriptor] : background_population_descriptors) {
			ret[descriptor] = supports_connection(background_populations.at(index));
		}
		return ret;
	}

	std::map<PopulationDescriptor, size_t> get_population_sizes(
	    std::map<size_t, PopulationDescriptor> const& external_population_descriptors,
	    std::map<size_t, PopulationDescriptor> const& internal_population_descriptors,
	    std::map<size_t, PopulationDescriptor> const& background_population_descriptors,
	    std::vector<ExternalPopulation> const& external_populations,
	    std::vector<Population> const& internal_populations,
	    std::vector<BackgroundSpikeSourcePopulation> const& background_populations)
	{
		std::map<PopulationDescriptor, size_t> population_sizes;
		for (auto const& [index, descriptor] : external_population_descriptors) {
			population_sizes[descriptor] = external_populations.at(index).size;
		}
		for (auto const& [index, descriptor] : internal_population_descriptors) {
			population_sizes[descriptor] = internal_populations.at(index).neurons.size();
		}
		for (auto const& [index, descriptor] : background_population_descriptors) {
			population_sizes[descriptor] = background_populations.at(index).size;
		}
		return population_sizes;
	}

	size_t get_num_connections(size_t max_num_projections)
	{
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

	std::vector<Projection> add_connections(
	    std::vector<Projection>&& projections,
	    std::map<size_t, PopulationDescriptor> const& external_population_descriptors,
	    std::map<size_t, PopulationDescriptor> const& internal_population_descriptors,
	    std::map<size_t, PopulationDescriptor> const& background_population_descriptors,
	    std::vector<ExternalPopulation> const& external_populations,
	    std::vector<Population> const& internal_populations,
	    std::vector<BackgroundSpikeSourcePopulation> const& background_populations)
	{
		if (projections.empty()) {
			return std::move(projections);
		}
		auto const max_num_projections = get_max_num_projections(internal_populations);
		auto const num_connections = get_num_connections(
		    std::accumulate(max_num_projections.begin(), max_num_projections.end(), 0));
		double const num_connections_per_projection =
		    static_cast<double>(num_connections) / static_cast<double>(projections.size());

		auto const population_sizes = get_population_sizes(
		    external_population_descriptors, internal_population_descriptors,
		    background_population_descriptors, external_populations, internal_populations,
		    background_populations);

		auto const support = get_supports_connection(
		    external_population_descriptors, internal_population_descriptors,
		    background_population_descriptors, external_populations, internal_populations,
		    background_populations);

		for (auto& projection : projections) {
			auto const max_num_connections = population_sizes.at(projection.population_pre) *
			                                 population_sizes.at(projection.population_post);
			auto const num_connections = std::min(
			    static_cast<size_t>(std::max(num_connections_per_projection, 1.)),
			    max_num_connections);
			auto const index =
			    std::find_if(
			        internal_population_descriptors.begin(), internal_population_descriptors.end(),
			        [projection](auto const& p) { return p.second == projection.population_post; })
			        ->first;
			std::vector<size_t> locations(num_connections);
			std::ranges::sample(
			    std::views::iota(static_cast<size_t>(0), max_num_connections), locations.begin(),
			    num_connections, m_rng);
			std::shuffle(locations.begin(), locations.end(), m_rng);

			for (auto const location : locations) {
				size_t const index_pre = location / population_sizes.at(projection.population_post);
				size_t const index_post =
				    location % population_sizes.at(projection.population_post);
				if (!support.at(projection.population_pre)[internal_populations.at(index)
				                                               .neurons.at(index_post)
				                                               .toNeuronRowOnDLS()
				                                               .toHemisphereOnDLS()]) {
					continue;
				}
				projection.connections.push_back(Projection::Connection(
				    index_pre, index_post, Projection::Connection::Weight()));
			}
		}
		return std::move(projections);
	}

private:
	uintmax_t m_seed;
	std::mt19937 m_rng;
};


/**
 * Tests routing.
 */
void test_routing(std::shared_ptr<Network> const& network)
{
	try {
		// constructs routing
		using namespace std::literals::chrono_literals;
		RoutingOptions options;
		options.synapse_driver_allocation_policy =
		    SynapseDriverOnDLSManager::AllocationPolicyGreedy();
		options.synapse_driver_allocation_timeout = 100ms;
		auto const routing_result = grenade::vx::network::build_routing(network, options);
		// check if network is valid
		[[maybe_unused]] auto const network_graph = build_network_graph(network, routing_result);
		std::cout << "y" << std::endl;
	} catch (UnsuccessfulRouting const&) {
		std::cout << "n" << std::endl;
	}
}


TEST(EnsureRoutingValidity, RandomNetwork)
{
	constexpr size_t num = 100;

	for (size_t i = 0; i < num; ++i) {
		auto const seed = std::random_device{}();
		RandomNetworkGenerator network_generator(seed);
		auto network = network_generator();
		EXPECT_NO_THROW(test_routing(network)) << "Seed: " << seed;
	}
}
