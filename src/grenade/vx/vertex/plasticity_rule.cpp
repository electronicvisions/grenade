#include "grenade/vx/vertex/plasticity_rule.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/synapse_array_view.h"
#include "halco/common/cerealization_geometry.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::vertex {

bool PlasticityRule::Timer::operator==(Timer const& other) const
{
	return (start == other.start) && (period == other.period) && (num_periods == other.num_periods);
}

bool PlasticityRule::Timer::operator!=(Timer const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void PlasticityRule::Timer::serialize(Archive& ar, std::uint32_t const)
{
	ar(start);
	ar(period);
	ar(num_periods);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::Timer const& timer)
{
	os << "Timer(start: " << timer.start << ", period: " << timer.period
	   << ", num_periods: " << timer.num_periods << ")";
	return os;
}

PlasticityRule::PlasticityRule(
    std::string kernel, Timer const& timer, std::vector<size_t> const& synapse_column_size) :
    m_kernel(std::move(kernel)), m_timer(timer), m_synapse_column_size(synapse_column_size)
{}

std::string const& PlasticityRule::get_kernel() const
{
	return m_kernel;
}

PlasticityRule::Timer const& PlasticityRule::get_timer() const
{
	return m_timer;
}

std::vector<Port> PlasticityRule::inputs() const
{
	std::vector<Port> ret;
	for (auto const& size : m_synapse_column_size) {
		ret.push_back(Port(size, ConnectionType::SynapticInput));
	}
	return ret;
}

Port PlasticityRule::output() const
{
	return Port(0, ConnectionType::MembraneVoltage);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule const& config)
{
	os << "PlasticityRule(\nkernel:\n" << config.m_kernel << "\ntimer:\n" << config.m_timer << ")";
	return os;
}

bool PlasticityRule::supports_input_from(
    SynapseArrayView const& /*input*/, std::optional<PortRestriction> const& restriction) const
{
	return !restriction;
}

bool PlasticityRule::operator==(PlasticityRule const& other) const
{
	return (m_kernel == other.m_kernel) && (m_timer == other.m_timer) &&
	       (m_synapse_column_size == other.m_synapse_column_size);
}

bool PlasticityRule::operator!=(PlasticityRule const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void PlasticityRule::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_kernel);
	ar(m_timer);
	ar(m_synapse_column_size);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::PlasticityRule::Timer)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::PlasticityRule)
CEREAL_CLASS_VERSION(grenade::vx::vertex::PlasticityRule::Timer, 0)
CEREAL_CLASS_VERSION(grenade::vx::vertex::PlasticityRule, 0)
