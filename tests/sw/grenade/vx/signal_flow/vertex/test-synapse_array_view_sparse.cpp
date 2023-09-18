#include <gtest/gtest.h>

#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"

using namespace lola::vx::v3;
using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace grenade::vx::signal_flow;
using namespace grenade::vx::signal_flow::vertex;

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

TEST(SynapseArrayViewSparse, toSynapseArrayView)
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

	SynapseArrayViewSparse::Synapses synapses_dense(
	    SynapseRowOnSynram::size * SynapseOnSynapseRow::size);
	for (size_t i = 0; i < SynapseRowOnSynram::size; ++i) {
		for (size_t j = 0; j < SynapseOnSynapseRow::size; ++j) {
			auto& synapse = synapses_dense.at(i * SynapseOnSynapseRow::size + j);
			synapse.index_row = i;
			synapse.index_column = j;
		}
	}

	SynapseArrayViewSparse::Synapses synapses_partial_dense(
	    (SynapseRowOnSynram::size - 1) * (SynapseOnSynapseRow::size - 1));
	for (size_t i = 0; i < SynapseRowOnSynram::size - 1; ++i) {
		for (size_t j = 0; j < SynapseOnSynapseRow::size - 1; ++j) {
			auto& synapse = synapses_partial_dense.at(i * (SynapseOnSynapseRow::size - 1) + j);
			synapse.index_row = i;
			synapse.index_column = j;
		}
	}

	SynapseArrayViewSparse::Synapses synapses_not_dense(SynapseRowOnSynram::size);
	for (size_t i = 0; i < SynapseRowOnSynram::size; ++i) {
		auto& synapse = synapses_not_dense.at(i);
		synapse.index_row = i;
		synapse.index_column = i;
	}

	EXPECT_NO_THROW(
	    SynapseArrayViewSparse(synram, rows, columns, synapses_dense).toSynapseArrayViewHandle());
	EXPECT_NO_THROW(SynapseArrayViewSparse(synram, rows, columns, synapses_partial_dense)
	                    .toSynapseArrayViewHandle());
	EXPECT_THROW(
	    SynapseArrayViewSparse(synram, rows, columns, synapses_not_dense)
	        .toSynapseArrayViewHandle(),
	    std::runtime_error);
}
