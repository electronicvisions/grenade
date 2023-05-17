#include <gtest/gtest.h>

#include "grenade/build-config.h"
#include "grenade/vx/compute/mac.h"
#include "grenade/vx/execution/backend/connection.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/signal_flow/execution_instance.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "haldls/vx/v3/systime.h"
#include "logging_ctrl.h"
#include "lola/vx/v3/chip.h"
#include "stadls/vx/v3/init_generator.h"
#include "stadls/vx/v3/playback_generator.h"
#include "stadls/vx/v3/run.h"
#include <fstream>
#include <regex>

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace stadls::vx::v3;
using namespace lola::vx::v3;

#ifdef WITH_GRENADE_PPU_SUPPORT
TEST(MAC, Single)
{
	// Construct connection to HW
	grenade::vx::execution::JITGraphExecutor executor;

	// fill graph inputs (with signal_flow::UInt5(0))
	std::vector<grenade::vx::signal_flow::UInt5> inputs(5);
	for (size_t i = 0; i < inputs.size(); ++i) {
		inputs[i] = grenade::vx::signal_flow::UInt5(i);
	}

	grenade::vx::compute::MAC::Weights const weights{
	    {grenade::vx::compute::MAC::Weight(0)}, {grenade::vx::compute::MAC::Weight(0)},
	    {grenade::vx::compute::MAC::Weight(0)}, {grenade::vx::compute::MAC::Weight(0)},
	    {grenade::vx::compute::MAC::Weight(0)},
	};

	std::unique_ptr<lola::vx::v3::Chip> chip = std::make_unique<lola::vx::v3::Chip>();

	{
		auto weights_copy = weights;
		grenade::vx::compute::MAC mac(weights_copy);

		auto const res = mac.run({inputs}, *chip, executor);
		EXPECT_EQ(res.size(), 1);
		EXPECT_EQ(res.at(0).size(), 1);
	}

	{
		grenade::vx::TemporaryDirectory dir("grenade-test-compute-mac-XXXXXX");
		auto weights_copy = weights;
		grenade::vx::compute::MAC mac(
		    weights_copy, 1, grenade::vx::common::Time(25), false, AtomicNeuronOnDLS(),
		    dir.get_path() / "madc.csv");

		mac.run({inputs}, *chip, executor);

		size_t single_linecount = 0;
		{
			std::ifstream file(dir.get_path() / "madc.csv");
			EXPECT_TRUE(file.is_open());
			std::string names;
			std::getline(file, names);
			EXPECT_EQ(names, "ExecutionIndex\tbatch\ttime\tvalue");
			for (std::string line; std::getline(file, line);) {
				EXPECT_TRUE(std::regex_match(line, std::regex("0\\t\\d+\\t\\d+\\t\\d+"))) << line;
				single_linecount++;
			}
		}

		mac.run({inputs}, *chip, executor);

		size_t second_linecount = 0;
		{
			std::ifstream file(dir.get_path() / "madc.csv");
			EXPECT_TRUE(file.is_open());
			std::string names;
			std::getline(file, names);
			EXPECT_EQ(names, "ExecutionIndex\tbatch\ttime\tvalue");
			for (std::string line; std::getline(file, line);) {
				EXPECT_TRUE(std::regex_match(line, std::regex("0\\t\\d+\\t\\d+\\t\\d+"))) << line;
				second_linecount++;
			}
		}
		EXPECT_GE(second_linecount, single_linecount);
	}
}
#endif
