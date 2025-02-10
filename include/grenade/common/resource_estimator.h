#pragma once
#include "dapr/copyable.h"
#include "dapr/property.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/vertex_on_topology.h"
#include <memory>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct Vertex;
struct Topology;


/**
 * Estimator for required resources of vertices in topologies.
 * To be used in partitioning.
 */
struct SYMBOL_VISIBLE ResourceEstimator : public dapr::Copyable<ResourceEstimator>
{
	/**
	 * Resource information for a vertex in the topology.
	 */
	struct Resource : public dapr::Property<Resource>
	{
		virtual ~Resource();

		/**
		 * Perform += operation on a resource.
		 * This performs += operation on all scalar parts of the resource.
		 */
		virtual Resource& operator+=(Resource const& other) GENPYBIND(hidden) = 0;

		/**
		 * Scale a resource information by a scalar factor in-place.
		 * This performs multiplicative scaling on all scalar parts of the resource.
		 */
		virtual Resource& operator*=(size_t factor) GENPYBIND(hidden) = 0;

		/**
		 * Perform + operation between two resources.
		 * This performs + operation on all scalar parts of the resource.
		 */
		std::unique_ptr<Resource> operator+(Resource const& other) const;
		/**
		 * Scale a resource information by a scalar factor.
		 * This performs multiplicative scaling on all scalar parts of the resource.
		 */
		std::unique_ptr<Resource> operator*(size_t factor) const;

		/**
		 * Get whether any scalar part of this resource is greater than the respective scalar part
		 * of the other resource.
		 */
		bool any_scalar_greater(Resource const& other) const;

		/**
		 * Get the maximal factor of a scalar part of this resource to the respective scalar part of
		 * the other resource.
		 */
		size_t max_scalar_factor(Resource const& other) const;

		/**
		 * Get scalar parts of this resource.
		 */
		virtual std::vector<size_t> scalar_values() const = 0;

		/**
		 * Get resource corresponding to sub-segment of this resource.
		 */
		virtual std::unique_ptr<Resource> subsegment(MultiIndexSequence const& other) const = 0;
	};

	/**
	 * Construct resource estimator for given topology.
	 */
	ResourceEstimator(Topology const& topology);

	/**
	 * Estimate required resources of given vertex in topology.
	 */
	virtual std::unique_ptr<Resource> operator()(
	    VertexOnTopology const& vertex_descriptor) const = 0;

	virtual ~ResourceEstimator() = default;

protected:
	Topology const& m_topology;
};

} // namespace common
} // namespace grenade
