#include "grenade/vx/execution/detail/generator/ppu.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/block.h"
#include "haldls/vx/v3/ppu.h"

namespace grenade::vx::execution::detail::generator {

using namespace haldls::vx::v3;
using namespace stadls::vx::v3;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

stadls::vx::PlaybackGeneratorReturn<PPUCommand::Builder, PPUCommand::Result> PPUCommand::generate()
    const
{
	Builder builder;
	Result current_time = Result(0);
	// write command to be executed
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PPUMemoryWord config(PPUMemoryWord::Value(static_cast<uint32_t>(m_status)));
		builder.write(current_time, PPUMemoryWordOnDLS(m_coord, ppu), config);
		current_time += Timer::Value(2);
	}
	return {std::move(builder), current_time};
}

stadls::vx::PlaybackGeneratorReturn<PPUStop::Builder, PPUStop::Result> PPUStop::generate() const
{
	Builder builder;

	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PPUMemoryWord config(
		    PPUMemoryWord::Value(static_cast<uint32_t>(ppu::detail::Status::stop)));
		builder.write(PPUMemoryWordOnDLS(m_coord, ppu), config);
	}
	// poll for completion by waiting until PPU is asleep
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PollingOmnibusBlockConfig config;
		config.set_address(PPUStatusRegister::read_addresses<PollingOmnibusBlockConfig::Address>(
		                       ppu.toPPUStatusRegisterOnDLS())
		                       .at(0));
		config.set_target(PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(true)));
		config.set_mask(PollingOmnibusBlockConfig::Value(0x00000001));
		builder.write(PollingOmnibusBlockConfigOnFPGA(), config);
		builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
		builder.block_until(PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());
	}
	// disable inhibit reset
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PPUControlRegister ctrl;
		ctrl.set_inhibit_reset(false);
		builder.write(ppu.toPPUControlRegisterOnDLS(), ctrl);
	}
	builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
	return {std::move(builder), {}};
}

} // namespace grenade::vx::execution::detail::generator
