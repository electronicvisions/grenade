#include <gtest/gtest.h>

#include "grenade/vx/config.h"
#include "stadls/vx/v2/playback_program_builder.h"

using namespace halco::common;
using namespace halco::hicann_dls::vx::v2;
using namespace haldls::vx::v2;
using namespace stadls::vx::v2;
using namespace lola::vx::v2;
using namespace grenade::vx;

TEST(convert_to_chip, General)
{
	ChipConfig chip;

	chip.crossbar.nodes[CrossbarNodeOnDLS(Enum(24))].set_mask(
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
	auto buffer_to_pad = chip.readout_source_selection.get_enable_buffer_to_pad();
	buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection()] =
	    !buffer_to_pad[SourceMultiplexerOnReadoutSourceSelection()];
	chip.readout_source_selection.set_enable_buffer_to_pad(buffer_to_pad);
	chip.madc_config.set_enable_calibration(!chip.madc_config.get_enable_calibration());

	PlaybackProgramBuilderDumper dumper;
	dumper.write(CrossbarOnDLS(), chip.crossbar);
	dumper.write(ReadoutSourceSelectionOnDLS(), chip.readout_source_selection);
	dumper.write(MADCConfigOnDLS(), chip.madc_config);
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

	// resetting enable_calibration
	chip.madc_config.set_enable_calibration(!chip.madc_config.get_enable_calibration());
	chip.madc_config.set_enable_dummy_data(!chip.madc_config.get_enable_dummy_data());

	ChipConfig chip_previous;
	chip_previous.madc_config.set_enable_dummy_data(
	    !chip_previous.madc_config.get_enable_dummy_data());

	dumper = PlaybackProgramBuilderDumper();
	dumper.write(CrossbarOnDLS(), chip.crossbar);
	dumper.write(ReadoutSourceSelectionOnDLS(), chip.readout_source_selection);
	// leaving out madc_config here
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

	auto const cocos_partial = dumper.done();

	auto const converted_chip_with_previous = convert_to_chip(cocos_partial, chip_previous);

	EXPECT_EQ(converted_chip_with_previous, chip);
}
