#include <gtest/gtest.h>

#include "grenade/vx/network/cadc_recording.h"
#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/network.h"
#include "grenade/vx/network/population.h"
#include "grenade/vx/network/projection.h"
#include <chrono>

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v2;
using namespace halco::common;

TEST(network_Network, General)
{
	Network network_a{
	    {{PopulationDescriptor(1), ExternalPopulation(2)}},
	    {{ProjectionDescriptor(3),
	      Projection(
	          Projection::ReceptorType::excitatory,
	          {Projection::Connection(4, 5, Projection::Connection::Weight(6))},
	          PopulationDescriptor(1), PopulationDescriptor(1))}},
	    MADCRecording{},
	    CADCRecording{},
	    {}};

	Network network_b{
	    {{PopulationDescriptor(2), ExternalPopulation(3)}},
	    {{ProjectionDescriptor(3),
	      Projection(
	          Projection::ReceptorType::excitatory,
	          {Projection::Connection(4, 5, Projection::Connection::Weight(6))},
	          PopulationDescriptor(1), PopulationDescriptor(1))}},
	    MADCRecording{},
	    CADCRecording{},
	    {}};

	Network network_c{
	    {{PopulationDescriptor(1), ExternalPopulation(2)}},
	    {{ProjectionDescriptor(4),
	      Projection(
	          Projection::ReceptorType::excitatory,
	          {Projection::Connection(4, 5, Projection::Connection::Weight(6))},
	          PopulationDescriptor(1), PopulationDescriptor(1))}},
	    MADCRecording{},
	    CADCRecording{},
	    {}};

	Network network_d{
	    {{PopulationDescriptor(1), ExternalPopulation(2)}},
	    {{ProjectionDescriptor(3),
	      Projection(
	          Projection::ReceptorType::excitatory,
	          {Projection::Connection(4, 5, Projection::Connection::Weight(6))},
	          PopulationDescriptor(1), PopulationDescriptor(1))}},
	    std::nullopt,
	    CADCRecording{},
	    {}};

	Network network_e{
	    {{PopulationDescriptor(1), ExternalPopulation(2)}},
	    {{ProjectionDescriptor(3),
	      Projection(
	          Projection::ReceptorType::excitatory,
	          {Projection::Connection(4, 5, Projection::Connection::Weight(6))},
	          PopulationDescriptor(1), PopulationDescriptor(1))}},
	    MADCRecording{},
	    std::nullopt,
	    {}};

	Network network_copy(network_a);
	EXPECT_EQ(network_a, network_copy);
	EXPECT_NE(network_b, network_copy);
	EXPECT_NE(network_c, network_copy);
	EXPECT_NE(network_d, network_copy);
	EXPECT_NE(network_e, network_copy);
}
