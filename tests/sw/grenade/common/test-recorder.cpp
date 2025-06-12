#include "grenade/common/multi_index.h"
#include "grenade/common/recorder.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace {

struct DummyRecorder : public Recorder
{
	DummyRecorder(
	    MultiIndexSequence const& shape,
	    TimeDomainOnTopology time_domain,
	    std::optional<ExecutionInstanceOnExecutor> execution_instance_on_executor) :
	    Recorder(shape, time_domain, execution_instance_on_executor)
	{
	}

	struct Results : public Recorder::Results
	{
		Results(size_t size, size_t batch_size) : m_size(size), m_batch_size(batch_size) {}

		virtual size_t size() const override
		{
			return m_size;
		}

		virtual size_t batch_size() const override
		{
			return m_batch_size;
		}

		virtual void set_section(Recorder::Results const&, MultiIndexSequence const&) override {}

		virtual std::unique_ptr<PortData> copy() const override
		{
			return std::make_unique<Results>(*this);
		}

		virtual std::unique_ptr<PortData> move() override
		{
			return std::make_unique<Results>(std::move(*this));
		}

	protected:
		virtual bool is_equal_to(PortData const& other) const override
		{
			return m_size == static_cast<Results const&>(other).m_size &&
			       m_batch_size == static_cast<Results const&>(other).m_batch_size;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}

	private:
		size_t m_size;
		size_t m_batch_size;
	};

	std::unique_ptr<Recorder::Results> create_empty_results(size_t batch_size) const override
	{
		return std::make_unique<Results>(0, batch_size);
	}

	virtual std::vector<Vertex::Port> get_input_ports() const override
	{
		return {};
	}

	virtual std::vector<Vertex::Port> get_output_ports() const override
	{
		return {};
	}

	virtual bool valid(TimeDomainRuntimes const&) const override
	{
		return false;
	}

	virtual std::unique_ptr<Vertex> copy() const override
	{
		return std::make_unique<DummyRecorder>(*this);
	}

	virtual std::unique_ptr<Vertex> move() override
	{
		return std::make_unique<DummyRecorder>(std::move(*this));
	}

	virtual bool is_equal_to(Vertex const& other) const override
	{
		return Recorder::is_equal_to(other);
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};

} // namespace

TEST(Recorder, General)
{
	CuboidMultiIndexSequence dummy_recorder_shape({4}, MultiIndex({3}));

	DummyRecorder dummy_recorder(
	    dummy_recorder_shape, TimeDomainOnTopology(1), ExecutionInstanceOnExecutor());

	EXPECT_EQ(dummy_recorder.get_shape(), dummy_recorder_shape);
	dummy_recorder.set_shape(CuboidMultiIndexSequence({4}, MultiIndex({5})));
	EXPECT_EQ(dummy_recorder.get_shape(), CuboidMultiIndexSequence({4}, MultiIndex({5})));

	EXPECT_EQ(dummy_recorder.get_time_domain(), TimeDomainOnTopology(1));

	dummy_recorder.set_time_domain(TimeDomainOnTopology(2));
	EXPECT_EQ(dummy_recorder.get_time_domain(), TimeDomainOnTopology(2));

	DummyRecorder::Results dummy_results(0, 2);

	auto const empty_results = dummy_recorder.create_empty_results(2);
	EXPECT_TRUE(empty_results);
	EXPECT_EQ(*empty_results, dummy_results);
}
