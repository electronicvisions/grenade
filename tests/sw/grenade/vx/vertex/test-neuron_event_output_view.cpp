#include <gtest/gtest.h>

#include "grenade/vx/vertex/neuron_event_output_view.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v2/neuron.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v2;
using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(NeuronEventOutputView, General)
{
	typename NeuronEventOutputView::Columns columns;
	for (auto nrn : iter_all<NeuronColumnOnDLS>()) {
		columns.push_back(nrn);
	}
	NeuronEventOutputView::Row row(0);
	NeuronEventOutputView::Neurons neurons{{row, {columns}}};

	EXPECT_NO_THROW(NeuronEventOutputView view(neurons));

	// produce duplicate entry
	neurons.at(row).at(0).at(1) = NeuronColumnOnDLS(0);
	EXPECT_THROW(NeuronEventOutputView view(neurons), std::runtime_error);
	neurons.at(row).at(0).at(1) = NeuronColumnOnDLS(1);

	// produce duplicate neuron in different groups
	neurons.at(row).push_back(columns);
	EXPECT_THROW(NeuronEventOutputView view(neurons), std::runtime_error);
	neurons.at(row).pop_back();

	NeuronEventOutputView config(neurons);
	EXPECT_EQ(config.get_neurons(), neurons);
	EXPECT_EQ(config.inputs().size(), 1);
	EXPECT_EQ(config.inputs().front().size, NeuronColumnOnDLS::size);
	EXPECT_EQ(config.output().size, NeuronEventOutputOnDLS::size);

	EXPECT_EQ(config.inputs().front().type, ConnectionType::MembraneVoltage);

	EXPECT_EQ(config.output().type, ConnectionType::CrossbarInputLabel);
}
