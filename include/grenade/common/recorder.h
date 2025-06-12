#pragma once
#include "dapr/property_holder.h"
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "hate/visibility.h"
#include <optional>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Recorder in topology.
 * A recorder defines a set of channels for which it expects signals , which are present during a
 * time domain to record as input and (optionally) yields recorded data as results (output port
 * index 0).
 */
struct SYMBOL_VISIBLE GENPYBIND(visible, holder_type("std::shared_ptr<grenade::common::Recorder>"))
    Recorder : public PartitionedVertex
{
	/**
	 * Results containing recorded data.
	 */
	struct Results : public BatchedPortData
	{
		/**
		 * Set section of results.
		 * The sequence is required to be included in the interval [0, size()).
		 */
		virtual void set_section(Results const& results, MultiIndexSequence const& sequence) = 0;

		/**
		 * Number of results per batch entry.
		 */
		virtual size_t size() const = 0;
	};

	/**
	 * Construct recorder with specified number of channels and time-domain.
	 * @param shape Shape of source channels to record
	 * @param time_domain Time domain onto which to place recorder
	 * @param execution_instance_on_executor Execution instance on executor onto which to partition
	 * recorder
	 */
	Recorder(
	    MultiIndexSequence const& shape,
	    TimeDomainOnTopology const& time_domain,
	    std::optional<ExecutionInstanceOnExecutor> const& execution_instance_on_executor =
	        std::nullopt);

	MultiIndexSequence const& get_shape() const GENPYBIND(hidden);

	GENPYBIND_MANUAL({
		parent.def(
		    "get_shape", [](GENPYBIND_PARENT_TYPE const& self) { return self.get_shape().copy(); });
	})

	void set_shape(MultiIndexSequence const& value);
	void set_shape(MultiIndexSequence&& value) GENPYBIND(hidden);

	/**
	 * Get time domain.
	 * A recorder is always part of the time domain from which it records signals.
	 */
	virtual std::optional<TimeDomainOnTopology> get_time_domain() const override;

	/**
	 * Set time domain.
	 */
	void set_time_domain(TimeDomainOnTopology const& value);

	/**
	 * Get section of recorder.
	 * The sequence is required to be included in the shape of the recorder.
	 */
	std::unique_ptr<Recorder> get_section(MultiIndexSequence const& sequence) const;

	/**
	 * Create empty results of recorder.
	 * The results can then be filled e.g. using `set_section`.
	 */
	virtual std::unique_ptr<Results> create_empty_results(size_t batch_size) const;

protected:
	bool is_equal_to(Vertex const& other) const override;
	std::ostream& print(std::ostream& os) const override;

	dapr::PropertyHolder<MultiIndexSequence> m_shape;
	TimeDomainOnTopology m_time_domain;
};

} // namespace common
} // namespace grenade
