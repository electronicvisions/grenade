#include "grenade/common/output_data.h"

#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <stdexcept>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace {

struct DummyVertexPortType : public EmptyVertexPortType<DummyVertexPortType>
{};

} // namespace


namespace {

struct DummyDefaultVertex : public Vertex
{
	bool valid_output_data{true};
	Port port;

	DummyDefaultVertex(Port port) : port(std::move(port)) {}

	virtual std::vector<Port> get_input_ports() const override
	{
		return {};
	}

	virtual std::vector<Port> get_output_ports() const override
	{
		return {port};
	}

	struct OutputData : public BatchedPortData
	{
		size_t m_batch_size;

		OutputData(size_t batch_size) : m_batch_size(batch_size) {}

		virtual size_t batch_size() const override
		{
			return m_batch_size;
		}

		virtual std::unique_ptr<PortData> copy() const override
		{
			return std::make_unique<OutputData>(*this);
		}

		virtual std::unique_ptr<PortData> move() override
		{
			return std::make_unique<OutputData>(std::move(*this));
		}

	protected:
		virtual bool is_equal_to(grenade::common::PortData const& other) const override
		{
			return m_batch_size == static_cast<OutputData const&>(other).m_batch_size;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}
	};

	virtual bool valid_output_port_data(size_t, PortData const& output_data) const override
	{
		return dynamic_cast<OutputData const*>(&output_data) != nullptr && valid_output_data;
	}

	virtual std::unique_ptr<Vertex> copy() const override
	{
		return std::make_unique<DummyDefaultVertex>(*this);
	}

	virtual std::unique_ptr<Vertex> move() override
	{
		return std::make_unique<DummyDefaultVertex>(std::move(*this));
	}

	virtual bool is_equal_to(Vertex const&) const override
	{
		return false; // unused
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};

} // namespace

TEST(OutputData, General)
{
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	DummyVertexPortType port_type_0;

	Vertex::Port port_0(
	    port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::yes, channels_0);

	DummyDefaultVertex vertex(port_0);

	Topology topology;
	auto const vertex_descr = topology.add_vertex(vertex);
	auto const vertex_descr_2 = topology.add_vertex(vertex);

	OutputData output_data;

	EXPECT_EQ(output_data.batch_size(), 0);

	// vertices generate output_data but not contained in output_data
	EXPECT_FALSE(output_data.valid(topology));

	output_data.ports.set(std::pair{vertex_descr, 0}, DummyDefaultVertex::OutputData(1));
	output_data.ports.set(std::pair{vertex_descr_2, 0}, DummyDefaultVertex::OutputData(1));
	// batch size homogeneous
	EXPECT_EQ(output_data.batch_size(), 1);
	EXPECT_TRUE(output_data.valid(topology));

	// output_data contained but not valid
	vertex.valid_output_data = false;
	topology.set(vertex_descr_2, vertex);
	EXPECT_FALSE(output_data.valid(topology));
	vertex.valid_output_data = true;
	topology.set(vertex_descr_2, vertex);

	// vertex doesn't generate output_data, but output_data are present
	vertex.port.requires_or_generates_data = Vertex::Port::RequiresOrGeneratesData::no;
	topology.set(vertex_descr_2, vertex);
	EXPECT_FALSE(output_data.valid(topology));
	vertex.port.requires_or_generates_data = Vertex::Port::RequiresOrGeneratesData::yes;
	topology.set(vertex_descr_2, vertex);

	// batch size heterogeneous
	output_data.ports.set(std::pair{vertex_descr_2, 0}, DummyDefaultVertex::OutputData(2));
	EXPECT_FALSE(output_data.valid(topology));
	EXPECT_THROW(output_data.batch_size(), std::runtime_error);
}
