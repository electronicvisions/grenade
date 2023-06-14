#include "grenade/vx/signal_flow/vertex/transformation.h"

#include "grenade/cerealization.h"
#include <cereal/types/memory.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void Transformation::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_function);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::Transformation)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::Transformation, 0)
