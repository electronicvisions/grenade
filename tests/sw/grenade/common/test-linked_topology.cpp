#include "grenade/common/linked_topology.h"

#include "grenade/common/input_data.h"
#include "grenade/common/inter_topology_hyper_edge/identity.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/output_data.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <gtest/gtest.h>


using namespace grenade::common;

namespace grenade_common_linked_topology_tests {

struct DummyVertexPortType : public EmptyVertexPortType<DummyVertexPortType>
{};


struct DummyDefaultVertex : public Vertex
{
	Port input_port;
	Port output_port;
	std::optional<TimeDomainOnTopology> time_domain;

	DummyDefaultVertex() = default;
	DummyDefaultVertex(Port in, Port out) : input_port(in), output_port(out) {}

	struct Parameterization : public PortData
	{
		Parameterization() = default;

		virtual std::unique_ptr<PortData> copy() const override
		{
			return std::make_unique<Parameterization>(*this);
		}

		virtual std::unique_ptr<PortData> move() override
		{
			return std::make_unique<Parameterization>(std::move(*this));
		}

	protected:
		virtual bool is_equal_to(grenade::common::PortData const&) const override
		{
			return true;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}
	};

	virtual bool valid_input_port_data(size_t, PortData const& parameterization) const override
	{
		return dynamic_cast<Parameterization const*>(&parameterization) != nullptr;
	}

	struct Results : public BatchedPortData
	{
		size_t m_batch_size;

		Results(size_t batch_size) : m_batch_size(batch_size) {}

		virtual size_t batch_size() const override
		{
			return m_batch_size;
		}

		virtual std::unique_ptr<PortData> copy() const override
		{
			return std::make_unique<Results>(*this);
		}

		virtual std::unique_ptr<PortData> move() override
		{
			return std::make_unique<Results>(std::move(*this));
		}

	protected:
		virtual bool is_equal_to(grenade::common::PortData const& other) const override
		{
			return m_batch_size == static_cast<Results const&>(other).m_batch_size;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}
	};

	virtual bool valid_output_port_data(size_t, PortData const& results) const override
	{
		return dynamic_cast<Results const*>(&results) != nullptr;
	}

	virtual std::optional<TimeDomainOnTopology> get_time_domain() const override
	{
		return time_domain;
	}

	virtual std::vector<Port> get_input_ports() const override
	{
		return {input_port};
	}

	virtual std::vector<Port> get_output_ports() const override
	{
		return {output_port};
	}

	virtual std::unique_ptr<Vertex> copy() const override
	{
		return std::make_unique<DummyDefaultVertex>(*this);
	}

	virtual std::unique_ptr<Vertex> move() override
	{
		return std::make_unique<DummyDefaultVertex>(std::move(*this));
	}

	virtual bool is_equal_to(Vertex const& other) const override
	{
		return input_port == static_cast<DummyDefaultVertex const&>(other).input_port &&
		       output_port == static_cast<DummyDefaultVertex const&>(other).output_port;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};

} // namespace grenade_common_linked_topology_tests

using namespace grenade_common_linked_topology_tests;


TEST(LinkedTopology, General)
{
	ListMultiIndexSequence channels({MultiIndex({1}), MultiIndex({2})});
	DummyVertexPortType port_type;

	Vertex::Port port(
	    port_type, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::yes, channels);

	DummyDefaultVertex vertex(port, port);

	InputData input_data;
	OutputData output_data;

	auto const reference_topology = std::make_shared<Topology>();
	auto const reference_vertex_descriptor = reference_topology->add_vertex(vertex);

	input_data.ports.set(
	    std::pair{reference_vertex_descriptor, 0}, DummyDefaultVertex::Parameterization());

	std::shared_ptr<LinkedTopology> linked_topology =
	    std::make_shared<LinkedTopology>(reference_topology);
	auto const linked_vertex_descriptor = linked_topology->add_vertex(vertex);

	output_data.ports.set(std::pair{linked_vertex_descriptor, 0}, DummyDefaultVertex::Results(2));

	linked_topology->add_inter_graph_hyper_edge(
	    {linked_vertex_descriptor}, {reference_vertex_descriptor},
	    IdentityInterTopologyHyperEdge());

	EXPECT_TRUE(linked_topology->valid());

	LinkedTopology linked_linked_topology(linked_topology);
	auto const linked_linked_vertex_descriptor = linked_linked_topology.add_vertex(vertex);

	linked_linked_topology.add_inter_graph_hyper_edge(
	    {linked_linked_vertex_descriptor}, {linked_vertex_descriptor},
	    IdentityInterTopologyHyperEdge());

	EXPECT_EQ(linked_topology->map_reference_input_data(input_data), input_data);
	EXPECT_EQ(linked_linked_topology.map_root_input_data(input_data), input_data);

	EXPECT_EQ(linked_topology->map_reference_output_data(output_data), output_data);
	EXPECT_EQ(linked_linked_topology.map_root_output_data(output_data), output_data);

	EXPECT_THROW(linked_topology->map_reference_input_data(InputData()), std::invalid_argument);
	EXPECT_THROW(linked_linked_topology.map_root_input_data(InputData()), std::invalid_argument);

	EXPECT_THROW(linked_topology->map_reference_output_data(OutputData()), std::invalid_argument);
	EXPECT_THROW(linked_linked_topology.map_root_output_data(OutputData()), std::invalid_argument);
}
