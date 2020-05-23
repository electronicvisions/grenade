#pragma once
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/neuron.h"
#include "hate/nil.h"
#include "hate/visibility.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/playback_program_builder.h"

namespace grenade::vx {

/**
 * Generator for neuron resets.
 * If all resets in a quad are to be resetted, more efficient packing is used.
 */
class NeuronResetMaskGenerator
{
public:
	typedef halco::common::typed_array<bool, halco::hicann_dls::vx::NeuronResetOnDLS>
	    enable_resets_type;

	NeuronResetMaskGenerator() SYMBOL_VISIBLE;

	/** Enable reset value per neuron. */
	halco::common::typed_array<bool, halco::hicann_dls::vx::NeuronResetOnDLS> enable_resets;

	typedef hate::Nil Result;

protected:
	/**
	 * Generate PlaybackProgramBuilder.
	 * @return PlaybackGeneratorReturn instance with sequence embodied and specified Result value
	 */
	stadls::vx::PlaybackGeneratorReturn<Result> generate() const SYMBOL_VISIBLE;

private:
	friend auto stadls::vx::generate<NeuronResetMaskGenerator>(NeuronResetMaskGenerator const&);
};

} // namespace grenade::vx
