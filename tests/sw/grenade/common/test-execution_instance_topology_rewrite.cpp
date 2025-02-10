#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/topology_rewrite/execution_instance.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/inter_topology_hyper_edge/identity.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/population.h"
#include "grenade/common/resource_estimator.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <memory>
#include <stdexcept>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace {

struct DummyVertex : public PartitionedVertex
{
	size_t resources;
	std::optional<TimeDomainOnTopology> time_domain;
	grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint
	    input_transition_constraint{
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported};
	grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint
	    output_transition_constraint{
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported};

	DummyVertex(
	    size_t resources,
	    std::optional<TimeDomainOnTopology> time_domain,
	    ConnectionOnExecutor connection_on_executor) :
	    PartitionedVertex(
	        ExecutionInstanceOnExecutor(ExecutionInstanceID(), connection_on_executor)),
	    resources(resources),
	    time_domain(time_domain)
	{
	}

	struct PortType : public EmptyVertexPortType<PortType>
	{};


	virtual std::optional<TimeDomainOnTopology> get_time_domain() const override
	{
		return time_domain;
	}

	virtual std::vector<Port> get_input_ports() const override
	{
		return {Port(
		    PortType(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
		    input_transition_constraint, grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
		    grenade::common::CuboidMultiIndexSequence({1}))};
	}

	virtual std::vector<Port> get_output_ports() const override
	{
		return {Port(
		    PortType(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
		    output_transition_constraint,
		    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
		    grenade::common::CuboidMultiIndexSequence({1}))};
	}

	virtual std::unique_ptr<Vertex> copy() const override
	{
		return std::make_unique<DummyVertex>(*this);
	}

	virtual std::unique_ptr<Vertex> move() override
	{
		return std::make_unique<DummyVertex>(std::move(*this));
	}

	virtual bool is_equal_to(Vertex const& other) const override
	{
		return resources == static_cast<DummyVertex const&>(other).resources &&
		       time_domain == static_cast<DummyVertex const&>(other).time_domain &&
		       PartitionedVertex::is_equal_to(other);
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};


struct DummyResource : public ResourceEstimator::Resource
{
	size_t resources;

	DummyResource() = default;
	DummyResource(size_t resources) : resources(resources) {}

	virtual Resource& operator+=(Resource const& other) override
	{
		auto const& other_resource = dynamic_cast<DummyResource const&>(other);
		resources += other_resource.resources;
		return *this;
	}

	virtual Resource& operator*=(size_t factor) override
	{
		resources *= factor;
		return *this;
	}

	virtual std::vector<size_t> scalar_values() const override
	{
		return {resources};
	}

	virtual std::unique_ptr<Resource> subsequence(MultiIndexSequence const&) const override
	{
		throw std::invalid_argument("DummyResource not subsequenceable.");
	}

	virtual std::unique_ptr<Resource> copy() const override
	{
		return std::make_unique<DummyResource>(*this);
	}

	virtual std::unique_ptr<Resource> move() override
	{
		return std::make_unique<DummyResource>(std::move(*this));
	}

	virtual bool is_equal_to(Resource const& other) const override
	{
		return resources == static_cast<DummyResource const&>(other).resources;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};


struct DummyResourceEstimator : ResourceEstimator
{
	virtual std::unique_ptr<ResourceEstimator> copy() const
	{
		return std::make_unique<DummyResourceEstimator>(*this);
	}

	DummyResourceEstimator(Topology const& topology) : ResourceEstimator(topology) {}

	virtual std::unique_ptr<Resource> operator()(VertexOnTopology const& vertex_descriptor) const
	{
		auto const& dummy_vertex =
		    dynamic_cast<DummyVertex const&>(m_topology.get(vertex_descriptor));
		return std::make_unique<DummyResource>(dummy_vertex.resources);
	}
};

} // namespace

TEST(ExecutionInstanceTopologyRewrite, General)
{
	ExecutionInstanceTopologyRewrite::SystemResources system_resources{
	    {ConnectionOnExecutor(1), DummyResource(3)}, {ConnectionOnExecutor(2), DummyResource(4)}};

	// unconnected without time domain
	{
		auto topology = std::make_shared<Topology>();
		auto linked_topology = std::make_shared<LinkedTopology>(topology);

		auto const vertex_descriptor_0 =
		    linked_topology->add_vertex(DummyVertex(1, std::nullopt, ConnectionOnExecutor(1)));

		auto const vertex_descriptor_1 =
		    linked_topology->add_vertex(DummyVertex(2, std::nullopt, ConnectionOnExecutor(1)));

		auto const vertex_descriptor_2 =
		    linked_topology->add_vertex(DummyVertex(2, std::nullopt, ConnectionOnExecutor(1)));

		DummyResourceEstimator dummy_resource_estimator(*linked_topology);
		ExecutionInstanceTopologyRewrite rewrite(
		    dummy_resource_estimator, system_resources, linked_topology);
		rewrite();

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_0))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(1), ConnectionOnExecutor(1)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_1))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(1), ConnectionOnExecutor(1)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_2))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(0), ConnectionOnExecutor(1)));
	}

	// unconnected with time domain
	{
		auto topology = std::make_shared<Topology>();
		auto linked_topology = std::make_shared<LinkedTopology>(topology);

		auto const vertex_descriptor_0 = linked_topology->add_vertex(
		    DummyVertex(1, TimeDomainOnTopology(0), ConnectionOnExecutor(1)));

		auto const vertex_descriptor_1 = linked_topology->add_vertex(
		    DummyVertex(2, TimeDomainOnTopology(1), ConnectionOnExecutor(1)));

		auto const vertex_descriptor_2 = linked_topology->add_vertex(
		    DummyVertex(2, TimeDomainOnTopology(0), ConnectionOnExecutor(1)));

		DummyResourceEstimator dummy_resource_estimator(*linked_topology);
		ExecutionInstanceTopologyRewrite rewrite(
		    dummy_resource_estimator, system_resources, linked_topology);
		rewrite();

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_0))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(0), ConnectionOnExecutor(1)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_1))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(1), ConnectionOnExecutor(1)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_2))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(0), ConnectionOnExecutor(1)));
	}

	// connected with time domain without transition support
	{
		auto topology = std::make_shared<Topology>();
		auto linked_topology = std::make_shared<LinkedTopology>(topology);

		auto const vertex_descriptor_0 = linked_topology->add_vertex(
		    DummyVertex(2, TimeDomainOnTopology(0), ConnectionOnExecutor(1)));

		auto const vertex_descriptor_1 = linked_topology->add_vertex(
		    DummyVertex(1, TimeDomainOnTopology(0), ConnectionOnExecutor(1)));

		DummyVertex dummy_vertex_without_input_transition_support(
		    1, TimeDomainOnTopology(0), ConnectionOnExecutor(1));
		dummy_vertex_without_input_transition_support.input_transition_constraint =
		    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported;
		auto const vertex_descriptor_2 =
		    linked_topology->add_vertex(dummy_vertex_without_input_transition_support);

		linked_topology->add_edge(
		    vertex_descriptor_0, vertex_descriptor_1,
		    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1})));

		linked_topology->add_edge(
		    vertex_descriptor_0, vertex_descriptor_2,
		    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1})));

		DummyResourceEstimator dummy_resource_estimator(*linked_topology);
		ExecutionInstanceTopologyRewrite rewrite(
		    dummy_resource_estimator, system_resources, linked_topology);
		rewrite();

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_0))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(0), ConnectionOnExecutor(1)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_1))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(1), ConnectionOnExecutor(1)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_2))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(0), ConnectionOnExecutor(1)));
	}

	// connected with time domain too large for connection
	{
		auto topology = std::make_shared<Topology>();
		auto linked_topology = std::make_shared<LinkedTopology>(topology);

		auto const vertex_descriptor_0 = linked_topology->add_vertex(
		    DummyVertex(2, TimeDomainOnTopology(0), ConnectionOnExecutor(1)));

		DummyVertex dummy_vertex_without_input_transition_support(
		    2, TimeDomainOnTopology(0), ConnectionOnExecutor(1));
		dummy_vertex_without_input_transition_support.input_transition_constraint =
		    Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported;
		auto const vertex_descriptor_1 =
		    linked_topology->add_vertex(dummy_vertex_without_input_transition_support);

		linked_topology->add_edge(
		    vertex_descriptor_0, vertex_descriptor_1,
		    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1})));

		DummyResourceEstimator dummy_resource_estimator(*linked_topology);
		ExecutionInstanceTopologyRewrite rewrite(
		    dummy_resource_estimator, system_resources, linked_topology);
		EXPECT_THROW(rewrite(), std::runtime_error);
	}

	// connected cycle with time domain
	{
		auto topology = std::make_shared<Topology>();
		auto linked_topology = std::make_shared<LinkedTopology>(topology);

		auto const vertex_descriptor_0 = linked_topology->add_vertex(
		    DummyVertex(2, TimeDomainOnTopology(0), ConnectionOnExecutor(1)));

		auto const vertex_descriptor_1 = linked_topology->add_vertex(
		    DummyVertex(1, TimeDomainOnTopology(0), ConnectionOnExecutor(1)));

		auto const vertex_descriptor_2 = linked_topology->add_vertex(
		    DummyVertex(1, TimeDomainOnTopology(0), ConnectionOnExecutor(1)));

		linked_topology->add_edge(
		    vertex_descriptor_0, vertex_descriptor_1,
		    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1})));

		linked_topology->add_edge(
		    vertex_descriptor_0, vertex_descriptor_2,
		    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1})));

		linked_topology->add_edge(
		    vertex_descriptor_2, vertex_descriptor_0,
		    Edge(CuboidMultiIndexSequence({1}), CuboidMultiIndexSequence({1})));

		DummyResourceEstimator dummy_resource_estimator(*linked_topology);
		ExecutionInstanceTopologyRewrite rewrite(
		    dummy_resource_estimator, system_resources, linked_topology);
		rewrite();

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_0))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(0), ConnectionOnExecutor(1)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_1))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(1), ConnectionOnExecutor(1)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_2))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(0), ConnectionOnExecutor(1)));
	}

	// two connections
	{
		auto topology = std::make_shared<Topology>();
		auto linked_topology = std::make_shared<LinkedTopology>(topology);

		auto const vertex_descriptor_0 = linked_topology->add_vertex(
		    DummyVertex(1, TimeDomainOnTopology(), ConnectionOnExecutor(1)));

		auto const vertex_descriptor_1 = linked_topology->add_vertex(
		    DummyVertex(2, TimeDomainOnTopology(), ConnectionOnExecutor(2)));

		auto const vertex_descriptor_2 = linked_topology->add_vertex(
		    DummyVertex(2, TimeDomainOnTopology(), ConnectionOnExecutor(1)));

		DummyResourceEstimator dummy_resource_estimator(*linked_topology);
		ExecutionInstanceTopologyRewrite rewrite(
		    dummy_resource_estimator, system_resources, linked_topology);
		rewrite();

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_0))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(0), ConnectionOnExecutor(1)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_1))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(1), ConnectionOnExecutor(2)));

		EXPECT_EQ(
		    dynamic_cast<PartitionedVertex const&>(linked_topology->get(vertex_descriptor_2))
		        .get_execution_instance_on_executor(),
		    ExecutionInstanceOnExecutor(ExecutionInstanceID(0), ConnectionOnExecutor(1)));
	}
}
