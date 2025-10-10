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
 * Type of signal this port of a vertex handles.
 */
struct GENPYBIND(inline_base("*")) SYMBOL_VISIBLE VertexPortType
    : public dapr::Property<VertexPortType>
{
	virtual ~VertexPortType();

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace common
} // namespace grenade
