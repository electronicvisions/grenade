#include "grenade/vx/config.h"

#include "lola/vx/hana.h"

namespace grenade::vx {

HemisphereConfig::HemisphereConfig() {}

bool HemisphereConfig::operator==(HemisphereConfig const& other) const
{
	return equal(*this, other);
}

bool HemisphereConfig::operator!=(HemisphereConfig const& other) const
{
	return unequal(*this, other);
}


ChipConfig::ChipConfig() {}

bool ChipConfig::operator==(ChipConfig const& other) const
{
	return equal(*this, other);
}

bool ChipConfig::operator!=(ChipConfig const& other) const
{
	return unequal(*this, other);
}

ChipConfig convert_to_chip(stadls::vx::v2::Dumper::done_type const& cocos)
{
	ChipConfig chip;
	auto const apply_coco = [&chip](auto const& coco) {
		auto const& [coord, config] = coco;
		typedef std::decay_t<decltype(config)> config_t;
		if constexpr (std::is_same_v<config_t, haldls::vx::v2::CrossbarNode>) {
			chip.crossbar_nodes[coord] = config;
		} else if constexpr (std::is_same_v<config_t, haldls::vx::v2::CommonPADIBusConfig>) {
			chip.hemispheres[coord.toHemisphereOnDLS()].common_padi_bus_config = config;
		} else if constexpr (std::is_same_v<config_t, haldls::vx::v2::SynapseDriverConfig>) {
			chip.hemispheres[coord.toSynapseDriverBlockOnDLS().toHemisphereOnDLS()]
			    .synapse_driver_block[coord.toSynapseDriverOnSynapseDriverBlock()] = config;
		} else if constexpr (std::is_same_v<config_t, lola::vx::v2::SynapseMatrix>) {
			chip.hemispheres[coord.toHemisphereOnDLS()].synapse_matrix = config;
		}
	};

	for (auto const& coco : cocos.values) {
		std::visit(apply_coco, coco);
	}
	return chip;
}

} // namespace grenade::vx
