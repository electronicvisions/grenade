#include "grenade/vx/signal_flow/vertex/transformation.h"

#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

Transformation::Function::~Function() {}

Transformation::Transformation(std::unique_ptr<Function> function) : m_function(std::move(function))
{}

Transformation::Transformation(Transformation const& other) :
    m_function(other.m_function ? other.m_function->clone() : nullptr)
{}

Transformation::Transformation(Transformation&& other) :
    m_function(std::move(other.m_function->clone()))
{}

Transformation& Transformation::operator=(Transformation const& other)
{
	m_function = other.m_function ? other.m_function->clone() : nullptr;
	return *this;
}

Transformation& Transformation::operator=(Transformation&& other)
{
	m_function = std::move(other.m_function);
	return *this;
}

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

} // namespace grenade::vx::signal_flow::vertex
