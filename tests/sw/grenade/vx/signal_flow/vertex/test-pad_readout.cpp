#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/neuron.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(PadReadoutView, General)
{
	PadReadoutView::Source source{
	    PadReadoutView::Source::Coord(Enum(12 /* arbitrary */)),
	    PadReadoutView::Source::Type::exc_synin};
	PadReadoutView::Coordinate coordinate;

	EXPECT_NO_THROW(PadReadoutView(source, coordinate));

	PadReadoutView vertex(source, coordinate);
	EXPECT_EQ(vertex.get_source(), source);
	EXPECT_EQ(vertex.get_coordinate(), coordinate);
	EXPECT_EQ(vertex.inputs().size(), 1);
	EXPECT_EQ(vertex.inputs().front().size, 1);
	EXPECT_EQ(vertex.output().size, 1);
	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::MembraneVoltage);
	EXPECT_EQ(vertex.output().type, ConnectionType::ExternalAnalogSignal);

	typename NeuronView::Columns columns;
	typename NeuronView::Configs configs;
	for (auto nrn : iter_all<NeuronColumnOnDLS>()) {
		columns.push_back(nrn);
		configs.push_back({NeuronView::Config::Label(0), true});
	}
	NeuronView::Row row(0);

	NeuronView neuron_view(columns, configs, row);

	EXPECT_TRUE(vertex.supports_input_from(
	    neuron_view,
	    PortRestriction(
	        12, 12 /* matching position of neuron of the recording's source in the NeuronView */)));
	EXPECT_FALSE(vertex.supports_input_from(
	    neuron_view,
	    PortRestriction(
	        12, 13 /* wrong size and non-matching neuron coordinates after restriction (13) */)));
	EXPECT_FALSE(vertex.supports_input_from(
	    neuron_view,
	    PortRestriction(
	        13,
	        13 /* not matching position of neuron of the recording's source in the NeuronView */)));
	EXPECT_FALSE(vertex.supports_input_from(neuron_view, std::nullopt));
}
