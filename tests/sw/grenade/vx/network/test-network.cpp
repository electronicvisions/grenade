#include <gtest/gtest.h>

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/network/cadc_recording.h"
#include "grenade/vx/network/external_source_population.h"
#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include <chrono>

using namespace grenade::vx::network;
using namespace grenade::common;
using namespace grenade::vx::common;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(network_Network, General)
{
	Network network_a{
	    {{ExecutionInstanceID(),
	      {{{PopulationOnExecutionInstance(1),
	         ExternalSourcePopulation(
	             {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()})}},
	       {{ProjectionOnExecutionInstance(3),
	         Projection(
	             Receptor(Receptor::ID(), Receptor::Type::excitatory),
	             {Projection::Connection(
	                 {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	                 Projection::Connection::Weight(6))},
	             PopulationOnExecutionInstance(1), PopulationOnExecutionInstance(1))}},
	       MADCRecording{},
	       CADCRecording{},
	       PadRecording{},
	       {}}}},
	    {},
	    {ExecutionInstanceID()},
	    {}};

	Network network_b{
	    {{ExecutionInstanceID(),
	      {{{PopulationOnExecutionInstance(2),
	         ExternalSourcePopulation(
	             {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron(),
	              ExternalSourcePopulation::Neuron()})}},
	       {{ProjectionOnExecutionInstance(3),
	         Projection(
	             Receptor(Receptor::ID(), Receptor::Type::excitatory),
	             {Projection::Connection(
	                 {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	                 Projection::Connection::Weight(6))},
	             PopulationOnExecutionInstance(1), PopulationOnExecutionInstance(1))}},
	       MADCRecording{},
	       CADCRecording{},
	       PadRecording{},
	       {}}}},
	    {},
	    {ExecutionInstanceID()},
	    {}};

	Network network_c{
	    {{ExecutionInstanceID(),
	      {{{PopulationOnExecutionInstance(1),
	         ExternalSourcePopulation(
	             {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()})}},
	       {{ProjectionOnExecutionInstance(4),
	         Projection(
	             Receptor(Receptor::ID(), Receptor::Type::excitatory),
	             {Projection::Connection(
	                 {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	                 Projection::Connection::Weight(6))},
	             PopulationOnExecutionInstance(1), PopulationOnExecutionInstance(1))}},
	       MADCRecording{},
	       CADCRecording{},
	       PadRecording{},
	       {}}}},
	    {},
	    {ExecutionInstanceID()},
	    {}};

	Network network_d{
	    {{ExecutionInstanceID(),
	      {{{PopulationOnExecutionInstance(1),
	         ExternalSourcePopulation(
	             {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()})}},
	       {{ProjectionOnExecutionInstance(3),
	         Projection(
	             Receptor(Receptor::ID(), Receptor::Type::excitatory),
	             {Projection::Connection(
	                 {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	                 Projection::Connection::Weight(6))},
	             PopulationOnExecutionInstance(1), PopulationOnExecutionInstance(1))}},
	       std::nullopt,
	       CADCRecording{},
	       PadRecording{},
	       {}}}},
	    {},
	    {ExecutionInstanceID()},
	    {}};

	Network network_e{
	    {{ExecutionInstanceID(),
	      {{{PopulationOnExecutionInstance(1),
	         ExternalSourcePopulation(
	             {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()})}},
	       {{ProjectionOnExecutionInstance(3),
	         Projection(
	             Receptor(Receptor::ID(), Receptor::Type::excitatory),
	             {Projection::Connection(
	                 {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	                 Projection::Connection::Weight(6))},
	             PopulationOnExecutionInstance(1), PopulationOnExecutionInstance(1))}},
	       MADCRecording{},
	       std::nullopt,
	       PadRecording{},
	       {}}}},
	    {},
	    {ExecutionInstanceID()},
	    {}};

	PlasticityRule rule;
	rule.kernel = "abc";
	Network network_f{
	    {{ExecutionInstanceID(),
	      {{{PopulationOnExecutionInstance(1),
	         ExternalSourcePopulation(
	             {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()})}},
	       {{ProjectionOnExecutionInstance(3),
	         Projection(
	             Receptor(Receptor::ID(), Receptor::Type::excitatory),
	             {Projection::Connection(
	                 {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	                 Projection::Connection::Weight(6))},
	             PopulationOnExecutionInstance(1), PopulationOnExecutionInstance(1))}},
	       MADCRecording{},
	       CADCRecording{},
	       PadRecording{},
	       {{PlasticityRuleOnExecutionInstance(), rule}}}}},
	    {},
	    {ExecutionInstanceID()},
	    {}};

	Network network_g{
	    {{ExecutionInstanceID(1),
	      {{{PopulationOnExecutionInstance(1),
	         ExternalSourcePopulation(
	             {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()})}},
	       {{ProjectionOnExecutionInstance(3),
	         Projection(
	             Receptor(Receptor::ID(), Receptor::Type::excitatory),
	             {Projection::Connection(
	                 {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	                 Projection::Connection::Weight(6))},
	             PopulationOnExecutionInstance(1), PopulationOnExecutionInstance(1))}},
	       MADCRecording{},
	       CADCRecording{},
	       PadRecording{},
	       {}}}},
	    {},
	    {ExecutionInstanceID()},
	    {}};

	Network network_h{
	    {{ExecutionInstanceID(1),
	      {{{PopulationOnExecutionInstance(1),
	         ExternalSourcePopulation(
	             {ExternalSourcePopulation::Neuron(), ExternalSourcePopulation::Neuron()})}},
	       {{ProjectionOnExecutionInstance(3),
	         Projection(
	             Receptor(Receptor::ID(), Receptor::Type::excitatory),
	             {Projection::Connection(
	                 {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	                 Projection::Connection::Weight(6))},
	             PopulationOnExecutionInstance(1), PopulationOnExecutionInstance(1))}},
	       MADCRecording{},
	       CADCRecording{},
	       std::nullopt,
	       {}}}},
	    {},
	    {ExecutionInstanceID(1)},
	    {}};

	Network network_copy(network_a);
	EXPECT_EQ(network_a, network_copy);
	EXPECT_NE(network_b, network_copy);
	EXPECT_NE(network_c, network_copy);
	EXPECT_NE(network_d, network_copy);
	EXPECT_NE(network_e, network_copy);
	EXPECT_NE(network_f, network_copy);
	EXPECT_NE(network_g, network_copy);
	EXPECT_NE(network_h, network_copy);
}
