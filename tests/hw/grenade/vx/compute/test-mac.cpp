#include <gtest/gtest.h>

#include "grenade/vx/backend/connection.h"
#include "grenade/vx/compute/mac.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "haldls/vx/v2/systime.h"
#include "logging_ctrl.h"
#include "lola/vx/v2/chip.h"
#include "stadls/vx/v2/init_generator.h"
#include "stadls/vx/v2/playback_generator.h"
#include "stadls/vx/v2/run.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v2;
using namespace stadls::vx::v2;
using namespace lola::vx::v2;

#ifdef WITH_GRENADE_PPU_SUPPORT
TEST(MAC, Single)
{
	// Construct connection to HW
	grenade::vx::backend::Connection connection;

	// fill graph inputs (with UInt5(0))
	std::vector<grenade::vx::UInt5> inputs(5);
	for (size_t i = 0; i < inputs.size(); ++i) {
		inputs[i] = grenade::vx::UInt5(i);
	}

	grenade::vx::compute::MAC::Weights weights{
	    {grenade::vx::compute::MAC::Weight(0)}, {grenade::vx::compute::MAC::Weight(0)},
	    {grenade::vx::compute::MAC::Weight(0)}, {grenade::vx::compute::MAC::Weight(0)},
	    {grenade::vx::compute::MAC::Weight(0)},
	};

	std::unique_ptr<lola::vx::v2::Chip> chip = std::make_unique<lola::vx::v2::Chip>();

	grenade::vx::compute::MAC mac(weights);

	auto const res = mac.run({inputs}, *chip, connection);
	EXPECT_EQ(res.size(), 1);
	EXPECT_EQ(res.at(0).size(), 1);
}
#endif
