#include "grenade/vx/execution/detail/generator/neuron_reset_mask.h"

namespace grenade::vx::execution::detail::generator {

NeuronResetMask::NeuronResetMask() : enable_resets() {}

stadls::vx::PlaybackGeneratorReturn<NeuronResetMask::Builder, NeuronResetMask::Result>
NeuronResetMask::generate() const
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3;
	using namespace lola::vx::v3;

	Builder builder;
	Result current_time = Result(0);
	for (auto const quad : iter_all<NeuronResetQuadOnDLS>()) {
		auto const single_resets = quad.toNeuronResetOnDLS();
		if (std::all_of(single_resets.begin(), single_resets.end(), [&](auto const& n) {
			    return enable_resets[n];
		    })) {
			builder.write(current_time, quad, NeuronResetQuad());
			current_time += Timer::Value(2);
		} else {
			for (auto const& n : single_resets) {
				if (enable_resets[n]) {
					builder.write(current_time, n, NeuronReset());
					current_time += Timer::Value(2);
				}
			}
		}
	}
	return {std::move(builder), current_time};
}

} // namespace grenade::vx::execution::detail::generator
