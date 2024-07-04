#pragma once
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "hate/nil.h"
#include "hate/visibility.h"
#include "stadls/vx/playback_generator.h"
#include "stadls/vx/v3/container_ticket.h"
#include "stadls/vx/v3/playback_program_builder.h"

namespace grenade::vx::execution::detail::generator {

/**
 * Generator for a playback program snippet for getting current hardware state.
 */
struct GetState
{
	struct Result
	{
		void apply(lola::vx::v3::Chip& chip) const SYMBOL_VISIBLE;

		halco::common::typed_array<
		    std::optional<stadls::vx::v3::ContainerTicket>,
		    halco::hicann_dls::vx::v3::PPUMemoryOnDLS>
		    ticket_ppu_memory;
		halco::common::typed_array<
		    std::optional<stadls::vx::v3::ContainerTicket>,
		    halco::hicann_dls::vx::v3::SynapseWeightMatrixOnDLS>
		    ticket_synaptic_weights;
	};
	typedef stadls::vx::v3::PlaybackProgramBuilder Builder;

	bool has_plasticity{false};

protected:
	stadls::vx::PlaybackGeneratorReturn<Builder, Result> generate() const SYMBOL_VISIBLE;

	friend auto stadls::vx::generate<GetState>(GetState const&);
};

} // namespace grenade::vx::execution::detail::generator
