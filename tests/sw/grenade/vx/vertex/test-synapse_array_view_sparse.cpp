#include <gtest/gtest.h>

#include "grenade/vx/vertex/synapse_array_view_sparse.h"

using namespace lola::vx::v2;
using namespace halco::common;
using namespace halco::hicann_dls::vx::v2;
using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(SynapseArrayViewSparse, General)
{
	// construct SynapseArrayViewSparse on one vertical half with size 256 (x) * 256 (y)
	SynapseArrayViewSparse::Columns columns;
	for (auto c : iter_all<SynapseOnSynapseRow>()) {
		columns.push_back(c);
	}

	SynapseArrayViewSparse::Rows rows;
	for (auto synapse_row : iter_all<SynapseRowOnSynram>()) {
		rows.push_back(synapse_row);
	}

	SynapseArrayViewSparse::Synram synram(0);

	SynapseArrayViewSparse::Synapses synapses(SynapseRowOnSynram::size * SynapseOnSynapseRow::size);
	for (size_t i = 0; i < SynapseRowOnSynram::size; ++i) {
		for (size_t j = 0; j < SynapseOnSynapseRow::size; ++j) {
			auto& synapse = synapses.at(i * SynapseOnSynapseRow::size + j);
			synapse.index_row = i;
			synapse.index_column = j;
		}
	}

	EXPECT_NO_THROW(SynapseArrayViewSparse(synram, rows, columns, synapses));

	SynapseArrayViewSparse vertex(synram, rows, columns, synapses);
	EXPECT_EQ(vertex.inputs().size(), 128);     // synapse drivers
	EXPECT_EQ(vertex.inputs().front().size, 1); // input size per synapse driver
	EXPECT_EQ(vertex.output().size, 256);       // neurons

	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::SynapseInputLabel);
	EXPECT_EQ(vertex.output().type, ConnectionType::SynapticInput);
}
