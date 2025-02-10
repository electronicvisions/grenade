#include "grenade/common/input_data.h"

#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex.h"
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
	std::optional<TimeDomainOnTopology> time_domain;
	bool valid_input_data{true};

	DummyDefaultVertex(std::optional<TimeDomainOnTopology> time_domain) : time_domain(time_domain)
	{
	}

	virtual std::vector<Port> get_input_ports() const override
	{
		return {};
	}

	virtual std::vector<Port> get_output_ports() const override
	{
		return {};
	}

	struct InputData : public BatchedPortData
	{
		size_t m_batch_size;

		InputData(size_t batch_size) : m_batch_size(batch_size) {}

		virtual size_t batch_size() const override
		{
			return m_batch_size;
		}

		virtual std::unique_ptr<PortData> copy() const override
		{
			return std::make_unique<InputData>(*this);
		}

		virtual std::unique_ptr<PortData> move() override
		{
			return std::make_unique<InputData>(std::move(*this));
		}

	protected:
		virtual bool is_equal_to(grenade::common::PortData const& other) const override
		{
			return m_batch_size == static_cast<InputData const&>(other).m_batch_size;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}
	};

	virtual bool valid_input_port_data(size_t, PortData const& input_data) const override
	{
		return dynamic_cast<InputData const*>(&input_data) != nullptr && valid_input_data;
	}

	virtual std::optional<TimeDomainOnTopology> get_time_domain() const override
	{
		return time_domain;
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


struct DummyTimeDomainRuntimes : public TimeDomainRuntimes
{
	size_t m_batch_size;

	DummyTimeDomainRuntimes(size_t batch_size) : m_batch_size(batch_size) {}

	virtual size_t batch_size() const override
	{
		return m_batch_size;
	}

	virtual std::unique_ptr<TimeDomainRuntimes> copy() const override
	{
		return std::make_unique<DummyTimeDomainRuntimes>(*this);
	}

	virtual std::unique_ptr<TimeDomainRuntimes> move() override
	{
		return std::make_unique<DummyTimeDomainRuntimes>(std::move(*this));
	}

protected:
	virtual bool is_equal_to(TimeDomainRuntimes const& other) const override
	{
		return m_batch_size == static_cast<DummyTimeDomainRuntimes const&>(other).m_batch_size;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};

} // namespace

TEST(InputData, General)
{
	ListMultiIndexSequence channels_0({MultiIndex({1}), MultiIndex({2})});
	DummyVertexPortType port_type_0;

	Vertex::Port port_0(
	    port_type_0, Vertex::Port::SumOrSplitSupport::yes,
	    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    Vertex::Port::RequiresOrGeneratesData::no, channels_0);

	DummyDefaultVertex vertex{std::nullopt};

	Topology topology;
	auto const vertex_descr = topology.add_vertex(vertex);

	InputData input_data;

	EXPECT_EQ(input_data.batch_size(), 0);

	input_data.ports.set(std::pair{vertex_descr, 0}, DummyDefaultVertex::InputData(1));
	EXPECT_EQ(input_data.batch_size(), 1);
	EXPECT_TRUE(input_data.valid(topology));

	DummyTimeDomainRuntimes time_domain_runtimes(2);
	input_data.time_domain_runtimes.set(TimeDomainOnTopology(0), time_domain_runtimes);
	// batch size not homogeneous
	EXPECT_THROW(input_data.batch_size(), std::runtime_error);
	time_domain_runtimes.m_batch_size = 1;
	// batch size homogeneous
	input_data.time_domain_runtimes.set(TimeDomainOnTopology(0), time_domain_runtimes);
	EXPECT_EQ(input_data.batch_size(), 1);

	EXPECT_TRUE(input_data.valid(topology));

	// time domain runtimes missing for vertex
	vertex.time_domain = TimeDomainOnTopology(1);
	topology.set(vertex_descr, vertex);
	EXPECT_FALSE(input_data.valid(topology));
}
