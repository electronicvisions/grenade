#pragma once
#include <optional>
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v2/synapse.h"
#include "halco/hicann-dls/vx/v2/synapse_driver.h"
#include "haldls/vx/v2/padi.h"
#include "haldls/vx/v2/synapse.h"
#include "hate/visibility.h"

namespace grenade::vx {

haldls::vx::v2::PADIEvent get_padi_event(
    halco::hicann_dls::vx::v2::SynapseDriverOnSynapseDriverBlock syndrv,
    haldls::vx::v2::SynapseQuad::Label label);

std::optional<haldls::vx::v2::SynapseQuad::Label> get_address(
    halco::hicann_dls::vx::v2::SynapseRowOnSynram synapse_row, UInt5 activation) SYMBOL_VISIBLE;

} // namespace grenade::vx
