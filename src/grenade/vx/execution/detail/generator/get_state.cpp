#include "grenade/vx/execution/detail/generator/get_state.h"

#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/timer.h"

namespace grenade::vx::execution::detail::generator {

using namespace haldls::vx::v3;
using namespace stadls::vx::v3;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

void GetState::Result::apply(lola::vx::v3::Chip& chip) const
{
	for (auto const ppu : iter_all<PPUMemoryOnDLS>()) {
		assert(ticket_ppu_memory[ppu]);
		chip.ppu_memory[ppu] =
		    dynamic_cast<haldls::vx::v3::PPUMemory const&>(ticket_ppu_memory[ppu]->get());
	}
	for (auto const coord : iter_all<SynapseWeightMatrixOnDLS>()) {
		if (!ticket_synaptic_weights[coord]) {
			continue;
		}
		chip.synapse_blocks[coord.toSynapseBlockOnDLS()].matrix.weights =
		    dynamic_cast<lola::vx::v3::SynapseWeightMatrix const&>(
		        ticket_synaptic_weights[coord]->get())
		        .values;
	}
}


stadls::vx::PlaybackGeneratorReturn<GetState::Builder, GetState::Result> GetState::generate() const
{
	Builder builder;
	Result result;
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		result.ticket_ppu_memory[ppu.toPPUMemoryOnDLS()].emplace(
		    builder.read(ppu.toPPUMemoryOnDLS()));
	}
	if (has_plasticity) {
		for (auto const coord : iter_all<SynapseWeightMatrixOnDLS>()) {
			result.ticket_synaptic_weights[coord].emplace(builder.read(coord));
		}
	}
	builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);

	return {std::move(builder), std::move(result)};
}

} // namespace grenade::vx::execution::detail::generator
