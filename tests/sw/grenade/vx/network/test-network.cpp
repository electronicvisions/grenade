#include <gtest/gtest.h>

#include "grenade/vx/network/cadc_recording.h"
#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include <chrono>

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(network_Network, General)
{
	Network network_a{
	    {{PopulationOnNetwork(1), ExternalSourcePopulation(2)}},
	    {{ProjectionOnNetwork(3),
	      Projection(
	          Receptor(Receptor::ID(), Receptor::Type::excitatory),
	          {Projection::Connection(
	              {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	              Projection::Connection::Weight(6))},
	          PopulationOnNetwork(1), PopulationOnNetwork(1))}},
	    MADCRecording{},
	    CADCRecording{},
	    PadRecording{},
	    {},
	    {}};

	Network network_b{
	    {{PopulationOnNetwork(2), ExternalSourcePopulation(3)}},
	    {{ProjectionOnNetwork(3),
	      Projection(
	          Receptor(Receptor::ID(), Receptor::Type::excitatory),
	          {Projection::Connection(
	              {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	              Projection::Connection::Weight(6))},
	          PopulationOnNetwork(1), PopulationOnNetwork(1))}},
	    MADCRecording{},
	    CADCRecording{},
	    PadRecording{},
	    {},
	    {}};

	Network network_c{
	    {{PopulationOnNetwork(1), ExternalSourcePopulation(2)}},
	    {{ProjectionOnNetwork(4),
	      Projection(
	          Receptor(Receptor::ID(), Receptor::Type::excitatory),
	          {Projection::Connection(
	              {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	              Projection::Connection::Weight(6))},
	          PopulationOnNetwork(1), PopulationOnNetwork(1))}},
	    MADCRecording{},
	    CADCRecording{},
	    PadRecording{},
	    {},
	    {}};

	Network network_d{
	    {{PopulationOnNetwork(1), ExternalSourcePopulation(2)}},
	    {{ProjectionOnNetwork(3),
	      Projection(
	          Receptor(Receptor::ID(), Receptor::Type::excitatory),
	          {Projection::Connection(
	              {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	              Projection::Connection::Weight(6))},
	          PopulationOnNetwork(1), PopulationOnNetwork(1))}},
	    std::nullopt,
	    CADCRecording{},
	    PadRecording{},
	    {},
	    {}};

	Network network_e{
	    {{PopulationOnNetwork(1), ExternalSourcePopulation(2)}},
	    {{ProjectionOnNetwork(3),
	      Projection(
	          Receptor(Receptor::ID(), Receptor::Type::excitatory),
	          {Projection::Connection(
	              {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	              Projection::Connection::Weight(6))},
	          PopulationOnNetwork(1), PopulationOnNetwork(1))}},
	    MADCRecording{},
	    std::nullopt,
	    PadRecording{},
	    {},
	    {}};

	PlasticityRule rule;
	rule.kernel = "abc";
	Network network_f{
	    {{PopulationOnNetwork(1), ExternalSourcePopulation(2)}},
	    {{ProjectionOnNetwork(3),
	      Projection(
	          Receptor(Receptor::ID(), Receptor::Type::excitatory),
	          {Projection::Connection(
	              {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	              Projection::Connection::Weight(6))},
	          PopulationOnNetwork(1), PopulationOnNetwork(1))}},
	    MADCRecording{},
	    CADCRecording{},
	    PadRecording{},
	    {{PlasticityRuleOnNetwork(), rule}},
	    {}};

	Network network_g{
	    {{PopulationOnNetwork(1), ExternalSourcePopulation(2)}},
	    {{ProjectionOnNetwork(3),
	      Projection(
	          Receptor(Receptor::ID(), Receptor::Type::excitatory),
	          {Projection::Connection(
	              {4, CompartmentOnLogicalNeuron()}, {5, CompartmentOnLogicalNeuron()},
	              Projection::Connection::Weight(6))},
	          PopulationOnNetwork(1), PopulationOnNetwork(1))}},
	    MADCRecording{},
	    CADCRecording{},
	    std::nullopt,
	    {},
	    {}};

	Network network_copy(network_a);
	EXPECT_EQ(network_a, network_copy);
	EXPECT_NE(network_b, network_copy);
	EXPECT_NE(network_c, network_copy);
	EXPECT_NE(network_d, network_copy);
	EXPECT_NE(network_e, network_copy);
	EXPECT_NE(network_f, network_copy);
	EXPECT_NE(network_g, network_copy);
}
