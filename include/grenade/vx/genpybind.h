#pragma once
#include <genpybind.h>

#define GENPYBIND_TAG_GRENADE_VX GENPYBIND(tag(grenade_vx))
#define GENPYBIND_TAG_GRENADE_VX_EXECUTION GENPYBIND(tag(grenade_vx_execution))
#define GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW GENPYBIND(tag(grenade_vx_signal_flow))
#define GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_ATOMIC                                             \
	GENPYBIND(tag(grenade_vx_network_placed_atomic))
#define GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL                                            \
	GENPYBIND(tag(grenade_vx_network_placed_logical))
#define GENPYBIND_MODULE GENPYBIND(module)
