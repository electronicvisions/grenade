#pragma once
#include <optional>
#include "grenade/vx/types.h"
#include "haldls/vx/synapse.h"
#include "hate/visibility.h"

namespace haldls::vx {
class PADIEvent;
} // namespace haldls::vx

namespace halco::hicann_dls::vx {
struct SynapseDriverOnSynapseDriverBlock;
struct SynapseRowOnSynram;
} // namespace halco::hicann_dls::vx

namespace grenade::vx {

haldls::vx::PADIEvent get_padi_event(
    halco::hicann_dls::vx::SynapseDriverOnSynapseDriverBlock syndrv,
    haldls::vx::SynapseQuad::Label label);

std::optional<haldls::vx::SynapseQuad::Label> get_address(
    halco::hicann_dls::vx::SynapseRowOnSynram synapse_row, UInt5 activation) SYMBOL_VISIBLE;

} // namespace grenade::vx
