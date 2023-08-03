#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/vector.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void CADCMembraneReadoutView::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<EntityOnChip>(this));
	ar(m_columns);
	ar(m_synram);
	ar(m_mode);
	ar(m_sources);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::CADCMembraneReadoutView)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::CADCMembraneReadoutView, 2)
