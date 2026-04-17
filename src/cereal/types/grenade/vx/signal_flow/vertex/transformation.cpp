#include "grenade/vx/signal_flow/vertex/transformation.h"

#include "cereal/types/dapr/property_holder.h"
#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void Transformation::Function::serialize(Archive&, std::uint32_t const)
{
}

template <typename Archive>
void Transformation::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<grenade::common::PartitionedVertex>(this));
	ar(CEREAL_NVP(m_function));
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::Transformation::Function)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::Transformation::Function, 0)

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::Transformation)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::Transformation, 0)
