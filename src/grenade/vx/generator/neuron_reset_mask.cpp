#include "grenade/vx/generator/neuron_reset_mask.h"

namespace grenade::vx::generator {

NeuronResetMask::NeuronResetMask() : enable_resets() {}

stadls::vx::v3::PlaybackGeneratorReturn<NeuronResetMask::Result> NeuronResetMask::generate() const
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;
	using namespace lola::vx::v3;

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

} // namespace grenade::vx::generator
