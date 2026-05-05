#pragma once
#include "grenade/vx/genpybind.h"

namespace grenade {
namespace vx GENPYBIND_TAG_GRENADE_VX {

/** Assumed ideal, maximal capacitance per neuron circuit in Farad. */
constexpr float GENPYBIND(visible) ideal_capacitance_per_neuron = 2.2e-12;

} // namespace vx
} // namespace grenade
