#pragma once
#include "dapr/property_holder.h"
#include "grenade/common/resource_estimator.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Resource estimator for population resources.
 */
struct SYMBOL_VISIBLE PopulationResourceEstimator : public grenade::common::ResourceEstimator
{
	struct Resource : public ResourceEstimator::Resource
	{
		size_t neuron_circuit_count;
		size_t background_circuit_count;
		/**
		 * Number of channels used per neuron on the resource.
		 * This is required in order to correctly perform subsequencing of the resource.
		 */
		std::vector<size_t> madc_channel_usage;

		/**
		 * Construct resource.
		 *
		 * @param neuron_circuit_count Number of neuron circuits required by resource.
		 * @param background_circuit_count Number of background generators required by resource
		 * @param madc_channel_usage Number of MADC channels used per neuron on the resource
		 * @param sequence Sequence of neurons represented by resource
		 */
		Resource(
		    size_t neuron_circuit_count,
		    size_t background_circuit_count,
		    std::vector<size_t> madc_channel_usage,
		    dapr::PropertyHolder<grenade::common::MultiIndexSequence> sequence =
		        dapr::PropertyHolder<grenade::common::MultiIndexSequence>());

		/**
		 * Get resource usage of sub-sequence of this resource.
		 */
		virtual std::unique_ptr<ResourceEstimator::Resource> subsequence(
		    grenade::common::MultiIndexSequence const& sequence) const override;

		virtual ResourceEstimator::Resource& operator+=(
		    ResourceEstimator::Resource const& other) override;
		virtual ResourceEstimator::Resource& operator*=(size_t factor) override;
		virtual std::vector<size_t> scalar_values() const override;

		virtual std::unique_ptr<ResourceEstimator::Resource> copy() const override;
		virtual std::unique_ptr<ResourceEstimator::Resource> move() override;

	protected:
		std::ostream& print(std::ostream& os) const override;
		bool is_equal_to(ResourceEstimator::Resource const& other) const override;
		dapr::PropertyHolder<grenade::common::MultiIndexSequence> m_sequence;
	};

	/**
	 * Construct population resource estimator with overestimation factor.
	 *
	 * @param topology Topology for which to estimate resources
	 * @param overestimation Map of scalar factors by which to multiply the estimated resources per
	 * vertex descriptor
	 */
	PopulationResourceEstimator(
	    grenade::common::Topology const& topology,
	    std::map<grenade::common::VertexOnTopology, double> overestimation);

	virtual std::unique_ptr<ResourceEstimator::Resource> operator()(
	    grenade::common::VertexOnTopology const& vertex_descriptor) const override;

	virtual std::unique_ptr<ResourceEstimator> copy() const override;

private:
	std::map<grenade::common::VertexOnTopology, double> m_overestimation;
};

} // namespace abstract
} // namespace grenade::vx::network
