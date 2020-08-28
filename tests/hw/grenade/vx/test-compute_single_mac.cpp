#include <gtest/gtest.h>

#include "grenade/vx/compute_single_mac.h"
#include "grenade/vx/config.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "haldls/vx/v2/systime.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "stadls/vx/v2/init_generator.h"
#include "stadls/vx/v2/playback_generator.h"
#include "stadls/vx/v2/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v2;
using namespace stadls::vx::v2;
using namespace lola::vx::v2;

TEST(ComputeSingleMAC, Single)
{
	logger_default_config(log4cxx::Level::getTrace());

	// Construct connection to HW
	auto connection = hxcomm::vx::get_connection_from_env();

	// Initialize chip
	{
		DigitalInit const init;
		auto [builder, _] = generate(init);
		auto program = builder.done();
		stadls::vx::v2::run(connection, program);
	}

	// fill graph inputs (with UInt5(0))
	std::vector<grenade::vx::UInt5> inputs(5);
	for (size_t i = 0; i < inputs.size(); ++i) {
		inputs[i] = grenade::vx::UInt5(i);
	}

	grenade::vx::ComputeSingleMAC::Weights weights{
	    {grenade::vx::ComputeSingleMAC::Weight(0)}, {grenade::vx::ComputeSingleMAC::Weight(0)},
	    {grenade::vx::ComputeSingleMAC::Weight(0)}, {grenade::vx::ComputeSingleMAC::Weight(0)},
	    {grenade::vx::ComputeSingleMAC::Weight(0)},
	};

	std::unique_ptr<grenade::vx::ChipConfig> chip = std::make_unique<grenade::vx::ChipConfig>();

	grenade::vx::ComputeSingleMAC mac(weights);

	auto const res = mac.run({inputs}, *chip, connection);
	EXPECT_EQ(res.size(), 1);
	EXPECT_EQ(res.at(0).size(), 1);
}
