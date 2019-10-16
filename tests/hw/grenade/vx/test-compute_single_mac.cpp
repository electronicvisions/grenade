#include <gtest/gtest.h>

#include "grenade/vx/compute_single_mac.h"
#include "grenade/vx/config.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/chip.h"
#include "haldls/vx/systime.h"
#include "hxcomm/vx/connection_from_env.h"
#include "logging_ctrl.h"
#include "stadls/vx/init_generator.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx;
using namespace stadls::vx;
using namespace lola::vx;

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
		stadls::vx::run(connection, program);
	}

	// fill graph inputs (with UInt5(0))
	std::vector<grenade::vx::UInt5> inputs(5);
	for (size_t i = 0; i < inputs.size(); ++i) {
		inputs[i] = grenade::vx::UInt5(i);
	}

	grenade::vx::ComputeSingleMAC::Weights weights{
	    {lola::vx::SynapseMatrix::Weight(0)}, {lola::vx::SynapseMatrix::Weight(0)},
	    {lola::vx::SynapseMatrix::Weight(0)}, {lola::vx::SynapseMatrix::Weight(0)},
	    {lola::vx::SynapseMatrix::Weight(0)},
	};

	grenade::vx::ComputeSingleMAC::RowModes row_modes{
	    haldls::vx::SynapseDriverConfig::RowMode::excitatory,
	    haldls::vx::SynapseDriverConfig::RowMode::excitatory,
	    haldls::vx::SynapseDriverConfig::RowMode::excitatory,
	    haldls::vx::SynapseDriverConfig::RowMode::excitatory,
	    haldls::vx::SynapseDriverConfig::RowMode::excitatory,
	};

	std::unique_ptr<grenade::vx::ChipConfig> chip = std::make_unique<grenade::vx::ChipConfig>();

	grenade::vx::ComputeSingleMAC mac(weights, row_modes, *chip);

	auto const res = mac.run(inputs, connection);
	EXPECT_EQ(res.size(), 1);
}
