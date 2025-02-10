#pragma once
#include "dapr/map.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/port_data.h"
#include "grenade/common/port_on_topology.h"
#include "hate/visibility.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct Topology;


/**
 * Data correponding to ports of vertices of topology.
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE Data
{
	typedef dapr::Map<PortOnTopology, PortData> Ports GENPYBIND(opaque(false));

	/**
	 * Data given per port per vertex of topology.
	 */
	Ports ports;

	/**
	 * Number of batch entries for which data is given.
	 * @throws std::runtime_error On batch size of contained elements not homogeneous.
	 */
	virtual size_t batch_size() const;

	bool operator==(Data const& other) const = default;
	bool operator!=(Data const& other) const = default;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, Data const& data) SYMBOL_VISIBLE;

protected:
	bool has_homogeneous_batch_size() const;
};

} // namespace common
} // namespace grenade
