#include <gtest/gtest.h>

#include "grenade/vx/network/madc_recording.h"
#include "grenade/vx/network/network_builder.h"

using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v2;
using namespace halco::common;

TEST(NetworkBuilder, General)
{
	NetworkBuilder builder;

	// pop size and spike record information have to have same size
	EXPECT_THROW(
	    builder.add(Population({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {true})),
	    std::runtime_error);

	Population population({AtomicNeuronOnDLS(Enum(0)), AtomicNeuronOnDLS(Enum(1))}, {true, false});

	auto const descriptor = builder.add(population);

	MADCRecording madc_recording;
	madc_recording.population = descriptor;
	madc_recording.index = 0;

	// population present, no MADC recording added yet
	EXPECT_NO_THROW(builder.add(madc_recording));

	// only one MADC recording per network allowed
	EXPECT_THROW(builder.add(madc_recording), std::runtime_error);

	// done resets builder
	builder.done();

	builder.add(population);
	// population present, no MADC recording added yet
	EXPECT_NO_THROW(builder.add(madc_recording));

	builder.done();

	builder.add(population);
	madc_recording.index = 2;
	// index out of range
	EXPECT_THROW(builder.add(madc_recording), std::runtime_error);

	madc_recording.index = 0;
	madc_recording.population = PopulationDescriptor(123);
	// population unknown
	EXPECT_THROW(builder.add(madc_recording), std::runtime_error);

	madc_recording.population = descriptor;
	// population present, no MADC recording added yet
	EXPECT_NO_THROW(builder.add(madc_recording));
}
