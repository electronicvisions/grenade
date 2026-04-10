#include "grenade/vx/network/routing/csp/vertex/crossbar_node.h"
#include "grenade/vx/network/routing/csp/helper/bitset_beautifier.h"
#include "grenade/vx/network/routing/csp/propagator/bitwise_and.h"
#include "hate/indent.h"

namespace grenade::vx::network::routing::csp {

CrossbarNode::CrossbarNode(size_t bit_count, Gecode::Space& space) :
    mask(Gecode::IntVar(space, 0, (1 << bit_count) - 1)),
    target(Gecode::IntVar(space, 0, (1 << bit_count) - 1)),
    m_bit_count(bit_count)
{
}
CrossbarNode::~CrossbarNode() {}

std::string CrossbarNode::get_name() const
{
	return "CrossbarNode";
}

void CrossbarNode::update(Gecode::Space& space, RoutingVertex& other)
{
	auto& other_casted = dynamic_cast<CrossbarNode&>(other);
	mask.update(space, other_casted.mask);
	target.update(space, other_casted.target);
}

Gecode::IntVarArgs CrossbarNode::get_parameters() const
{
	Gecode::IntVarArgs ret;
	ret << mask;
	ret << target;
	return ret;
}

std::vector<std::string> CrossbarNode::get_parameter_names() const
{
	return std::vector<std::string>{"Mask", "Target"};
}

Gecode::BoolVar CrossbarNode::apply_filter(Gecode::Home space, Gecode::IntVar value)
{
	Gecode::IntVar tmp(space, 0, (1 << m_bit_count) - 1);
	propagator::bitwise_and(space, value, mask, tmp);
	Gecode::BoolVar ret(space, 0, 1);
	Gecode::rel(space, tmp, Gecode::IRT_EQ, target, ret);
	return ret;
}

std::unique_ptr<RoutingVertex> CrossbarNode::copy() const
{
	return std::make_unique<CrossbarNode>(*this);
}

std::unique_ptr<RoutingVertex> CrossbarNode::move()
{
	return std::make_unique<CrossbarNode>(std::move(*this));
}

std::vector<size_t> CrossbarNode::get_parameter_bitcounts() const
{
	return std::vector<size_t>(2, m_bit_count);
}

bool CrossbarNode::is_equal_to(RoutingVertex const&) const
{
	return false;
}

} // namespace grenade::vx::network::routing::csp