#include "grenade/vx/signal_flow/vertex/madc_readout.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/optional.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void MADCReadoutView::Source::serialize(Archive& ar, std::uint32_t const)
{
	ar(coord);
	ar(type);
}

template <typename Archive>
void MADCReadoutView::SourceSelection::serialize(Archive& ar, std::uint32_t const)
{
	ar(initial);
	ar(period);
}

template <typename Archive>
void MADCReadoutView::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<EntityOnChip>(this));
	ar(m_first_source);
	ar(m_second_source);
	ar(m_source_selection);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::MADCReadoutView)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::MADCReadoutView, 0)
