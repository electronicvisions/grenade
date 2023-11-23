#include <gtest/gtest.h>

#include "grenade/vx/network/generate_input.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/network_graph_builder.h"
#include "grenade/vx/network/routing/portfolio_router.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "haldls/vx/v3/event.h"

using namespace grenade::vx;
using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;
using namespace haldls::vx::v3;

TEST(network_InputGenerator, General)
{
	auto const network = std::make_shared<Network>(Network{
	    {{common::ExecutionInstanceID(),
	      Network::ExecutionInstance{
	          {{PopulationOnExecutionInstance(0),
	            ExternalSourcePopulation(
	                {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron(true),
	                 ExternalSourcePopulation::Neuron()})},
	           {PopulationOnExecutionInstance(1),
	            Population(
	                {Population::Neuron(
	                     LogicalNeuronOnDLS(
	                         LogicalNeuronCompartments(
	                             {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	                         AtomicNeuronOnDLS(Enum(0))),
	                     Population::Neuron::Compartments{
	                         {CompartmentOnLogicalNeuron(),
	                          Population::Neuron::Compartment{
	                              Population::Neuron::Compartment::SpikeMaster(0, false),
	                              {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}}),
	                 Population::Neuron(
	                     LogicalNeuronOnDLS(
	                         LogicalNeuronCompartments(
	                             {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}}),
	                         AtomicNeuronOnDLS(Enum(1))),
	                     Population::Neuron::Compartments{
	                         {CompartmentOnLogicalNeuron(),
	                          Population::Neuron::Compartment{
	                              Population::Neuron::Compartment::SpikeMaster(0, false),
	                              {{Receptor(Receptor::ID(), Receptor::Type::excitatory)}}}}})})}},
	          {{ProjectionOnExecutionInstance(0),
	            Projection(
	                Receptor(Receptor::ID(), Receptor::Type::excitatory),
	                {
	                    Projection::Connection(
	                        {0, CompartmentOnLogicalNeuron()}, {0, CompartmentOnLogicalNeuron()},
	                        Projection::Connection::Weight(63))
	                    // second source not connected, but recorded -> we
	                    // expect events to be present
	                    // third source not connected and not recorded -> we
	                    // expect events to be filtered
	                },
	                PopulationOnExecutionInstance(0), PopulationOnExecutionInstance(1))}},
	          std::nullopt,
	          std::nullopt,
	          std::nullopt,
	          {}}}},
	    {},
	    {common::ExecutionInstanceID()},
	    {}});

	auto const routing = routing::PortfolioRouter()(network);

	auto const network_graph = build_network_graph(network, routing);

	constexpr size_t batch_size = 3; // arbitrary
	InputGenerator generator(network_graph, batch_size);

	// we expect this in every batch for every spike label
	std::vector<common::Time> times_broadcasted_over_neurons_and_batches{
	    common::Time(0),
	    common::Time(10),
	    common::Time(20),
	    common::Time(30),
	};

	// we expect this in every batch
	std::vector<std::vector<common::Time>> times_broadcasted_over_batches{
	    {
	        common::Time(1),
	        common::Time(11),
	        common::Time(21),
	        common::Time(31),
	    },
	    {
	        common::Time(2),
	        common::Time(12),
	        common::Time(22),
	        common::Time(32),
	    },
	    {
	        common::Time(42), // should not be included since source is not connected
	        common::Time(42),
	        common::Time(42),
	        common::Time(42),
	    },
	};

	// we expect one of these in each batch
	std::vector<std::vector<std::vector<common::Time>>> times_not_broadcasted{
	    {{common::Time(3)}, {common::Time(4)}, {common::Time(42)}},
	    {{common::Time(5)}, {common::Time(6)}, {common::Time(42)}},
	    {{common::Time(7)}, {common::Time(8)}, {common::Time(42)}},
	};

	// select which to add by index
	auto add = [&](size_t i) {
		assert(i < 3);
		if (i == 0) {
			generator.add(times_broadcasted_over_neurons_and_batches, PopulationOnNetwork());
		} else if (i == 1) {
			generator.add(times_broadcasted_over_batches, PopulationOnNetwork());
		} else if (i == 2) {
			generator.add(times_not_broadcasted, PopulationOnNetwork());
		}
	};

	// iterate all permutations of add() invocations
	std::vector<size_t> range{0, 1, 2};
	do {
		for (auto const& i : range) {
			add(i);
		}
		signal_flow::InputData const data = generator.done();

		EXPECT_EQ(data.batch_size(), batch_size);
		EXPECT_TRUE(network_graph.get_graph_translation()
		                .execution_instances.at(grenade::vx::common::ExecutionInstanceID())
		                .event_input_vertex);
		EXPECT_TRUE(data.data.contains(
		    *network_graph.get_graph_translation()
		         .execution_instances.at(grenade::vx::common::ExecutionInstanceID())
		         .event_input_vertex));
		EXPECT_TRUE(std::holds_alternative<std::vector<signal_flow::TimedSpikeToChipSequence>>(
		    data.data.at(*network_graph.get_graph_translation()
		                      .execution_instances.at(grenade::vx::common::ExecutionInstanceID())
		                      .event_input_vertex)));
		auto const& spikes = std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(
		    data.data.at(*network_graph.get_graph_translation()
		                      .execution_instances.at(grenade::vx::common::ExecutionInstanceID())
		                      .event_input_vertex));
		EXPECT_EQ(spikes.size(), batch_size);

		assert(network_graph.get_graph_translation()
		           .execution_instances.at(common::ExecutionInstanceID())
		           .spike_labels.at(PopulationOnExecutionInstance(0))
		           .at(0)
		           .at(CompartmentOnLogicalNeuron())
		           .at(0));
		auto const spike_label_0 = *(network_graph.get_graph_translation()
		                                 .execution_instances.at(common::ExecutionInstanceID())
		                                 .spike_labels.at(PopulationOnExecutionInstance(0))
		                                 .at(0)
		                                 .at(CompartmentOnLogicalNeuron())
		                                 .at(0));
		assert(network_graph.get_graph_translation()
		           .execution_instances.at(common::ExecutionInstanceID())
		           .spike_labels.at(PopulationOnExecutionInstance(0))
		           .at(1)
		           .at(CompartmentOnLogicalNeuron())
		           .at(0));
		auto const spike_label_1 = *(network_graph.get_graph_translation()
		                                 .execution_instances.at(common::ExecutionInstanceID())
		                                 .spike_labels.at(PopulationOnExecutionInstance(0))
		                                 .at(1)
		                                 .at(CompartmentOnLogicalNeuron())
		                                 .at(0));
		std::vector<signal_flow::TimedSpikeToChipSequence> expectation{
		    {
		        signal_flow::TimedSpikeToChip(
		            common::Time(0),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(0),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(1),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(2),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(3),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(4),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(10),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(10),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(11),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(12),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(20),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(20),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(21),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(22),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(30),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(30),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(31),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(32),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		    },
		    {
		        signal_flow::TimedSpikeToChip(
		            common::Time(0),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(0),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(1),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(2),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(5),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(6),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(10),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(10),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(11),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(12),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(20),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(20),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(21),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(22),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(30),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(30),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(31),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(32),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		    },
		    {
		        signal_flow::TimedSpikeToChip(
		            common::Time(0),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(0),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(1),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(2),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(7),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(8),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(10),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(10),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(11),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(12),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(20),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(20),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(21),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(22),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(30),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(30),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(31),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_0}))),
		        signal_flow::TimedSpikeToChip(
		            common::Time(32),
		            signal_flow::TimedSpikeToChip::Data(SpikePack1ToChip({spike_label_1}))),
		    },
		};

		EXPECT_EQ(spikes, expectation);
	} while (std::next_permutation(range.begin(), range.end()));
}
