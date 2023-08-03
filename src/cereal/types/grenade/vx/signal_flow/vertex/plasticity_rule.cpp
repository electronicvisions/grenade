#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"

#include "cereal/types/halco/common/geometry.h"
#include "cereal/types/halco/common/typed_array.h"
#include "grenade/cerealization.h"
#include <cereal/types/map.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::signal_flow::vertex {

template <typename Archive>
void PlasticityRule::Timer::serialize(Archive& ar, std::uint32_t const)
{
	ar(start);
	ar(period);
	ar(num_periods);
}

template <typename Archive>
void PlasticityRule::RawRecording::serialize(Archive& ar, std::uint32_t const)
{
	ar(scratchpad_memory_size);
}

template <typename Archive>
void PlasticityRule::TimedRecording::ObservablePerSynapse::serialize(
    Archive& ar, std::uint32_t const)
{
	ar(type);
	ar(layout_per_row);
}

template <typename Archive>
void PlasticityRule::TimedRecording::ObservablePerNeuron::serialize(
    Archive& ar, std::uint32_t const)
{
	ar(type);
	ar(layout);
}

template <typename Archive>
void PlasticityRule::TimedRecording::ObservableArray::serialize(Archive& ar, std::uint32_t const)
{
	ar(type);
	ar(size);
}

template <typename Archive>
void PlasticityRule::TimedRecording::serialize(Archive& ar, std::uint32_t const)
{
	ar(observables);
}

template <typename Archive>
void PlasticityRule::SynapseViewShape::serialize(Archive& ar, std::uint32_t const)
{
	ar(num_rows);
	ar(columns.size());
}

template <typename Archive>
void PlasticityRule::NeuronViewShape::serialize(Archive& ar, std::uint32_t const)
{
	ar(columns);
	ar(row);
	ar(neuron_readout_sources);
}

template <typename Archive>
void PlasticityRule::serialize(Archive& ar, std::uint32_t const)
{
	ar(cereal::base_class<EntityOnChip>(this));
	ar(m_kernel);
	ar(m_timer);
	ar(m_synapse_view_shapes);
	ar(m_neuron_view_shapes);
	ar(m_recording);
}

} // namespace grenade::vx::signal_flow::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::PlasticityRule::Timer)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::PlasticityRule)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::PlasticityRule::Timer, 0)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::PlasticityRule, 3)
