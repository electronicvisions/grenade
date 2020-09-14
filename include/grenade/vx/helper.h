#pragma once
#include <optional>
#include "grenade/vx/types.h"
#include "halco/hicann-dls/vx/v1/synapse.h"
#include "halco/hicann-dls/vx/v1/synapse_driver.h"
#include "haldls/vx/v1/padi.h"
#include "haldls/vx/v1/synapse.h"
#include "hate/visibility.h"

namespace grenade::vx {

haldls::vx::v1::PADIEvent get_padi_event(
    halco::hicann_dls::vx::v1::SynapseDriverOnSynapseDriverBlock syndrv,
    haldls::vx::v1::SynapseQuad::Label label);

std::optional<haldls::vx::v1::SynapseQuad::Label> get_address(
    halco::hicann_dls::vx::v1::SynapseRowOnSynram synapse_row, UInt5 activation) SYMBOL_VISIBLE;

} // namespace grenade::vx
