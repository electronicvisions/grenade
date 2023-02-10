#include "grenade/vx/generator/ppu.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/barrier.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/block.h"
#include "haldls/vx/v3/ppu.h"
#include "stadls/vx/v3/playback_program_builder.h"

namespace grenade::vx::generator {

using namespace haldls::vx::v3;
using namespace stadls::vx::v3;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

PlaybackGeneratorReturn<BlockingPPUCommand::Result> BlockingPPUCommand::generate() const
{
	PlaybackProgramBuilder builder;
	// write command to be executed
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PPUMemoryWord config(PPUMemoryWord::Value(static_cast<uint32_t>(m_status)));
		builder.write(PPUMemoryWordOnDLS(m_coord, ppu), config);
	}
	// poll for completion by waiting until PPU set `idle` status
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PollingOmnibusBlockConfig config;
		config.set_address(PPUMemoryWord::addresses<PollingOmnibusBlockConfig::Address>(
		                       PPUMemoryWordOnDLS(m_coord, ppu))
		                       .at(0));
		config.set_target(
		    PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(ppu::detail::Status::idle)));
		config.set_mask(PollingOmnibusBlockConfig::Value(0xffffffff));
		builder.write(PollingOmnibusBlockConfigOnFPGA(), config);
		builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
		builder.block_until(PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());
	}

	return {std::move(builder), hate::Nil{}};
}

} // namespace grenade::vx::generator
