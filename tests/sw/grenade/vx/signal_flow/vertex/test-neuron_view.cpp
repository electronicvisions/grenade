#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/neuron_view.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/neuron.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(NeuronView, General)
{
	typename NeuronView::Columns columns;
	typename NeuronView::Configs configs;
	for (auto nrn : iter_all<NeuronColumnOnDLS>()) {
		columns.push_back(nrn);
		configs.push_back({NeuronView::Config::Label(0), true});
	}
	NeuronView::Row row(0);

	EXPECT_NO_THROW(NeuronView(columns, configs, row));

	// produce duplicate entry
	columns.at(1) = NeuronColumnOnDLS(0);
	EXPECT_THROW(NeuronView(columns, configs, row);, std::runtime_error);
	columns.at(1) = NeuronColumnOnDLS(1);

	NeuronView config(columns, configs, row);
	EXPECT_EQ(config.get_columns(), columns);
	EXPECT_EQ(config.get_configs(), configs);
	EXPECT_EQ(config.get_row(), row);
	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, NeuronColumnOnDLS::size);
	EXPECT_EQ(config.output().size, NeuronColumnOnDLS::size);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::SynapticInput);

	EXPECT_EQ(config.output().type, ConnectionType::MembraneVoltage);
}
