#include <gtest/gtest.h>

#include "grenade/vx/network/generate_input.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/routing_builder.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/io_data_map.h"
#include "haldls/vx/v3/event.h"

using namespace grenade::vx;
using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;
using namespace haldls::vx::v3;

TEST(network_InputGenerator, General)
{
	auto const network = std::make_shared<Network>(Network{
	    {{PopulationDescriptor(0), ExternalPopulation(3)},
	     {PopulationDescriptor(1),
	      Population({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {false, false})}},
	    {{ProjectionDescriptor(0),
	      Projection(
	          Projection::ReceptorType::excitatory,
	          {
	              Projection::Connection(0, 0, Projection::Connection::Weight(63)),
	              Projection::Connection(
	                  1, 1, Projection::Connection::Weight(63)) // third source not connected -> we
	                                                            // expect events to be filtered
	          },
	          PopulationDescriptor(0), PopulationDescriptor(1))}},
	    std::nullopt,
	    std::nullopt,
	    {},
	    {}});

	auto const routing = build_routing(network);

	auto const network_graph = build_network_graph(network, routing);

	constexpr size_t batch_size = 3; // arbitrary
	InputGenerator generator(network_graph, batch_size);

	// we expect this in every batch for every spike label
	std::vector<signal_flow::TimedSpike::Time> times_broadcasted_over_neurons_and_batches{
	    signal_flow::TimedSpike::Time(0),
	    signal_flow::TimedSpike::Time(10),
	    signal_flow::TimedSpike::Time(20),
	    signal_flow::TimedSpike::Time(30),
	};

	// we expect this in every batch
	std::vector<std::vector<signal_flow::TimedSpike::Time>> times_broadcasted_over_batches{
	    {
	        signal_flow::TimedSpike::Time(1),
	        signal_flow::TimedSpike::Time(11),
	        signal_flow::TimedSpike::Time(21),
	        signal_flow::TimedSpike::Time(31),
	    },
	    {
	        signal_flow::TimedSpike::Time(2),
	        signal_flow::TimedSpike::Time(12),
	        signal_flow::TimedSpike::Time(22),
	        signal_flow::TimedSpike::Time(32),
	    },
	    {
	        signal_flow::TimedSpike::Time(
	            42), // should not be included since source is not connected
	        signal_flow::TimedSpike::Time(42),
	        signal_flow::TimedSpike::Time(42),
	        signal_flow::TimedSpike::Time(42),
	    },
	};

	// we expect one of these in each batch
	std::vector<std::vector<std::vector<signal_flow::TimedSpike::Time>>> times_not_broadcasted{
	    {{signal_flow::TimedSpike::Time(3)},
	     {signal_flow::TimedSpike::Time(4)},
	     {signal_flow::TimedSpike::Time(42)}},
	    {{signal_flow::TimedSpike::Time(5)},
	     {signal_flow::TimedSpike::Time(6)},
	     {signal_flow::TimedSpike::Time(42)}},
	    {{signal_flow::TimedSpike::Time(7)},
	     {signal_flow::TimedSpike::Time(8)},
	     {signal_flow::TimedSpike::Time(42)}},
	};

	// select which to add by index
	auto add = [&](size_t i) {
		assert(i < 3);
		if (i == 0) {
			generator.add(times_broadcasted_over_neurons_and_batches, PopulationDescriptor(0));
		} else if (i == 1) {
			generator.add(times_broadcasted_over_batches, PopulationDescriptor(0));
		} else if (i == 2) {
			generator.add(times_not_broadcasted, PopulationDescriptor(0));
		}
	};

	// iterate all permutations of add() invocations
	std::vector<size_t> range{0, 1, 2};
	do {
		for (auto const& i : range) {
			add(i);
		}
		signal_flow::IODataMap const data = generator.done();

		EXPECT_EQ(data.batch_size(), batch_size);
		EXPECT_TRUE(network_graph.get_event_input_vertex());
		EXPECT_TRUE(data.data.contains(*network_graph.get_event_input_vertex()));
		EXPECT_TRUE(std::holds_alternative<std::vector<signal_flow::TimedSpikeSequence>>(
		    data.data.at(*network_graph.get_event_input_vertex())));
		auto const& spikes = std::get<std::vector<signal_flow::TimedSpikeSequence>>(
		    data.data.at(*network_graph.get_event_input_vertex()));
		EXPECT_EQ(spikes.size(), batch_size);

		assert(network_graph.get_spike_labels().at(PopulationDescriptor(0)).at(0).at(0));
		auto const spike_label_0 =
		    *(network_graph.get_spike_labels().at(PopulationDescriptor(0)).at(0).at(0));
		assert(network_graph.get_spike_labels().at(PopulationDescriptor(0)).at(1).at(0));
		auto const spike_label_1 =
		    *(network_graph.get_spike_labels().at(PopulationDescriptor(0)).at(1).at(0));
		std::vector<signal_flow::TimedSpikeSequence> expectation{
		    {
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(0),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(0),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(1),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(2),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(3),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(4),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(10),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(10),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(11),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(12),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(20),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(20),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(21),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(22),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(30),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(30),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(31),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(32),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		    },
		    {
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(0),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(0),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(1),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(2),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(5),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(6),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(10),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(10),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(11),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(12),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(20),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(20),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(21),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(22),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(30),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(30),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(31),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(32),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		    },
		    {
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(0),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(0),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(1),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(2),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(7),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(8),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(10),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(10),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(11),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(12),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(20),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(20),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(21),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(22),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(30),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(30),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(31),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpike(
		            signal_flow::TimedSpike::Time(32),
		            signal_flow::TimedSpike::Payload(SpikePack1ToChip({spike_label_1}))),
		    },
		};

		EXPECT_EQ(spikes, expectation);
	} while (std::next_permutation(range.begin(), range.end()));
}
