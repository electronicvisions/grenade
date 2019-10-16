#include "grenade/vx/helper.h"

#include "halco/hicann-dls/vx/synapse_driver.h"
#include "haldls/vx/padi.h"

namespace grenade::vx {

haldls::vx::PADIEvent get_padi_event(
    halco::hicann_dls::vx::SynapseDriverOnSynapseDriverBlock const syndrv,
    haldls::vx::SynapseQuad::Label const label)
{
	using namespace haldls::vx;
	using namespace halco::hicann_dls::vx;

	PADIEvent event;
	event.set_event_address(label);
	event.set_row_select_address(PADIEvent::RowSelectAddress(syndrv / PADIBusOnPADIBusBlock::size));
	auto fire_bus = event.get_fire_bus();
	fire_bus[syndrv.toPADIBusOnPADIBusBlock()] = true;
	event.set_fire_bus(fire_bus);
	return event;
}

std::optional<haldls::vx::SynapseQuad::Label> get_address(
    halco::hicann_dls::vx::SynapseRowOnSynram const synapse_row, UInt5 const activation)
{
	using namespace haldls::vx;

	if (activation != 0) {
		UInt5 const hw_activation = UInt5(UInt5::max - activation);
		auto const is_top = (synapse_row.toEnum() % 2);
		// convention: odd rows feature high-bit in decoder address
		SynapseQuad::Label const address(hw_activation + 1 + (is_top ? 32 : 0));
		return address;
	}
	return std::nullopt;
}


} // namespace grenade::vx
