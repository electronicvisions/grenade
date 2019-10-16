#include <gtest/gtest.h>

#include "grenade/vx/vertex/synapse_array_view.h"

using namespace lola::vx;
using namespace haldls::vx;
using namespace halco::common;
using namespace halco::hicann_dls::vx;
using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(SynapseArrayView, General)
{
	// construct SynapseArrayView on one vertical half with size 256 (x) * 256 (y)
	SynapseArrayView::synapse_rows_type synapse_rows;
	// fill synapse array view
	for (auto synapse_row : iter_all<SynapseRowOnSynram>()) {
		// all synapse rows
		synapse_rows.push_back(SynapseArrayView::SynapseRow());
		synapse_rows.back().coordinate = synapse_row;
		synapse_rows.back().weights = std::vector<SynapseRow::Weight>(123);
		// even rows inhibitory, odd rows excitatory
		synapse_rows.back().mode = synapse_row.toEnum() % 2
		                               ? SynapseDriverConfig::RowMode::excitatory
		                               : SynapseDriverConfig::RowMode::inhibitory;
	}

	EXPECT_NO_THROW(auto _ = SynapseArrayView(synapse_rows););

	// weight shape not rectangular
	synapse_rows.back().weights.clear();
	EXPECT_THROW(auto _ = SynapseArrayView(synapse_rows);, std::runtime_error);
	synapse_rows.back().weights = std::vector<SynapseRow::Weight>(123);

	SynapseArrayView vertex(synapse_rows);
	EXPECT_EQ(vertex.inputs().size(), 1);
	EXPECT_EQ(vertex.inputs().front().size, 256);
	EXPECT_EQ(vertex.output().size, 123);

	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::SynapseInputLabel);
	EXPECT_EQ(vertex.output().type, ConnectionType::SynapticInput);
}
