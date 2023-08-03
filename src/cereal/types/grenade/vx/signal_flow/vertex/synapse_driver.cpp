#include "grenade/vx/signal_flow/vertex/synapse_driver.h"

#include "cereal/types/halco/common/geometry.h"
#include "cereal/types/halco/common/typed_array.h"
#include "grenade/cerealization.h"

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void SynapseDriver::Config::serialize(Archive& ar, std::uint32_t const)
{
	ar(row_address_compare_mask);
	ar(row_modes);
	ar(enable_address_out);
}

template <typename Archive>
void SynapseDriver::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<EntityOnChip>(this));
	ar(m_coordinate);
	ar(m_config);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::SynapseDriver)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::SynapseDriver, 1)
