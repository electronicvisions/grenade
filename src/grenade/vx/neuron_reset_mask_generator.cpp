#include "grenade/vx/neuron_reset_mask_generator.h"

#include <hate/join.h>

namespace grenade::vx {

NeuronResetMaskGenerator::NeuronResetMaskGenerator() : enable_resets() {}

stadls::vx::PlaybackGeneratorReturn<NeuronResetMaskGenerator::Result>
NeuronResetMaskGenerator::generate() const
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx;
	using namespace haldls::vx;
	using namespace stadls::vx;
	using namespace lola::vx;

	PlaybackProgramBuilder builder;
	NeuronReset const reset;
	for (auto const neuron : iter_all<NeuronResetOnDLS>()) {
		if (enable_resets[neuron]) {
			builder.write(neuron, reset);
		}
	}
	return {std::move(builder), Result{}};
}

} // namespace grenade::vx
