#include "grenade/vx/execution/detail/generator/ppu.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/fpga.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/block.h"
#include "haldls/vx/v3/ppu.h"
#include "hate/variant.h"
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
	// increase instruction timeout
	InstructionTimeoutConfig instruction_timeout;
	instruction_timeout.set_value(InstructionTimeoutConfig::Value(
	    1000000 * InstructionTimeoutConfig::Value::fpga_clock_cycles_per_us));
	builder.write(halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(), instruction_timeout);
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
	// reset instruction timeout to default
	builder.write(
	    halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(), InstructionTimeoutConfig());
	return {std::move(builder), {}};
}


stadls::vx::PlaybackGeneratorReturn<PPUStop::Builder, PPUStop::Result> PPUStop::generate() const
{
	Builder builder;

	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PPUMemoryWord config(
		    PPUMemoryWord::Value(static_cast<uint32_t>(ppu::detail::Status::stop)));
		auto const ppu_coord =
		    std::get<PPUMemoryBlockOnPPU>(m_symbols.at("status").coordinate).toMin();
		builder.write(PPUMemoryWordOnDLS(ppu_coord, ppu), config);
	}
	// poll for completion by waiting until PPU has stopped state
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PollingOmnibusBlockConfig config;
		auto const ppu_stopped_coord =
		    std::get<PPUMemoryBlockOnPPU>(m_symbols.at("stopped").coordinate).toMin();
		config.set_address(PPUMemoryWord::addresses<PollingOmnibusBlockConfig::Address>(
		                       PPUMemoryWordOnDLS(ppu_stopped_coord, ppu))
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


signal_flow::OutputData::ReadPPUSymbols::value_type::mapped_type::mapped_type
PPUReadHooks::Result::evaluate() const
{
	signal_flow::OutputData::ReadPPUSymbols::value_type::mapped_type::mapped_type ret;
	for (auto const& [name, ticket] : tickets) {
		std::visit(
		    hate::overloaded{
		        [&ret, name](std::map<
		                     halco::hicann_dls::vx::v3::HemisphereOnDLS,
		                     stadls::vx::v3::ContainerTicket> const& tickets) {
			        ret[name] = std::map<
			            halco::hicann_dls::vx::v3::HemisphereOnDLS, haldls::vx::v3::PPUMemoryBlock>{
			            {halco::hicann_dls::vx::v3::HemisphereOnDLS::top,
			             dynamic_cast<haldls::vx::v3::PPUMemoryBlock const&>(
			                 tickets.at(halco::hicann_dls::vx::v3::HemisphereOnDLS::top).get())},
			            {halco::hicann_dls::vx::v3::HemisphereOnDLS::bottom,
			             dynamic_cast<haldls::vx::v3::PPUMemoryBlock const&>(
			                 tickets.at(halco::hicann_dls::vx::v3::HemisphereOnDLS::bottom)
			                     .get())}};
		        },
		        [&ret, name](stadls::vx::v3::ContainerTicket const& ticket) {
			        if (auto const ptr = dynamic_cast<lola::vx::v3::ExternalPPUMemoryBlock const*>(
			                &ticket.get());
			            ptr) {
				        ret[name] = *ptr;
			        } else {
				        ret[name] = dynamic_cast<lola::vx::v3::ExternalPPUDRAMMemoryBlock const&>(
				            ticket.get());
			        }
		        },
		    },
		    ticket);
	}
	return ret;
}

stadls::vx::PlaybackGeneratorReturn<PPUReadHooks::Builder, PPUReadHooks::Result>
PPUReadHooks::generate() const
{
	Builder builder;
	Result result;

	for (auto const& name : m_symbol_names) {
		if (!m_symbols.contains(name)) {
			throw std::runtime_error("Provided unknown symbol name via ExecutionInstanceHooks.");
		}
		std::visit(
		    hate::overloaded{
		        [&](halco::hicann_dls::vx::v3::PPUMemoryBlockOnPPU const& coordinate) {
			        std::map<HemisphereOnDLS, ContainerTicket> values;
			        values.emplace(
			            HemisphereOnDLS::top,
			            builder.read(PPUMemoryBlockOnDLS(coordinate, PPUOnDLS::top)));
			        values.emplace(
			            HemisphereOnDLS::bottom,
			            builder.read(PPUMemoryBlockOnDLS(coordinate, PPUOnDLS::bottom)));
			        result.tickets[name] = values;
		        },
		        [&](halco::hicann_dls::vx::v3::ExternalPPUMemoryBlockOnFPGA const& coordinate) {
			        result.tickets.insert({name, builder.read(coordinate)});
		        },
		        [&](halco::hicann_dls::vx::v3::ExternalPPUDRAMMemoryBlockOnFPGA const& coordinate) {
			        result.tickets.insert({name, builder.read(coordinate)});
		        }},
		    m_symbols.at(name).coordinate);
	}
	if (!builder.empty()) {
		builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
	}

	return {std::move(builder), std::move(result)};
}


stadls::vx::PlaybackGeneratorReturn<PPUPeriodicCADCRead::Builder, PPUPeriodicCADCRead::Result>
PPUPeriodicCADCRead::generate() const
{
	Builder builder;
	Result result;

	InstructionTimeoutConfig instruction_timeout;
	instruction_timeout.set_value(InstructionTimeoutConfig::Value(
	    100000 * InstructionTimeoutConfig::Value::fpga_clock_cycles_per_us));
	builder.write(halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(), instruction_timeout);

	// wait for ppu command idle
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PPUMemoryWordOnPPU ppu_status_coord;
		ppu_status_coord = std::get<PPUMemoryBlockOnPPU>(m_symbols.at("status").coordinate).toMin();
		PollingOmnibusBlockConfig polling_config;
		polling_config.set_address(PPUMemoryWord::addresses<PollingOmnibusBlockConfig::Address>(
		                               PPUMemoryWordOnDLS(ppu_status_coord, ppu))
		                               .at(0));
		polling_config.set_target(
		    PollingOmnibusBlockConfig::Value(static_cast<uint32_t>(ppu::detail::Status::idle)));
		polling_config.set_mask(PollingOmnibusBlockConfig::Value(0xffffffff));
		builder.write(PollingOmnibusBlockConfigOnFPGA(), polling_config);
		builder.block_until(BarrierOnFPGA(), Barrier::omnibus);
		builder.block_until(PollingOmnibusBlockOnFPGA(), PollingOmnibusBlock());
	}

	builder.write(
	    halco::hicann_dls::vx::InstructionTimeoutConfigOnFPGA(), InstructionTimeoutConfig());

	// generate tickets for extmem readout of periodic cadc recording data
	std::visit(
	    hate::overloaded{
	        [&](ExternalPPUMemoryBlockOnFPGA const& coord_top,
	            ExternalPPUMemoryBlockOnFPGA const& coord_bot) {
		        if (m_used_hemispheres[HemisphereOnDLS::top]) {
			        result.tickets[PPUOnDLS::top] = builder.read(coord_top);
		        }
		        if (m_used_hemispheres[HemisphereOnDLS::bottom]) {
			        result.tickets[PPUOnDLS::bottom] = builder.read(coord_bot);
		        }
	        },
	        [&](ExternalPPUDRAMMemoryBlockOnFPGA const& coord_top,
	            ExternalPPUDRAMMemoryBlockOnFPGA const& coord_bot) {
		        if (m_used_hemispheres[HemisphereOnDLS::top]) {
			        result.tickets[PPUOnDLS::top] = builder.read(coord_top);
		        }
		        if (m_used_hemispheres[HemisphereOnDLS::bottom]) {
			        result.tickets[PPUOnDLS::bottom] = builder.read(coord_bot);
		        }
	        },
	        [&](auto const&, auto const&) {
		        throw std::logic_error(
		            "Reading CADC samples from given coordinate types not implemented.");
	        }},
	    m_symbols.at("periodic_cadc_samples_top").coordinate,
	    m_symbols.at("periodic_cadc_samples_bot").coordinate);

	// Reset the offset, from where the PPU begins to write the recorded CADC data
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		builder.write(
		    PPUMemoryWordOnDLS(
		        std::get<PPUMemoryBlockOnPPU>(
		            m_symbols.at("periodic_cadc_readout_memory_offset").coordinate)
		            .toMin(),
		        ppu),
		    PPUMemoryWord(PPUMemoryWord::Value(static_cast<uint32_t>(0))));
	}

	// wait for response data
	builder.block_until(BarrierOnFPGA(), haldls::vx::v3::Barrier::omnibus);

	return {std::move(builder), std::move(result)};
}


} // namespace grenade::vx::execution::detail::generator
