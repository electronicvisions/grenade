#pragma once
#include "dapr/property.h"
#include "grenade/common/genpybind.h"
#include "hate/visibility.h"

namespace cereal {
struct access;
} // namespace cereal


namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Data supplied to an input port or generated from an output port of a vertex.
 * Data is separated from the topology description.
 */
struct GENPYBIND(inline_base("*")) SYMBOL_VISIBLE PortData : public dapr::Property<PortData>
{
	virtual ~PortData();

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive&, std::uint32_t);
};

} // namespace common
} // namespace grenade
