#include <gtest/gtest.h>

#include "grenade/vx/vertex/cadc_readout.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(CADCMembraneReadoutView, General)
{
	CADCMembraneReadoutView::Columns columns{CADCMembraneReadoutView::Columns::value_type(0),
	                                         CADCMembraneReadoutView::Columns::value_type(1)};
	CADCMembraneReadoutView::Synram synram(0);
	EXPECT_NO_THROW(CADCMembraneReadoutView(columns, synram));
	CADCMembraneReadoutView::Columns non_unique_columns{
	    CADCMembraneReadoutView::Columns::value_type(0),
	    CADCMembraneReadoutView::Columns::value_type(0)};
	EXPECT_THROW(CADCMembraneReadoutView(non_unique_columns, synram), std::runtime_error);

	CADCMembraneReadoutView config(columns, synram);

	EXPECT_EQ(config.get_columns(), columns);
	EXPECT_EQ(config.get_synram(), synram);

	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, 2);
	EXPECT_EQ(config.output().size, 2);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::MembraneVoltage);
	EXPECT_EQ(config.output().type, ConnectionType::Int8);
}
