#include <gtest/gtest.h>

#include "grenade/common/edge.h"
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
#include "grenade/vx/network/abstract/multi_index_sequence_dimension_unit/mechanism_on_compartment.h"
#include "grenade/vx/network/abstract/multicompartment/compartment.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_connection.h"
#include "grenade/vx/network/abstract/multicompartment/compartment_connection/conductance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/capacitance.h"
#include "grenade/vx/network/abstract/multicompartment/mechanism/synaptic_current.h"
#include "grenade/vx/network/abstract/multicompartment/neuron.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated.h"
#include "grenade/vx/network/abstract/projection_synapse/uncalibrated_signed.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(MultiCompartmentNeuron, General)
{
	auto network = std::make_shared<grenade::common::Topology>();

	constexpr size_t size_i = 128;
	constexpr size_t size_o = 1;

	grenade::common::Population population_input{
	    abstract::ExternalSourceNeuron(),
	    grenade::common::CuboidMultiIndexSequence(
	        {size_i}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    abstract::ExternalSourceNeuron::ParameterSpace(size_i),
	    grenade::common::TimeDomainOnTopology()};

	auto population_input_descriptor = network->add_vertex(population_input);

	abstract::Neuron output_neuron;

	abstract::Compartment compartment;
	auto const mechanism_capacitance_on_compartment =
	    compartment.mechanisms.insert(abstract::MechanismCapacitance());
	auto const mechanism_synin_on_compartment =
	    compartment.mechanisms.insert(abstract::MechanismSynapticInputCurrent());

	auto const compartment_on_neuron_0 = output_neuron.add_compartment(compartment);
	auto const compartment_on_neuron_1 = output_neuron.add_compartment(compartment);

	auto const compartment_connection_on_neuron = output_neuron.add_compartment_connection(
	    compartment_on_neuron_0, compartment_on_neuron_1,
	    abstract::CompartmentConnectionConductance());

	abstract::Neuron::ParameterSpace output_neuron_parameter_space;

	abstract::Compartment::ParameterSpace compartment_parameter_space;
	compartment_parameter_space.mechanisms.set(
	    mechanism_capacitance_on_compartment,
	    abstract::MechanismCapacitance::ParameterSpace(
	        {{ccalix::CapacitanceInFarad(1.5e-12), ccalix::CapacitanceInFarad(1.5e-12)}}));
	compartment_parameter_space.mechanisms.set(
	    mechanism_synin_on_compartment, abstract::MechanismSynapticInputCurrent::ParameterSpace(
	                                        {{lola::vx::v3::AtomicNeuron::AnalogValue(500),
	                                          lola::vx::v3::AtomicNeuron::AnalogValue(500)}},
	                                        {{lola::vx::v3::AtomicNeuron::AnalogValue(600),
	                                          lola::vx::v3::AtomicNeuron::AnalogValue(600)}},
	                                        {{ccalix::TimeInS(10e-6), ccalix::TimeInS(10e-6)}}));
	output_neuron_parameter_space.compartments.emplace(
	    compartment_on_neuron_0, compartment_parameter_space);
	output_neuron_parameter_space.compartments.emplace(
	    compartment_on_neuron_1, compartment_parameter_space);

	output_neuron_parameter_space.compartment_connections.set(
	    compartment_connection_on_neuron,
	    abstract::CompartmentConnectionConductance::ParameterSpace(
	        {{ccalix::TimeInS(10e-6), ccalix::TimeInS(10e-6)}}));

	grenade::common::Population population_output{
	    output_neuron,
	    grenade::common::CuboidMultiIndexSequence(
	        {size_o}, grenade::common::MultiIndex({0}),
	        {grenade::common::CellOnPopulationDimensionUnit()}),
	    output_neuron_parameter_space, grenade::common::TimeDomainOnTopology()};

	auto population_output_descriptor = network->add_vertex(population_output);

	grenade::common::Projection projection_ih(
	    abstract::UncalibratedSynapse(),
	    abstract::UncalibratedSynapse::ParameterSpace{
	        std::vector(size_i * size_o, abstract::UncalibratedSynapse::Weight(63))},
	    grenade::common::SequenceConnector{
	        grenade::common::CuboidMultiIndexSequence(
	            {size_i}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {size_o}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {size_i, size_o}, {grenade::common::CellOnPopulationDimensionUnit(),
	                               grenade::common::CellOnPopulationDimensionUnit()})},
	    grenade::common::TimeDomainOnTopology());

	auto projection_io_descriptor = network->add_vertex(projection_ih);
	network->add_edge(
	    population_input_descriptor, projection_io_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {size_i, 1}, grenade::common::MultiIndex({0, 0}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {size_i}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        0, 0));
	network->add_edge(
	    projection_io_descriptor, population_output_descriptor,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence(
	            {size_o}, {grenade::common::CellOnPopulationDimensionUnit()}),
	        grenade::common::CuboidMultiIndexSequence(
	            {size_o, 1, 1},
	            grenade::common::MultiIndex({0, 0, mechanism_synin_on_compartment.value()}),
	            {grenade::common::CellOnPopulationDimensionUnit(),
	             grenade::common::CompartmentOnNeuronDimensionUnit(),
	             abstract::MechanismOnCompartmentDimensionUnit()}),
	        0, 0));

	grenade::vx::execution::JITGraphExecutor executor;
	abstract::FixtureCalibration calibration;
	calibration.chips[grenade::common::ExecutionInstanceOnExecutor()]
	                 [grenade::vx::common::ChipOnConnection()] = {};
	abstract::GreedyMapper mapper;
	EXPECT_NO_THROW(mapper(network, calibration, executor));
}
