#pragma once
#include "dapr/map.h"
#include "dapr/property.h"
#include "dapr/property_holder.h"
#include "grenade/common/data.h"
#include "grenade/common/edge.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/port_on_topology.h"
#include "grenade/common/vertex.h"
#include "grenade/common/vertex_on_topology.h"

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct Topology;


/**
 * OutputData of an experiment execution.
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE OutputData : public Data
{
	/**
	 * Global backend-specific results.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) Global : public dapr::Property<Global>
	{
		virtual ~Global();

		virtual void merge(Global& other) = 0;
		virtual void merge(Global&& other) GENPYBIND(hidden) = 0;
	};

	GENPYBIND(return_value_policy(reference_internal))
	Global const& get_global() const;
	void set_global(Global const& global);
	bool has_global() const;

	/**
	 * Get whether output data is valid for topology.
	 * @param topology Topology to check against
	 */
	bool valid(Topology const& topology) const;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, OutputData const& value) SYMBOL_VISIBLE;

	void merge(OutputData& other) SYMBOL_VISIBLE;
	void merge(OutputData&& other) GENPYBIND(hidden) SYMBOL_VISIBLE;

	bool operator==(OutputData const& other) const = default;
	bool operator!=(OutputData const& other) const = default;

private:
	dapr::PropertyHolder<Global> m_global;
};

} // namespace common
} // namespace grenade
