#pragma once
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <iosfwd>

namespace grenade::vx {
namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct GENPYBIND(visible) Receptor
{
	/** Receptor type. */
	enum class Type
	{
		excitatory,
		inhibitory
	};
};

std::ostream& operator<<(std::ostream& os, Receptor::Type const& receptor_type) SYMBOL_VISIBLE;

} // namespace network
} // namespace grenade::vx
