#pragma once
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v2/neuron.h"
#include "hate/nil.h"
#include "hate/visibility.h"
#include "stadls/vx/v2/playback_generator.h"
#include "stadls/vx/v2/playback_program_builder.h"

namespace grenade::vx::generator {

/**
 * Generator for neuron resets.
 * If all resets in a quad are to be resetted, more efficient packing is used.
 */
class NeuronResetMask
{
public:
	typedef halco::common::typed_array<bool, halco::hicann_dls::vx::v2::NeuronResetOnDLS>
	    enable_resets_type;

	NeuronResetMask() SYMBOL_VISIBLE;

	/** Enable reset value per neuron. */
	halco::common::typed_array<bool, halco::hicann_dls::vx::v2::NeuronResetOnDLS> enable_resets;

	typedef stadls::vx::v2::PlaybackProgramBuilder Builder;
	typedef hate::Nil Result;

protected:
	/**
	 * Generate PlaybackProgramBuilder.
	 * @return PlaybackGeneratorReturn instance with sequence embodied and specified Result value
	 */
	stadls::vx::v2::PlaybackGeneratorReturn<Result> generate() const SYMBOL_VISIBLE;

private:
	friend auto stadls::vx::generate<NeuronResetMask>(NeuronResetMask const&);
};

} // namespace grenade::vx::generator
