#pragma once
#include "grenade/common/genpybind.h"
#include "grenade/common/port_data.h"
#include "hate/visibility.h"

namespace cereal {
struct access;
} // namespace cereal


namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Data with batch dimension supplied to an input port or generated from an output port of a vertex.
 * Data is separated from the topology description.
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE BatchedPortData : public PortData
{
	virtual size_t batch_size() const = 0;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive&, std::uint32_t);
};

} // namespace common
} // namespace grenade
