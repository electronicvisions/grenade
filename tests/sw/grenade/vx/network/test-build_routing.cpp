#include <gtest/gtest.h>

#include "grenade/common/edge.h"
#include "grenade/common/inter_topology_hyper_edge/fixture.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/population.h"
#include "grenade/common/projection.h"
#include "grenade/common/projection_connector/sequence.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/topology.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/network/abstract/calibration/fixture.h"
#include "grenade/vx/network/abstract/mapper/greedy.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(build_routing, ProjectionOverlap)
{
	auto topology = std::make_shared<grenade::common::Topology>();

	grenade::common::Population population{
	    abstract::UncalibratedNeuron{
	        abstract::UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             abstract::UncalibratedNeuron::Compartment{
	                 abstract::UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0), Receptor::Type::excitatory},
	                   {grenade::common::ReceptorOnCompartment(1), Receptor::Type::inhibitory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {2}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    abstract::UncalibratedNeuron::ParameterSpace(
	        2, {{grenade::common::CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};

	grenade::common::Population external_population{
	    abstract::ExternalSourceNeuron{},
	    grenade::common::CuboidMultiIndexSequence(
	        {2}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    abstract::ExternalSourceNeuron::ParameterSpace(2), grenade::common::TimeDomainOnTopology()};

	// no overlap
	auto descriptor = topology->add_vertex(population);

	grenade::common::Projection projection_1(
	    abstract::UncalibratedSynapse{},
	    abstract::UncalibratedSynapse::ParameterSpace{{abstract::UncalibratedSynapse::Weight(63)}},
	    grenade::common::SequenceConnector{
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1, 1})},
	    grenade::common::TimeDomainOnTopology());

	grenade::common::Projection projection_2(
	    abstract::UncalibratedSynapse{},
	    abstract::UncalibratedSynapse::ParameterSpace{{abstract::UncalibratedSynapse::Weight(63)}},
	    grenade::common::SequenceConnector{
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1, 1})},
	    grenade::common::TimeDomainOnTopology());

	auto projection_1_descriptor = topology->add_vertex(projection_1);
	topology->add_edge(
	    descriptor, projection_1_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
	topology->add_edge(
	    projection_1_descriptor, descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	auto projection_2_descriptor = topology->add_vertex(projection_2);
	topology->add_edge(
	    descriptor, projection_2_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
	topology->add_edge(
	    projection_2_descriptor, descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	std::map<
	    grenade::common::ConnectionOnExecutor, grenade::vx::execution::backend::StatefulConnection>
	    connections;
	connections.emplace(
	    grenade::common::ConnectionOnExecutor(),
	    grenade::vx::execution::backend::StatefulConnection(
	        grenade::vx::execution::backend::InitializedConnection(
	            hxcomm::MultiConnection<hxcomm::vx::ZeroMockConnection>()),
	        {{true}}));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections));
	abstract::FixtureCalibration calibration;
	EXPECT_NO_THROW(abstract::GreedyMapper()(topology, calibration, executor));

	topology->clear_vertex(projection_2_descriptor);
	topology->remove_vertex(projection_2_descriptor);

	// same projection
	auto projection_3_descriptor = topology->add_vertex(projection_1);
	topology->add_edge(
	    descriptor, projection_3_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
	topology->add_edge(
	    projection_3_descriptor, descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	EXPECT_NO_THROW(abstract::GreedyMapper()(topology, calibration, executor));

	// overlap
	topology->clear_vertex(projection_3_descriptor);
	topology->remove_vertex(projection_3_descriptor);

	grenade::common::Projection projection_4(
	    abstract::UncalibratedSynapse{},
	    abstract::UncalibratedSynapse::ParameterSpace{
	        {abstract::UncalibratedSynapse::Weight(63), abstract::UncalibratedSynapse::Weight(63)}},
	    grenade::common::SequenceConnector{
	        grenade::common::CuboidMultiIndexSequence({2}),
	        grenade::common::CuboidMultiIndexSequence({2}),
	        grenade::common::ListMultiIndexSequence(
	            {grenade::common::MultiIndex({0, 0}), grenade::common::MultiIndex({1, 1})})},
	    grenade::common::TimeDomainOnTopology());
	auto projection_4_descriptor = topology->add_vertex(projection_4);
	topology->add_edge(
	    descriptor, projection_4_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {2, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence({2}), 0, 0));
	topology->add_edge(
	    projection_4_descriptor, descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({2}),
	        grenade::common::CuboidMultiIndexSequence(
	            {2, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	EXPECT_NO_THROW(abstract::GreedyMapper()(topology, calibration, executor));

	// from external no overlap
	topology->clear_vertex(projection_1_descriptor);
	topology->remove_vertex(projection_1_descriptor);
	topology->clear_vertex(projection_4_descriptor);
	topology->remove_vertex(projection_4_descriptor);
	auto input_descriptor = topology->add_vertex(external_population);

	projection_1_descriptor = topology->add_vertex(projection_1);
	topology->add_edge(
	    input_descriptor, projection_1_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
	topology->add_edge(
	    projection_1_descriptor, descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	projection_2_descriptor = topology->add_vertex(projection_2);
	topology->add_edge(
	    input_descriptor, projection_2_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
	topology->add_edge(
	    projection_2_descriptor, descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	EXPECT_NO_THROW(abstract::GreedyMapper()(topology, calibration, executor));

	topology->clear_vertex(projection_2_descriptor);
	topology->remove_vertex(projection_2_descriptor);

	// same projection
	projection_3_descriptor = topology->add_vertex(projection_1);
	topology->add_edge(
	    input_descriptor, projection_3_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
	topology->add_edge(
	    projection_3_descriptor, descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	EXPECT_NO_THROW(abstract::GreedyMapper()(topology, calibration, executor));

	// overlap
	topology->clear_vertex(projection_3_descriptor);
	topology->remove_vertex(projection_3_descriptor);

	projection_4_descriptor = topology->add_vertex(projection_4);
	topology->add_edge(
	    input_descriptor, projection_4_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {2, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence({2}), 0, 0));
	topology->add_edge(
	    projection_4_descriptor, descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({2}),
	        grenade::common::CuboidMultiIndexSequence(
	            {2, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	EXPECT_NO_THROW(abstract::GreedyMapper()(topology, calibration, executor));
}

TEST(build_routing, EmptyProjection)
{
	auto topology = std::make_shared<grenade::common::Topology>();

	grenade::common::Population population{
	    abstract::UncalibratedNeuron{
	        abstract::UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             abstract::UncalibratedNeuron::Compartment{
	                 abstract::UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0), Receptor::Type::excitatory},
	                   {grenade::common::ReceptorOnCompartment(1), Receptor::Type::inhibitory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {1}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    abstract::UncalibratedNeuron::ParameterSpace(
	        1, {{grenade::common::CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};

	auto population_descriptor = topology->add_vertex(population);

	grenade::common::Projection projection_1(
	    abstract::UncalibratedSynapse{}, abstract::UncalibratedSynapse::ParameterSpace{{}},
	    grenade::common::SequenceConnector{
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence({0, 0})},
	    grenade::common::TimeDomainOnTopology());

	auto projection_1_descriptor = topology->add_vertex(projection_1);
	topology->add_edge(
	    population_descriptor, projection_1_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
	topology->add_edge(
	    projection_1_descriptor, population_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({1}),
	        grenade::common::CuboidMultiIndexSequence(
	            {1, 1, 1}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	std::map<
	    grenade::common::ConnectionOnExecutor, grenade::vx::execution::backend::StatefulConnection>
	    connections;
	connections.emplace(
	    grenade::common::ConnectionOnExecutor(),
	    grenade::vx::execution::backend::StatefulConnection(
	        grenade::vx::execution::backend::InitializedConnection(
	            hxcomm::MultiConnection<hxcomm::vx::ZeroMockConnection>()),
	        {{true}}));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections));
	abstract::FixtureCalibration calibration;
	EXPECT_NO_THROW(abstract::GreedyMapper()(topology, calibration, executor));
}

TEST(build_routing, I100H64O3)
{
	auto topology = std::make_shared<grenade::common::Topology>();

	grenade::common::Population population_input{
	    abstract::ExternalSourceNeuron{},
	    grenade::common::CuboidMultiIndexSequence(
	        {100}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    abstract::ExternalSourceNeuron::ParameterSpace(100),
	    grenade::common::TimeDomainOnTopology()};

	auto population_input_descriptor = topology->add_vertex(population_input);

	grenade::common::Population population_hidden{
	    abstract::UncalibratedNeuron{
	        abstract::UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             abstract::UncalibratedNeuron::Compartment{
	                 abstract::UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0), Receptor::Type::excitatory},
	                   {grenade::common::ReceptorOnCompartment(1), Receptor::Type::inhibitory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {64}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    abstract::UncalibratedNeuron::ParameterSpace(
	        64, {{grenade::common::CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};

	auto population_hidden_descriptor = topology->add_vertex(population_hidden);

	grenade::common::Population population_output{
	    abstract::UncalibratedNeuron{
	        abstract::UncalibratedNeuron::Compartments{
	            {grenade::common::CompartmentOnNeuron(),
	             abstract::UncalibratedNeuron::Compartment{
	                 abstract::UncalibratedNeuron::Compartment::SpikeMaster(0),
	                 {{{grenade::common::ReceptorOnCompartment(0), Receptor::Type::excitatory},
	                   {grenade::common::ReceptorOnCompartment(1), Receptor::Type::inhibitory}}}}}},
	        LogicalNeuronCompartments(
	            {{CompartmentOnLogicalNeuron(), {AtomicNeuronOnLogicalNeuron()}}})},
	    grenade::common::CuboidMultiIndexSequence(
	        {3}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    abstract::UncalibratedNeuron::ParameterSpace(
	        3, {{grenade::common::CompartmentOnNeuron(), 1}}),
	    grenade::common::TimeDomainOnTopology()};

	auto population_output_descriptor = topology->add_vertex(population_output);

	grenade::common::Projection projection_ih(
	    abstract::UncalibratedSignedSynapse{
	        grenade::common::ReceptorOnCompartment(0), grenade::common::ReceptorOnCompartment(1)},
	    abstract::UncalibratedSignedSynapse::ParameterSpace{
	        std::vector(100 * 64, abstract::UncalibratedSignedSynapse::Weight(63))},
	    grenade::common::SequenceConnector{
	        grenade::common::CuboidMultiIndexSequence(
	            {100}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {64}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {100, 64}, {grenade::common::CellOnPopulationDimensionUnit(),
	                        grenade::common::CellOnPopulationDimensionUnit()})},
	    grenade::common::TimeDomainOnTopology());

	grenade::common::Projection projection_ho(
	    abstract::UncalibratedSignedSynapse{
	        grenade::common::ReceptorOnCompartment(0), grenade::common::ReceptorOnCompartment(1)},
	    abstract::UncalibratedSignedSynapse::ParameterSpace{
	        std::vector(64 * 3, abstract::UncalibratedSignedSynapse::Weight(63))},
	    grenade::common::SequenceConnector{
	        grenade::common::CuboidMultiIndexSequence(
	            {64}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {3}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {64, 3}, {grenade::common::CellOnPopulationDimensionUnit(),
	                      grenade::common::CellOnPopulationDimensionUnit()})},
	    grenade::common::TimeDomainOnTopology());

	auto projection_ih_descriptor = topology->add_vertex(projection_ih);
	topology->add_edge(
	    population_input_descriptor, projection_ih_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {100, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {100}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        0, 0));
	topology->add_edge(
	    projection_ih_descriptor, population_hidden_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {64, 2}, {grenade::common::CellOnPopulationDimensionUnit(),
	                      grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {64, 1, 2}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	auto projection_ho_descriptor = topology->add_vertex(projection_ho);
	topology->add_edge(
	    population_hidden_descriptor, projection_ho_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {64, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {64}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        0, 0));
	topology->add_edge(
	    projection_ho_descriptor, population_output_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {3, 2}, {grenade::common::CellOnPopulationDimensionUnit(),
	                     grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {3, 1, 2}, grenade::common::MultiIndex({0, 0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             grenade::common::ReceptorOnCompartmentDimensionUnit()}),
	        0, 0));

	std::map<
	    grenade::common::ConnectionOnExecutor, grenade::vx::execution::backend::StatefulConnection>
	    connections;
	connections.emplace(
	    grenade::common::ConnectionOnExecutor(),
	    grenade::vx::execution::backend::StatefulConnection(
	        grenade::vx::execution::backend::InitializedConnection(
	            hxcomm::MultiConnection<hxcomm::vx::ZeroMockConnection>()),
	        {{true}}));
	grenade::vx::execution::JITGraphExecutor executor(std::move(connections));
	abstract::FixtureCalibration calibration;
	abstract::GreedyMapper mapper;
	std::vector<AtomicNeuronOnDLS> neuron_permutation;
	for (size_t i = 0; i < 64; ++i) {
		neuron_permutation.push_back(AtomicNeuronOnDLS(Enum(i)));
	}
	for (size_t i = 256; i < 259; ++i) {
		neuron_permutation.push_back(AtomicNeuronOnDLS(Enum(i)));
	}
	mapper.set_neuron_permutation(neuron_permutation);
	EXPECT_NO_THROW(mapper(topology, calibration, executor));
}
