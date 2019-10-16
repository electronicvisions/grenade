#include <gtest/gtest.h>

#include "grenade/vx/vertex/neuron_view.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/neuron.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx;
using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(NeuronView, General)
{
	typename NeuronView::neurons_type nrns;
	for (auto nrn : iter_all<NeuronColumnOnDLS>()) {
		nrns.push_back(nrn);
	}

	EXPECT_NO_THROW(auto _ = NeuronView(nrns););

	// produce duplicate entry
	nrns.at(1) = NeuronColumnOnDLS(0);
	EXPECT_THROW(auto _ = NeuronView(nrns);, std::runtime_error);
	nrns.at(1) = NeuronColumnOnDLS(1);

	NeuronView vertex(nrns);
	EXPECT_EQ(vertex.inputs().size(), 1);
	EXPECT_EQ(vertex.inputs().front().size, NeuronColumnOnDLS::size);
	EXPECT_EQ(vertex.output().size, NeuronColumnOnDLS::size);

	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::SynapticInput);
	EXPECT_EQ(vertex.output().type, ConnectionType::MembraneVoltage);
}
