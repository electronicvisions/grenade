#include "grenade/common/topology_rewrite/population.h"

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

struct DummyCell : public Population::Cell
{
	DummyCell() = default;

	struct ParameterSpace : public Population::Cell::ParameterSpace
	{
		std::vector<size_t> value;

		ParameterSpace(std::vector<size_t> value) : value(value) {}

		virtual bool valid(
		    size_t, Population::Cell::ParameterSpace::Parameterization const&) const override
		{
			return false;
		}

		virtual size_t size() const override
		{
			return value.size();
		}

		virtual std::unique_ptr<Population::Cell::ParameterSpace> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			std::vector<size_t> section_value;
			for (auto const& element : sequence.get_elements()) {
				section_value.push_back(value.at(element.value.at(0)));
			}
			return std::make_unique<ParameterSpace>(section_value);
		}

		virtual std::unique_ptr<Population::Cell::ParameterSpace> copy() const override
		{
			return std::make_unique<ParameterSpace>(*this);
		}

		virtual std::unique_ptr<Population::Cell::ParameterSpace> move() override
		{
			return std::make_unique<ParameterSpace>(std::move(*this));
		}

		virtual bool is_equal_to(Population::Cell::ParameterSpace const& other) const override
		{
			return value == static_cast<ParameterSpace const&>(other).value;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}
	};

	virtual bool valid(Population::Cell::ParameterSpace const&) const override
	{
		return true;
	}

	virtual bool valid(size_t, Population::Cell::Dynamics const&) const override
	{
		return false;
	}

	virtual std::vector<Vertex::Port> get_input_ports() const override
	{
		return {};
	}

	virtual std::vector<Vertex::Port> get_output_ports() const override
	{
		return {};
	}

	virtual bool requires_time_domain() const override
	{
		return false;
	}

	virtual bool valid(TimeDomainRuntimes const&) const override
	{
		return false;
	}

	virtual bool is_partitionable() const override
	{
		return true;
	}

	virtual std::unique_ptr<Population::Cell> copy() const override
	{
		return std::make_unique<DummyCell>(*this);
	}

	virtual std::unique_ptr<Population::Cell> move() override
	{
		return std::make_unique<DummyCell>(std::move(*this));
	}

	virtual bool is_equal_to(Population::Cell const&) const override
	{
		return true;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};


struct DummyResource : public ResourceEstimator::Resource
{
	std::vector<size_t> resources_per_cell;
	dapr::PropertyHolder<MultiIndexSequence> sequence;

	DummyResource() = default;
	DummyResource(std::vector<size_t> resources_per_cell, MultiIndexSequence const& sequence) :
	    resources_per_cell(resources_per_cell), sequence(sequence)
	{
	}
	DummyResource(std::vector<size_t> resources_per_cell) :
	    resources_per_cell(resources_per_cell), sequence()
	{
	}

	virtual Resource& operator+=(Resource const& other) override
	{
		auto const& other_resource = dynamic_cast<DummyResource const&>(other);
		if (other_resource.resources_per_cell.size() != resources_per_cell.size()) {
			throw std::invalid_argument("Resources not matching.");
		}
		for (size_t i = 0; i < resources_per_cell.size(); ++i) {
			resources_per_cell.at(i) += other_resource.resources_per_cell.at(i);
		}
		return *this;
	}

	virtual Resource& operator*=(size_t factor) override
	{
		for (size_t i = 0; i < resources_per_cell.size(); ++i) {
			resources_per_cell.at(i) *= factor;
		}
		return *this;
	}

	virtual std::vector<size_t> scalar_values() const override
	{
		size_t sum = 0;
		for (auto const& resource_per_cell : resources_per_cell) {
			sum += resource_per_cell;
		}
		return {sum};
	}

	virtual std::unique_ptr<Resource> subsequence(
	    MultiIndexSequence const& other_sequence) const override
	{
		if (!sequence->includes(other_sequence)) {
			throw std::invalid_argument("Other sequence not included in resource sequence.");
		}
		std::vector<size_t> new_resources_per_cell;
		auto const other_sequence_indices =
		    CuboidMultiIndexSequence({sequence->size()})
		        .related_sequence_subset_restriction(*sequence, other_sequence)
		        ->get_elements();
		for (auto const& sequence_index : other_sequence_indices) {
			new_resources_per_cell.push_back(resources_per_cell.at(sequence_index.value.at(0)));
		}
		return std::make_unique<DummyResource>(new_resources_per_cell, other_sequence);
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
		return resources_per_cell == static_cast<DummyResource const&>(other).resources_per_cell &&
		       sequence == static_cast<DummyResource const&>(other).sequence;
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
		auto const& population = dynamic_cast<Population const&>(m_topology.get(vertex_descriptor));
		std::vector<size_t> resources_per_cell(population.size(), 1);
		return std::make_unique<DummyResource>(resources_per_cell, population.get_shape());
	}
};

} // namespace

TEST(PopulationTopologyRewrite, General)
{
	DummyCell dummy_cell;

	DummyCell::ParameterSpace dummy_parameter_space({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});

	CuboidMultiIndexSequence dummy_shape({11}, {CellOnPopulationDimensionUnit()});

	Population population(
	    dummy_cell, dummy_shape, dummy_parameter_space, std::nullopt, std::nullopt);

	auto topology = std::make_shared<Topology>();
	auto const reference_vertex_descriptor = topology->add_vertex(population);

	auto linked_topology = std::make_shared<LinkedTopology>(topology);
	auto const linked_vertex_descriptor = linked_topology->add_vertex(population);

	linked_topology->add_inter_graph_hyper_edge(
	    {linked_vertex_descriptor}, {reference_vertex_descriptor},
	    IdentityInterTopologyHyperEdge());

	DummyResourceEstimator dummy_resource_estimator(*linked_topology);

	PopulationTopologyRewrite::SystemResources system_resources{
	    {ConnectionOnExecutor(1), DummyResource({3})},
	    {ConnectionOnExecutor(2), DummyResource({4})}};

	PopulationTopologyRewrite rewrite(dummy_resource_estimator, system_resources, linked_topology);
	rewrite();

	EXPECT_EQ(linked_topology->num_vertices(), 4);

	Population expected_population_1(
	    dummy_cell, CuboidMultiIndexSequence({3}, {CellOnPopulationDimensionUnit()}),
	    DummyCell::ParameterSpace({0, 1, 2}), std::nullopt,
	    ExecutionInstanceOnExecutor(ExecutionInstanceID(), ConnectionOnExecutor(1)));
	EXPECT_EQ(linked_topology->get(VertexOnTopology(1)), expected_population_1);

	Population expected_population_2(
	    dummy_cell,
	    CuboidMultiIndexSequence({4}, MultiIndex({3}), {CellOnPopulationDimensionUnit()}),
	    DummyCell::ParameterSpace({3, 4, 5, 6}), std::nullopt,
	    ExecutionInstanceOnExecutor(ExecutionInstanceID(), ConnectionOnExecutor(2)));
	EXPECT_EQ(linked_topology->get(VertexOnTopology(2)), expected_population_2);

	Population expected_population_3(
	    dummy_cell,
	    CuboidMultiIndexSequence({3}, MultiIndex({7}), {CellOnPopulationDimensionUnit()}),
	    DummyCell::ParameterSpace({7, 8, 9}), std::nullopt,
	    ExecutionInstanceOnExecutor(ExecutionInstanceID(), ConnectionOnExecutor(1)));
	EXPECT_EQ(linked_topology->get(VertexOnTopology(3)), expected_population_3);

	Population expected_population_4(
	    dummy_cell,
	    CuboidMultiIndexSequence({1}, MultiIndex({10}), {CellOnPopulationDimensionUnit()}),
	    DummyCell::ParameterSpace({10}), std::nullopt,
	    ExecutionInstanceOnExecutor(ExecutionInstanceID(), ConnectionOnExecutor(2)));
	EXPECT_EQ(linked_topology->get(VertexOnTopology(4)), expected_population_4);
}
