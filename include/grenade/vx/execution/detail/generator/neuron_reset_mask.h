#pragma once
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "haldls/vx/v3/timer.h"
#include "hate/visibility.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/absolute_time_playback_program_builder.h"

namespace grenade::vx::execution::detail::generator {

/**
 * Generator for neuron resets.
 * If all resets in a quad are to be resetted, more efficient packing is used.
 */
class NeuronResetMask
{
public:
	typedef halco::common::typed_array<bool, halco::hicann_dls::vx::v3::NeuronResetOnDLS>
	    enable_resets_type;

	NeuronResetMask() SYMBOL_VISIBLE;

	/** Enable reset value per neuron. */
	halco::common::typed_array<bool, halco::hicann_dls::vx::v3::NeuronResetOnDLS> enable_resets;

	typedef stadls::vx::v3::AbsoluteTimePlaybackProgramBuilder Builder;
	typedef haldls::vx::v3::Timer::Value Result;

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

private:
	friend auto stadls::vx::generate<NeuronResetMask>(NeuronResetMask const&);
};

} // namespace grenade::vx::execution::detail::generator
