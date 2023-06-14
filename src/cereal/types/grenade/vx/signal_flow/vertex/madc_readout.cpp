#include "grenade/vx/signal_flow/vertex/madc_readout.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void MADCReadoutView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_coord);
	ar(m_config);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::MADCReadoutView)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::MADCReadoutView, 0)
