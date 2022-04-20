#include <gtest/gtest.h>

#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/madc_readout.h"
#include "grenade/vx/vertex/neuron_view.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/neuron.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(MADCReadoutView, General)
{
	typename MADCReadoutView::Coord coord(Enum(12));
	typename MADCReadoutView::Config config(MADCReadoutView::Config::exc_synin);

	EXPECT_NO_THROW(MADCReadoutView(coord, config));

	MADCReadoutView vertex(coord, config);
	EXPECT_EQ(vertex.get_coord(), coord);
	EXPECT_EQ(vertex.get_config(), config);
	EXPECT_EQ(vertex.inputs().size(), 1);
	EXPECT_EQ(vertex.inputs().front().size, 1);
	EXPECT_EQ(vertex.output().size, 1);
	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::MembraneVoltage);
	EXPECT_EQ(vertex.output().type, ConnectionType::TimedMADCSampleFromChipSequence);

	typename NeuronView::Columns columns;
	typename NeuronView::Configs configs;
	for (auto nrn : iter_all<NeuronColumnOnDLS>()) {
		columns.push_back(nrn);
		configs.push_back({NeuronView::Config::Label(0), true});
	}
	NeuronView::Row row(0);

	NeuronView neuron_view(columns, configs, row);

	EXPECT_TRUE(vertex.supports_input_from(neuron_view, PortRestriction(12, 12)));
	EXPECT_FALSE(vertex.supports_input_from(neuron_view, PortRestriction(12, 13)));
	EXPECT_FALSE(vertex.supports_input_from(neuron_view, std::nullopt));
}
