#pragma once
#include "dapr/map.h"
#include "dapr/property.h"
#include "dapr/property_holder.h"
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/data.h"
#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_on_executor.h"
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
	 * Executor-specific results.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) Executor : public dapr::Property<Executor>
	{
		virtual ~Executor();
	};

	/**
	 * Execution instance-specific results.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) ExecutionInstance
	    : public dapr::Property<ExecutionInstance>
	{
		virtual ~ExecutionInstance();
	};

	GENPYBIND(return_value_policy(reference_internal))
	Executor const& get_executor() const;
	void set_executor(Executor const& global);
	bool has_executor() const;

	typedef dapr::Map<ExecutionInstanceOnExecutor, ExecutionInstance> ExecutionInstances
	    GENPYBIND(opaque(false));
	ExecutionInstances execution_instances;

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
	dapr::PropertyHolder<Executor> m_executor;
};

} // namespace common
} // namespace grenade
