#include "grenade/common/strong_component_invariant_vertex.h"

#include "grenade/common/multi_index_sequence/cuboid.h"


namespace grenade::common {

StrongComponentInvariantVertex::StrongComponentInvariantVertex(StrongComponentInvariant&& value) :
    m_value(std::move(value))
{
}

StrongComponentInvariantVertex::StrongComponentInvariantVertex(
    StrongComponentInvariant const& value) :
    m_value(value)
{
}

std::unique_ptr<Vertex::StrongComponentInvariant>
StrongComponentInvariantVertex::get_strong_component_invariant() const
{
	return m_value->copy();
}

std::vector<grenade::common::Vertex::Port> StrongComponentInvariantVertex::get_input_ports() const
{
	return {Port(
	    PortType(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence({1}))};
}

std::vector<grenade::common::Vertex::Port> StrongComponentInvariantVertex::get_output_ports() const
{
	return {Port(
	    PortType(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    grenade::common::CuboidMultiIndexSequence({1}))};
}

std::unique_ptr<grenade::common::Vertex> StrongComponentInvariantVertex::copy() const
{
	return std::make_unique<StrongComponentInvariantVertex>(*this);
}

std::unique_ptr<grenade::common::Vertex> StrongComponentInvariantVertex::move()
{
	return std::make_unique<StrongComponentInvariantVertex>(*this);
}

bool StrongComponentInvariantVertex::is_equal_to(Vertex const& other) const
{
	return m_value == static_cast<StrongComponentInvariantVertex const&>(other).m_value;
}

std::ostream& StrongComponentInvariantVertex::print(std::ostream& os) const
{
	return os << "StrongComponentInvariantVertex(" << m_value << ")";
}

} // namespace grenade::common
