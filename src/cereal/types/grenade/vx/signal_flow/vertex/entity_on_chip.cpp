#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/utility.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void EntityOnChip::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<grenade::common::PartitionedVertex>(this));
	ar(chip_on_connection);
	ar(m_time_domain);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::EntityOnChip)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::EntityOnChip, 0)
