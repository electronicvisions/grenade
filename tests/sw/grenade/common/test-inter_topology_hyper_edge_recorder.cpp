#include "grenade/common/recorder.h"

#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/inter_topology_hyper_edge/recorder.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_port_type/empty.h"
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
		std::vector<std::vector<size_t>> value;

		Results(std::vector<std::vector<size_t>> value) : value(std::move(value)) {}

		virtual size_t size() const override
		{
			return value.at(0).size();
		}

		virtual size_t batch_size() const override
		{
			return value.size();
		}

		virtual void set_section(
		    Recorder::Results const& results, MultiIndexSequence const& sequence) override
		{
			auto const* sequence_results = dynamic_cast<Results const*>(&results);
			if (!sequence_results) {
				throw std::invalid_argument("Results type invalid for recorder.");
			}
			auto const sequence_elements = sequence.get_elements();
			for (size_t b = 0; b < batch_size(); ++b) {
				for (size_t i = 0; i < sequence_elements.size(); ++i) {
					value.at(b).at(sequence_elements.at(i).value.at(0)) =
					    sequence_results->value.at(b).at(i);
				}
			}
		}

		virtual std::unique_ptr<PortData> copy() const override
		{
			return std::make_unique<Results>(*this);
		}

		virtual std::unique_ptr<PortData> move() override
		{
			return std::make_unique<Results>(std::move(*this));
		}

		virtual bool is_equal_to(PortData const& other) const override
		{
			return value == static_cast<Results const&>(other).value;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}
	};

	std::unique_ptr<Recorder::Results> create_empty_results(size_t batch_size) const override
	{
		return std::make_unique<Results>(
		    std::vector<std::vector<size_t>>(batch_size, std::vector<size_t>(get_shape().size())));
	}

	struct SourceSignalPortType : public EmptyVertexPortType<SourceSignalPortType>
	{};

	struct ResultsPortType : public EmptyVertexPortType<ResultsPortType>
	{};

	virtual std::vector<Vertex::Port> get_input_ports() const override
	{
		return {Vertex::Port(
		    SourceSignalPortType(), Vertex::Port::SumOrSplitSupport::no,
		    Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		    Vertex::Port::RequiresOrGeneratesData::yes, get_shape())};
	}

	virtual std::vector<Vertex::Port> get_output_ports() const override
	{
		return {Vertex::Port(
		    ResultsPortType(), Vertex::Port::SumOrSplitSupport::yes,
		    Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
		    Vertex::Port::RequiresOrGeneratesData::no, get_shape())};
	}

	virtual bool valid(TimeDomainRuntimes const&) const override
	{
		return true;
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


struct DummyVertex : public Vertex
{
	DummyVertex() = default;

	virtual bool valid_input_port_data(size_t, PortData const&) const override
	{
		return false;
	}

	virtual bool valid_output_port_data(size_t, PortData const&) const override
	{
		return false;
	}

	virtual std::optional<TimeDomainOnTopology> get_time_domain() const override
	{
		return std::nullopt;
	}

	virtual std::vector<Port> get_input_ports() const override
	{
		return {};
	}

	virtual std::vector<Port> get_output_ports() const override
	{
		return {};
	}

	virtual std::unique_ptr<Vertex> copy() const override
	{
		return std::make_unique<DummyVertex>(*this);
	}

	virtual std::unique_ptr<Vertex> move() override
	{
		return std::make_unique<DummyVertex>(std::move(*this));
	}

	virtual bool is_equal_to(Vertex const&) const override
	{
		return true;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};


struct DummyPortData : public PortData
{
	DummyPortData() = default;

	virtual std::unique_ptr<PortData> copy() const override
	{
		return std::make_unique<DummyPortData>(*this);
	}

	virtual std::unique_ptr<PortData> move() override
	{
		return std::make_unique<DummyPortData>(std::move(*this));
	}

	virtual bool is_equal_to(PortData const&) const override
	{
		return true;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};

} // namespace

TEST(RecorderInterTopologyHyperEdge, General)
{
	CuboidMultiIndexSequence dummy_recorder_shape({4}, MultiIndex({3}));

	DummyRecorder recorder(
	    dummy_recorder_shape, TimeDomainOnTopology(1), ExecutionInstanceOnExecutor());

	DummyRecorder::Results results({{1, 2, 3, 4}, {5, 6, 7, 8}});

	RecorderInterTopologyHyperEdge hyper_edge;

	// multiple reference vertices
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor_1 = reference_topology->add_vertex(DummyVertex());
		auto const reference_vertex_descriptor_2 = reference_topology->add_vertex(DummyVertex());

		LinkedTopology linked_topology(reference_topology);
		auto const linked_vertex_descriptor = linked_topology.add_vertex(recorder);

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor},
		    {reference_vertex_descriptor_1, reference_vertex_descriptor_2}, linked_topology));
	}

	// reference vertex type not matching
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor = reference_topology->add_vertex(DummyVertex());

		LinkedTopology linked_topology(reference_topology);
		auto const linked_vertex_descriptor = linked_topology.add_vertex(recorder);

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology));
	}

	// linked vertex type not matching
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor = reference_topology->add_vertex(recorder);

		LinkedTopology linked_topology(reference_topology);
		auto const linked_vertex_descriptor = linked_topology.add_vertex(DummyVertex());

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology));
	}

	// linked recorder shape not included in reference recorder shape
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor = reference_topology->add_vertex(recorder);

		LinkedTopology linked_topology(reference_topology);

		DummyRecorder linked_recorder(
		    CuboidMultiIndexSequence({5}, MultiIndex({6})), TimeDomainOnTopology(), std::nullopt);

		auto const linked_vertex_descriptor = linked_topology.add_vertex(linked_recorder);

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology));
	}

	auto const reference_topology = std::make_shared<Topology>();
	auto const reference_vertex_descriptor = reference_topology->add_vertex(recorder);

	LinkedTopology linked_topology(reference_topology);
	auto const linked_vertex_descriptor_1 = linked_topology.add_vertex(recorder);
	auto const linked_vertex_descriptor_2 = linked_topology.add_vertex(recorder);

	EXPECT_TRUE(hyper_edge.valid(
	    {linked_vertex_descriptor_1, linked_vertex_descriptor_2}, {reference_vertex_descriptor},
	    linked_topology));

	DummyPortData dummy_port_data;

	EXPECT_THROW(
	    hyper_edge.map_output_data(
	        {{dummy_port_data}, {dummy_port_data}},
	        {linked_vertex_descriptor_1, linked_vertex_descriptor_2}, {reference_vertex_descriptor},
	        linked_topology),
	    std::invalid_argument);

	DummyRecorder linked_recorder_1(
	    CuboidMultiIndexSequence({2}, MultiIndex({3})), TimeDomainOnTopology(), std::nullopt);

	DummyRecorder linked_recorder_2(
	    CuboidMultiIndexSequence({2}, MultiIndex({5})), TimeDomainOnTopology(), std::nullopt);

	linked_topology.set(linked_vertex_descriptor_1, linked_recorder_1);
	linked_topology.set(linked_vertex_descriptor_2, linked_recorder_2);

	DummyRecorder::Results results_1({{1, 2}, {5, 6}});
	DummyRecorder::Results results_2({{3, 4}, {7, 8}});

	auto const mapped_output_data = hyper_edge.map_output_data(
	    {{results_1}, {results_2}}, {linked_vertex_descriptor_1, linked_vertex_descriptor_2},
	    {reference_vertex_descriptor}, linked_topology);

	EXPECT_EQ(mapped_output_data.size(), 1);
	EXPECT_EQ(mapped_output_data.at(0).size(), 1);
	EXPECT_TRUE(mapped_output_data.at(0).at(0));
	EXPECT_EQ(*mapped_output_data.at(0).at(0), results);
}
