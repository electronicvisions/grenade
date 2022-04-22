#include <gtest/gtest.h>

#include "grenade/vx/vertex/cadc_membrane_readout_view.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(CADCMembraneReadoutView, General)
{
	auto const mode = CADCMembraneReadoutView::Mode::hagen;
	CADCMembraneReadoutView::Columns columns{
	    {CADCMembraneReadoutView::Columns::value_type::value_type(0),
	     CADCMembraneReadoutView::Columns::value_type::value_type(1)},
	    {CADCMembraneReadoutView::Columns::value_type::value_type(2)}};
	CADCMembraneReadoutView::Synram synram(0);
	EXPECT_NO_THROW(CADCMembraneReadoutView(columns, synram, mode));
	CADCMembraneReadoutView::Columns non_unique_columns{
	    {CADCMembraneReadoutView::Columns::value_type::value_type(0),
	     CADCMembraneReadoutView::Columns::value_type::value_type(0)}};
	EXPECT_THROW(CADCMembraneReadoutView(non_unique_columns, synram, mode), std::runtime_error);
	CADCMembraneReadoutView::Columns non_unique_column_collections{
	    {CADCMembraneReadoutView::Columns::value_type::value_type(0),
	     CADCMembraneReadoutView::Columns::value_type::value_type(1)},
	    {CADCMembraneReadoutView::Columns::value_type::value_type(0)}};
	EXPECT_THROW(
	    CADCMembraneReadoutView(non_unique_column_collections, synram, mode), std::runtime_error);

	CADCMembraneReadoutView config(columns, synram, mode);

	EXPECT_EQ(config.get_columns(), columns);
	EXPECT_EQ(config.get_synram(), synram);

	EXPECT_EQ(config.inputs().size(), 2);
	EXPECT_EQ(config.inputs().at(0).size, 2);
	EXPECT_EQ(config.inputs().at(1).size, 1);
	EXPECT_EQ(config.output().size, 3);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::MembraneVoltage);
	EXPECT_EQ(config.output().type, ConnectionType::Int8);
}
