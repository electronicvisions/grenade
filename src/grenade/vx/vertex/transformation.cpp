#include "grenade/vx/vertex/transformation.h"

#include "grenade/cerealization.h"
#include <stdexcept>
#include <cereal/types/memory.hpp>

namespace grenade::vx::vertex {

Transformation::Function::~Function() {}

Transformation::Transformation(std::unique_ptr<Function> function) : m_function(std::move(function))
{}

std::vector<Port> Transformation::inputs() const
{
	assert(m_function);
	return m_function->inputs();
}

Port Transformation::output() const
{
	assert(m_function);
	return m_function->output();
}

Transformation::Function::Value Transformation::apply(
    std::vector<Function::Value> const& value) const
{
	assert(m_function);
	return m_function->apply(value);
}

std::ostream& operator<<(std::ostream& os, Transformation const&)
{
	os << "Transformation()";
	return os;
}

bool Transformation::operator==(Transformation const& other) const
{
	// FIXME (maybe typeid?)
	if (static_cast<bool>(m_function) != static_cast<bool>(other.m_function)) {
		return false;
	}
	if (!m_function && !other.m_function) {
		return true;
	}
	return (m_function->equal(*other.m_function));
}

bool Transformation::operator!=(Transformation const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void Transformation::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_function);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::Transformation)
CEREAL_CLASS_VERSION(grenade::vx::vertex::Transformation, 0)
