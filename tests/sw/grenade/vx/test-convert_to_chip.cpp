#include <gtest/gtest.h>

#include "grenade/vx/config.h"
#include "stadls/vx/v1/playback_program_builder.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v1;
using namespace haldls::vx::v1;
using namespace stadls::vx::v1;
using namespace lola::vx::v1;
using namespace grenade::vx;

TEST(convert_to_chip, General)
{
	ChipConfig chip;

	chip.crossbar_nodes[CrossbarNodeOnDLS(Enum(24))].set_mask(
	    CrossbarNode::neuron_label_type(0x1234));
	auto enable_spl1 =
	    chip.hemispheres[HemisphereOnDLS(0)].common_padi_bus_config.get_enable_spl1();
	enable_spl1[PADIBusOnPADIBusBlock(2)] = true;
	chip.hemispheres[HemisphereOnDLS(0)].common_padi_bus_config.set_enable_spl1(enable_spl1);
	chip.hemispheres[HemisphereOnDLS(1)]
	    .synapse_driver_block[SynapseDriverOnSynapseDriverBlock(15)]
	    .set_row_mode_top(SynapseDriverConfig::RowMode::excitatory);
	chip.hemispheres[HemisphereOnDLS(1)]
	    .synapse_matrix.weights[SynapseRowOnSynram(13)][SynapseOnSynapseRow(3)] =
	    SynapseMatrix::Weight(42);

	PlaybackProgramBuilderDumper dumper;
	for (auto const node : iter_all<CrossbarNodeOnDLS>()) {
		dumper.write(node, chip.crossbar_nodes[node]);
	}
	for (auto const hemisphere : iter_all<HemisphereOnDLS>()) {
		dumper.write(hemisphere.toSynramOnDLS(), chip.hemispheres[hemisphere].synapse_matrix);
		dumper.write(
		    hemisphere.toCommonPADIBusConfigOnDLS(),
		    chip.hemispheres[hemisphere].common_padi_bus_config);

		for (auto const syndrv : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
			dumper.write(
			    SynapseDriverOnDLS(syndrv, hemisphere.toSynapseDriverBlockOnDLS()),
			    chip.hemispheres[hemisphere].synapse_driver_block[syndrv]);
		}
	}

	auto const cocos = dumper.done();

	auto const converted_chip = convert_to_chip(cocos);

	EXPECT_EQ(converted_chip, chip);
}
