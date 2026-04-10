#pragma once
#include "hate/visibility.h"
#include <array>

namespace Gecode {
class Home;
class IntVar;
} // namespace Gecode


namespace grenade::vx::network::routing::csp::propagator {

/**
 * Propagator performing bitwise AND operation.
 * Operation performed is x0 & x1 = x2.
 */
void bitwise_and(Gecode::Home home, Gecode::IntVar x0, Gecode::IntVar x1, Gecode::IntVar x2)
    SYMBOL_VISIBLE;

/**
 * Propagator performing bitwise AND operation.
 * Operation performed is x0 & x1 = x2, for which x0 is a constant.
 */
void bitwise_and(Gecode::Home home, int x0, Gecode::IntVar x1, Gecode::IntVar x2) SYMBOL_VISIBLE;

} // namespace grenade::vx::network::routing::csp::propagator
