#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"

using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(CADCMembraneReadoutView, General)
{
	auto const mode = CADCMembraneReadoutView::Mode::hagen;
	CADCMembraneReadoutView::Columns columns{
	    {CADCMembraneReadoutView::Columns::value_type::value_type(0),
	     CADCMembraneReadoutView::Columns::value_type::value_type(1)},
	    {CADCMembraneReadoutView::Columns::value_type::value_type(2)}};
	CADCMembraneReadoutView::Sources sources{
	    {CADCMembraneReadoutView::Sources::value_type::value_type::adaptation,
	     CADCMembraneReadoutView::Sources::value_type::value_type::adaptation},
	    {CADCMembraneReadoutView::Sources::value_type::value_type::adaptation}};
	CADCMembraneReadoutView::Synram synram(0);
	EXPECT_NO_THROW(CADCMembraneReadoutView(columns, synram, mode, sources));
	CADCMembraneReadoutView::Columns non_unique_columns{
	    {CADCMembraneReadoutView::Columns::value_type::value_type(0),
	     CADCMembraneReadoutView::Columns::value_type::value_type(0)}};
	EXPECT_THROW(
	    CADCMembraneReadoutView(non_unique_columns, synram, mode, sources), std::runtime_error);
	CADCMembraneReadoutView::Columns non_unique_column_collections{
	    {CADCMembraneReadoutView::Columns::value_type::value_type(0),
	     CADCMembraneReadoutView::Columns::value_type::value_type(1)},
	    {CADCMembraneReadoutView::Columns::value_type::value_type(0)}};
	EXPECT_THROW(
	    CADCMembraneReadoutView(non_unique_column_collections, synram, mode, sources),
	    std::runtime_error);
	CADCMembraneReadoutView::Sources non_matching_outer_sources{
	    {CADCMembraneReadoutView::Sources::value_type::value_type::adaptation,
	     CADCMembraneReadoutView::Sources::value_type::value_type::adaptation}};
	EXPECT_THROW(
	    CADCMembraneReadoutView(columns, synram, mode, non_matching_outer_sources),
	    std::runtime_error);
	CADCMembraneReadoutView::Sources non_matching_inner_sources{
	    {CADCMembraneReadoutView::Sources::value_type::value_type::adaptation},
	    {CADCMembraneReadoutView::Sources::value_type::value_type::adaptation}};
	EXPECT_THROW(
	    CADCMembraneReadoutView(columns, synram, mode, non_matching_inner_sources),
	    std::runtime_error);

	CADCMembraneReadoutView config(columns, synram, mode, sources);

	EXPECT_EQ(config.get_columns(), columns);
	EXPECT_EQ(config.get_synram(), synram);
	EXPECT_EQ(config.get_sources(), sources);

	EXPECT_EQ(config.inputs().size(), 2);
	EXPECT_EQ(config.inputs().at(0).size, 2);
	EXPECT_EQ(config.inputs().at(1).size, 1);
	EXPECT_EQ(config.output().size, 3);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::MembraneVoltage);
	EXPECT_EQ(config.output().type, ConnectionType::Int8);
}
