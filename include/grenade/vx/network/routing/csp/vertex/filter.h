#pragma once

#include "grenade/vx/network/routing/csp/routing_vertex.h"
#include <gecode/int.hh>

namespace grenade::vx::network::routing::csp {

/**
 * Abstract filter vertex.
 */
struct SYMBOL_VISIBLE Filter : public RoutingVertex
{
	/**
	 * Applies a filter to a input value.
	 * @param space Space in which return value is contained.
	 * @param value Value on which filter is applied.
	 * @return Boolean value, describing if event passes through filter.
	 */
	virtual Gecode::BoolVar apply_filter(Gecode::Home space, Gecode::IntVar value) = 0;
};

} // namespace grenade::vx::network::routing::csp