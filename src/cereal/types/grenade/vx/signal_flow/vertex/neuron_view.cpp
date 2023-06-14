#include "grenade/vx/signal_flow/vertex/neuron_view.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/optional.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void NeuronView::Config::serialize(Archive& ar, std::uint32_t const)
{
	ar(label);
	ar(enable_reset);
}

template <typename Archive>
void NeuronView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_columns);
	ar(m_row);
	ar(m_configs);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::NeuronView)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::NeuronView, 2)
