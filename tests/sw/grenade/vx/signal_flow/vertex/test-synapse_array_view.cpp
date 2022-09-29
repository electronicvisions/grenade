#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"

using namespace lola::vx::v3;
using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

TEST(SynapseArrayView, General)
{
	// construct SynapseArrayView on one vertical half with size 256 (x) * 256 (y)
	SynapseArrayView::Columns columns;
	for (auto c : iter_all<SynapseOnSynapseRow>()) {
		columns.push_back(c);
	}

	SynapseArrayView::Rows rows;
	for (auto synapse_row : iter_all<SynapseRowOnSynram>()) {
		rows.push_back(synapse_row);
	}

	SynapseArrayView::Synram synram(0);

	SynapseArrayView::Weights weights(SynapseRowOnSynram::size);
	for (auto& row : weights) {
		row.resize(SynapseOnSynapseRow::size);
	}

	SynapseArrayView::Labels labels(SynapseRowOnSynram::size);
	for (auto& row : labels) {
		row.resize(SynapseOnSynapseRow::size);
	}

	EXPECT_NO_THROW(SynapseArrayView(synram, rows, columns, weights, labels));

	SynapseArrayView vertex(synram, rows, columns, weights, labels);
	EXPECT_EQ(vertex.inputs().size(), 128);     // synapse drivers
	EXPECT_EQ(vertex.inputs().front().size, 1); // input size per synapse driver
	EXPECT_EQ(vertex.output().size, 256);       // neurons

	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::SynapseInputLabel);
	EXPECT_EQ(vertex.output().type, ConnectionType::SynapticInput);
}
