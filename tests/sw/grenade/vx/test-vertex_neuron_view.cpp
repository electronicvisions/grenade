#include <gtest/gtest.h>

#include "grenade/vx/vertex/neuron_view.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v1/neuron.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v1;
using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(NeuronView, General)
{
	typename NeuronView::Columns columns;
	for (auto nrn : iter_all<NeuronColumnOnDLS>()) {
		columns.push_back(nrn);
	}
	NeuronView::Row row(0);

	EXPECT_NO_THROW(NeuronView(columns, row));

	// produce duplicate entry
	columns.at(1) = NeuronColumnOnDLS(0);
	EXPECT_THROW(NeuronView(columns, row);, std::runtime_error);
	columns.at(1) = NeuronColumnOnDLS(1);

	NeuronView config(columns, row);
	EXPECT_EQ(config.get_columns(), columns);
	EXPECT_EQ(config.get_row(), row);
	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, NeuronColumnOnDLS::size);
	EXPECT_EQ(config.output().size, NeuronColumnOnDLS::size);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::SynapticInput);

	EXPECT_EQ(config.output().type, ConnectionType::MembraneVoltage);
}
