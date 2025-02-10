#pragma once
#include "dapr/map.h"
#include "grenade/common/data.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/port_on_topology.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/vertex.h"
#include "grenade/common/vertex_on_topology.h"

namespace cereal {
struct access;
} // namespace cereal


namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct Topology;


/**
 * InputData of topology.
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE InputData : public Data
{
	typedef dapr::Map<TimeDomainOnTopology, TimeDomainRuntimes> TimeDomainRuntimesMap
	    GENPYBIND(opaque(false));

	/**
	 * Runtimes for each time domain on which vertices are executed.
	 */
	TimeDomainRuntimesMap time_domain_runtimes;

	/**
	 * Number of batch entries for which input data is given.
	 * @throws std::runtime_error On batch size of contained elements not homogeneous.
	 */
	virtual size_t batch_size() const override;

	/**
	 * Get whether input data is valid for topology.
	 * This is the case exactly if input data is given for each port on each vertex in the topology
	 * which requires dynamics, they are valid for the port and for all time domains their
	 * runtimes are present.
	 * @param topology Topology to check against
	 */
	bool valid(Topology const& topology) const;

	bool operator==(InputData const& other) const = default;
	bool operator!=(InputData const& other) const = default;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, InputData const& data) SYMBOL_VISIBLE;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t version);
};

} // namespace common
} // namespace grenade
