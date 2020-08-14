#include "grenade/vx/neuron_reset_mask_generator.h"

#include <hate/join.h>

namespace grenade::vx {

NeuronResetMaskGenerator::NeuronResetMaskGenerator() : enable_resets() {}

stadls::vx::v1::PlaybackGeneratorReturn<NeuronResetMaskGenerator::Result>
NeuronResetMaskGenerator::generate() const
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx;
	using namespace haldls::vx::v1;
	using namespace stadls::vx::v1;
	using namespace lola::vx::v1;

	PlaybackProgramBuilder builder;
	for (auto const quad : iter_all<NeuronResetQuadOnDLS>()) {
		auto const single_resets = quad.toNeuronResetOnDLS();
		if (std::all_of(single_resets.begin(), single_resets.end(), [&](auto const& n) {
			    return enable_resets[n];
		    })) {
			builder.write(quad, NeuronResetQuad());
		} else {
			for (auto const& n : single_resets) {
				if (enable_resets[n]) {
					builder.write(n, NeuronReset());
				}
			}
		}
	}
	return {std::move(builder), Result{}};
}

} // namespace grenade::vx
