#include "grenade/common/inter_topology_hyper_edge/identity.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <gtest/gtest.h>


using namespace grenade::common;

namespace grenade_common_inter_topology_hyper_edge_identity_tests {

struct DummyVertexPortType : public EmptyVertexPortType<DummyVertexPortType>
{};


struct DummyDefaultVertex : public Vertex
{
	Port input_port;
	Port output_port;
	std::optional<TimeDomainOnTopology> time_domain;

	DummyDefaultVertex() = default;
	DummyDefaultVertex(Port in, Port out, std::optional<TimeDomainOnTopology> time_domain) :
	    input_port(in), output_port(out), time_domain(time_domain)
	{
	}

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

} // namespace grenade_common_inter_topology_hyper_edge_identity_tests

using namespace grenade_common_inter_topology_hyper_edge_identity_tests;


TEST(IdentityInterTopologyHyperEdge, General)
{
	DummyDefaultVertex vertex;
	DummyDefaultVertex::Parameterization parameterization;
	DummyDefaultVertex::Results results(2);

	IdentityInterTopologyHyperEdge value;

	auto const reference_topology = std::make_shared<Topology>();
	auto const reference_vertex_descriptor = reference_topology->add_vertex(vertex);


	LinkedTopology linked_topology(reference_topology);
	auto const linked_vertex_descriptor = linked_topology.add_vertex(vertex);

	EXPECT_TRUE(
	    (value.valid({linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology)));

	// vertex count needs to match for identity to work
	EXPECT_FALSE((value.valid(
	    {linked_vertex_descriptor, linked_vertex_descriptor}, {reference_vertex_descriptor},
	    linked_topology)));

	EXPECT_EQ(
	    value
	        .map_input_data(
	            {{parameterization}}, {linked_vertex_descriptor}, {reference_vertex_descriptor},
	            linked_topology)
	        .size(),
	    1);
	EXPECT_EQ(
	    value
	        .map_input_data(
	            {{parameterization}}, {linked_vertex_descriptor}, {reference_vertex_descriptor},
	            linked_topology)
	        .at(0)
	        .size(),
	    1);
	EXPECT_TRUE(value
	                .map_input_data(
	                    {{parameterization}}, {linked_vertex_descriptor},
	                    {reference_vertex_descriptor}, linked_topology)
	                .at(0)
	                .at(0));
	EXPECT_EQ(
	    *value
	         .map_input_data(
	             {{parameterization}}, {linked_vertex_descriptor}, {reference_vertex_descriptor},
	             linked_topology)
	         .at(0)
	         .at(0),
	    parameterization);
	EXPECT_FALSE(value
	                 .map_input_data(
	                     {{std::nullopt}}, {linked_vertex_descriptor},
	                     {reference_vertex_descriptor}, linked_topology)
	                 .at(0)
	                 .at(0));

	EXPECT_EQ(
	    value
	        .map_output_data(
	            {{results}}, {linked_vertex_descriptor}, {reference_vertex_descriptor},
	            linked_topology)
	        .size(),
	    1);
	EXPECT_EQ(
	    value
	        .map_output_data(
	            {{results}}, {linked_vertex_descriptor}, {reference_vertex_descriptor},
	            linked_topology)
	        .at(0)
	        .size(),
	    1);
	EXPECT_TRUE(value
	                .map_output_data(
	                    {{results}}, {linked_vertex_descriptor}, {reference_vertex_descriptor},
	                    linked_topology)
	                .at(0)
	                .at(0));
	EXPECT_EQ(
	    *value
	         .map_output_data(
	             {{results}}, {linked_vertex_descriptor}, {reference_vertex_descriptor},
	             linked_topology)
	         .at(0)
	         .at(0),
	    results);
	EXPECT_FALSE(value
	                 .map_output_data(
	                     {{std::nullopt}}, {linked_vertex_descriptor},
	                     {reference_vertex_descriptor}, linked_topology)
	                 .at(0)
	                 .at(0));

	EXPECT_EQ(value, value);
	std::stringstream ss;
	ss << value;
	EXPECT_EQ(ss.str(), "IdentityInterTopologyHyperEdge()");
}
