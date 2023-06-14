#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/cerealization.h"
#include <cereal/types/vector.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void SynapseArrayViewSparse::Synapse::serialize(Archive& ar, std::uint32_t const)
{
	ar(weight);
	ar(label);
	ar(index_row);
	ar(index_column);
}

template <typename Archive>
void SynapseArrayViewSparse::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_synapses);
	ar(m_columns);
	ar(m_rows);
	ar(m_synram);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::SynapseArrayViewSparse)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::SynapseArrayViewSparse, 0)
