#include "grenade/vx/execution/detail/generator/ppu.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/block.h"
#include "haldls/vx/v3/ppu.h"
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail::generator {

using namespace haldls::vx::v3;
using namespace stadls::vx::v3;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

void PPUCommand::Result::evaluate() const
{
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		auto const previous_status_value = static_cast<grenade::vx::ppu::detail::Status>(
		    dynamic_cast<PPUMemoryWord const&>(previous_status[ppu].get()).get_value().value());
		if (previous_status_value != expected_previous_status) {
			auto logger =
			    log4cxx::Logger::getLogger("grenade.execution.detail.generator.PPUCommand");
			LOG4CXX_WARN(
			    logger, "evaluate(): PPU status of " << ppu << "(" << previous_status_value
			                                         << ") doesn't match expectation ("
			                                         << expected_previous_status << ")");
		}
	}
}

stadls::vx::PlaybackGeneratorReturn<PPUCommand::Builder, PPUCommand::Result> PPUCommand::generate()
    const
{
	Builder builder;
	Result result;
	haldls::vx::v3::Timer::Value current_time(0);
	// write command to be executed
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		result.previous_status[ppu] = builder.read(current_time, PPUMemoryWordOnDLS(m_coord, ppu));
		current_time += Timer::Value(1);
		PPUMemoryWord config(PPUMemoryWord::Value(static_cast<uint32_t>(m_status)));
		builder.write(current_time, PPUMemoryWordOnDLS(m_coord, ppu), config);
		current_time += Timer::Value(2);
	}
	result.duration = current_time;
	result.expected_previous_status = m_expected_previous_status;
	return {std::move(builder), result};
}


stadls::vx::PlaybackGeneratorReturn<PPUStart::Builder, PPUStart::Result> PPUStart::generate() const
{
	Builder builder;

	for (auto const ppu : iter_all<PPUOnDLS>()) {
		haldls::vx::v3::PPUControlRegister ctrl;
		ctrl.set_inhibit_reset(true);
		builder.write(ppu.toPPUControlRegisterOnDLS(), ctrl);
	}
	// wait for PPUs to be ready
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		using namespace haldls::vx::v3;
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
	return {std::move(builder), {}};
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
