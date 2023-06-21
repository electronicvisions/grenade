#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/vertex/madc_readout.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/neuron.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(MADCReadoutView, General)
{
	{
		MADCReadoutView::Source first_source{
		    MADCReadoutView::Source::Coord(Enum(12)), MADCReadoutView::Source::Type::exc_synin};
		std::optional<MADCReadoutView::Source> second_source;
		MADCReadoutView::SourceSelection source_selection;

		EXPECT_NO_THROW(MADCReadoutView(first_source, second_source, source_selection));

		MADCReadoutView vertex(first_source, second_source, source_selection);
		EXPECT_EQ(vertex.get_first_source(), first_source);
		EXPECT_EQ(vertex.get_second_source(), second_source);
		EXPECT_EQ(vertex.get_source_selection(), source_selection);
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
	{
		MADCReadoutView::Source first_source{
		    MADCReadoutView::Source::Coord(Enum(12)), MADCReadoutView::Source::Type::exc_synin};
		std::optional<MADCReadoutView::Source> second_source;
		MADCReadoutView::SourceSelection source_selection{
		    MADCReadoutView::SourceSelection::Initial(1),
		    MADCReadoutView::SourceSelection::Period()};

		EXPECT_THROW(
		    MADCReadoutView(first_source, second_source, source_selection), std::runtime_error);
	}
	{
		MADCReadoutView::Source first_source{
		    MADCReadoutView::Source::Coord(Enum(12)), MADCReadoutView::Source::Type::exc_synin};
		std::optional<MADCReadoutView::Source> second_source;
		MADCReadoutView::SourceSelection source_selection{
		    MADCReadoutView::SourceSelection::Initial(),
		    MADCReadoutView::SourceSelection::Period(1)};

		EXPECT_THROW(
		    MADCReadoutView(first_source, second_source, source_selection), std::runtime_error);
	}
	{
		MADCReadoutView::Source first_source{
		    MADCReadoutView::Source::Coord(Enum(12)), MADCReadoutView::Source::Type::exc_synin};
		std::optional<MADCReadoutView::Source> second_source{MADCReadoutView::Source{
		    MADCReadoutView::Source::Coord(Enum(14)), MADCReadoutView::Source::Type::exc_synin}};
		MADCReadoutView::SourceSelection source_selection{
		    MADCReadoutView::SourceSelection::Initial(),
		    MADCReadoutView::SourceSelection::Period(1)};

		EXPECT_THROW(
		    MADCReadoutView(first_source, second_source, source_selection), std::runtime_error);
	}
	{
		MADCReadoutView::Source first_source{
		    MADCReadoutView::Source::Coord(Enum(11)), MADCReadoutView::Source::Type::exc_synin};
		std::optional<MADCReadoutView::Source> second_source{MADCReadoutView::Source{
		    MADCReadoutView::Source::Coord(Enum(13)), MADCReadoutView::Source::Type::exc_synin}};
		MADCReadoutView::SourceSelection source_selection{
		    MADCReadoutView::SourceSelection::Initial(),
		    MADCReadoutView::SourceSelection::Period(1)};

		EXPECT_THROW(
		    MADCReadoutView(first_source, second_source, source_selection), std::runtime_error);
	}
	{
		MADCReadoutView::Source first_source{
		    MADCReadoutView::Source::Coord(Enum(12)), MADCReadoutView::Source::Type::exc_synin};
		std::optional<MADCReadoutView::Source> second_source{MADCReadoutView::Source{
		    MADCReadoutView::Source::Coord(Enum(13)), MADCReadoutView::Source::Type::inh_synin}};
		MADCReadoutView::SourceSelection source_selection{
		    MADCReadoutView::SourceSelection::Initial(),
		    MADCReadoutView::SourceSelection::Period(1)};

		EXPECT_NO_THROW(MADCReadoutView(first_source, second_source, source_selection));

		MADCReadoutView vertex(first_source, second_source, source_selection);
		EXPECT_EQ(vertex.get_first_source(), first_source);
		EXPECT_EQ(vertex.get_second_source(), second_source);
		EXPECT_EQ(vertex.get_source_selection(), source_selection);
		EXPECT_EQ(vertex.inputs().size(), 2);
		EXPECT_EQ(vertex.inputs().front().size, 1);
		EXPECT_EQ(vertex.inputs().back().size, 1);
		EXPECT_EQ(vertex.output().size, 1);
		EXPECT_EQ(vertex.inputs().front().type, ConnectionType::MembraneVoltage);
		EXPECT_EQ(vertex.inputs().back().type, ConnectionType::MembraneVoltage);
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
		EXPECT_TRUE(vertex.supports_input_from(neuron_view, PortRestriction(12, 13)));
		EXPECT_FALSE(vertex.supports_input_from(neuron_view, PortRestriction(12, 14)));
		EXPECT_FALSE(vertex.supports_input_from(neuron_view, std::nullopt));
	}
}
