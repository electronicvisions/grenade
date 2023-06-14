#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/vector.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void SynapseArrayView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_weights);
	ar(m_labels);
	ar(m_columns);
	ar(m_rows);
	ar(m_synram);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::SynapseArrayView)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::SynapseArrayView, 0)
