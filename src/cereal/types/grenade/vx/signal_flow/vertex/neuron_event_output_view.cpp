#include "grenade/vx/signal_flow/vertex/neuron_event_output_view.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void NeuronEventOutputView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_neurons);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::NeuronEventOutputView)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::NeuronEventOutputView, 1)
