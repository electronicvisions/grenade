#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/population.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/common/edge.h"
#include "grenade/common/inter_topology_hyper_edge/population.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace {

struct DummyCell : public Population::Cell
{
	DummyCell() = default;

	struct ParameterSpace : public Population::Cell::ParameterSpace
	{
		struct Parameterization : public Population::Cell::ParameterSpace::Parameterization
		{
			Parameterization(std::vector<size_t> values) : values(values) {}

			std::vector<size_t> values;

			virtual size_t size() const override
			{
				return values.size();
			}

			virtual std::unique_ptr<Population::Cell::ParameterSpace::Parameterization> get_section(
			    MultiIndexSequence const& sequence) const override
			{
				std::vector<size_t> new_values;
				for (auto const& element : sequence.get_elements()) {
					new_values.push_back(values.at(element.value.at(0)));
				}

				return std::make_unique<Parameterization>(std::move(new_values));
			}

			virtual std::unique_ptr<PortData> copy() const override
			{
				return std::make_unique<Parameterization>(*this);
			}

			virtual std::unique_ptr<PortData> move() override
			{
				return std::make_unique<Parameterization>(std::move(*this));
			}

			virtual bool is_equal_to(PortData const& other) const override
			{
				return values == static_cast<Parameterization const&>(other).values;
			}

			virtual std::ostream& print(std::ostream& os) const override
			{
				return os;
			}
		};

		ParameterSpace(size_t size) : m_size(size) {}

		virtual bool valid(
		    size_t input_port_on_cell,
		    Population::Cell::ParameterSpace::Parameterization const&) const override
		{
			return input_port_on_cell == 0;
		}

		virtual size_t size() const override
		{
			return m_size;
		}

		virtual std::unique_ptr<Population::Cell::ParameterSpace> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			return std::make_unique<ParameterSpace>(sequence.size());
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
			return m_size == static_cast<ParameterSpace const&>(other).m_size;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}

	private:
		size_t m_size;
	};

	struct Dynamics : public Population::Cell::Dynamics
	{
		Dynamics(std::vector<size_t> values, size_t batch_size) :
		    values(values), m_batch_size(batch_size)
		{
		}

		std::vector<size_t> values;

		virtual size_t size() const override
		{
			return values.size();
		}

		virtual size_t batch_size() const override
		{
			return m_batch_size;
		}

		virtual std::unique_ptr<Population::Cell::Dynamics> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			std::vector<size_t> new_values;
			for (auto const& element : sequence.get_elements()) {
				new_values.push_back(values.at(element.value.at(0)));
			}

			return std::make_unique<Dynamics>(new_values, m_batch_size);
		}

		virtual std::unique_ptr<PortData> copy() const override
		{
			return std::make_unique<Dynamics>(*this);
		}

		virtual std::unique_ptr<PortData> move() override
		{
			return std::make_unique<Dynamics>(std::move(*this));
		}

		virtual bool is_equal_to(PortData const& other) const override
		{
			return values == static_cast<Dynamics const&>(other).values &&
			       m_batch_size == static_cast<Dynamics const&>(other).m_batch_size;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}

	private:
		size_t m_batch_size;
	};

	struct ParameterizationPortType : public EmptyVertexPortType<ParameterizationPortType>
	{};

	struct DynamicsPortType : public EmptyVertexPortType<DynamicsPortType>
	{};

	virtual bool valid(Population::Cell::ParameterSpace const&) const override
	{
		return true;
	}

	virtual bool valid(size_t input_port_on_cell, Population::Cell::Dynamics const&) const override
	{
		return input_port_on_cell == 1;
	}

	virtual std::vector<Vertex::Port> get_input_ports() const override
	{
		return {
		    Vertex::Port(
		        ParameterizationPortType(), Vertex::Port::SumOrSplitSupport::no,
		        Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		        Vertex::Port::RequiresOrGeneratesData::yes,
		        ListMultiIndexSequence({MultiIndex({0})})),
		    Vertex::Port(
		        DynamicsPortType(), Vertex::Port::SumOrSplitSupport::no,
		        Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		        Vertex::Port::RequiresOrGeneratesData::yes,
		        ListMultiIndexSequence({MultiIndex({0})}))};
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
		return false;
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

TEST(PopulationInterTopologyHyperEdge, General)
{
	DummyCell dummy_cell;

	DummyCell::ParameterSpace dummy_parameter_space(10);
	DummyCell::ParameterSpace::Parameterization dummy_parameterization(
	    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
	DummyCell::Dynamics dummy_dynamics({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 5);

	CuboidMultiIndexSequence dummy_shape({10}, {CellOnPopulationDimensionUnit()});

	Population population(
	    dummy_cell, dummy_shape, dummy_parameter_space, std::nullopt, std::nullopt);

	PopulationInterTopologyHyperEdge hyper_edge;

	// reference vertex count not matching
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor_1 = reference_topology->add_vertex(DummyVertex());
		auto const reference_vertex_descriptor_2 = reference_topology->add_vertex(DummyVertex());

		LinkedTopology linked_topology(reference_topology);
		auto const linked_vertex_descriptor = linked_topology.add_vertex(population);

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor},
		    {reference_vertex_descriptor_1, reference_vertex_descriptor_2}, linked_topology));
	}

	// reference vertex type not matching
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor = reference_topology->add_vertex(DummyVertex());

		LinkedTopology linked_topology(reference_topology);
		auto const linked_vertex_descriptor = linked_topology.add_vertex(population);

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology));
	}

	// linked vertex type not matching
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor = reference_topology->add_vertex(population);

		LinkedTopology linked_topology(reference_topology);
		auto const linked_vertex_descriptor = linked_topology.add_vertex(DummyVertex());

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology));
	}

	// linked population shape not included in reference population shape
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor = reference_topology->add_vertex(population);

		LinkedTopology linked_topology(reference_topology);

		Population linked_population(
		    dummy_cell,
		    CuboidMultiIndexSequence({5}, MultiIndex({6}), {CellOnPopulationDimensionUnit()}),
		    DummyCell::ParameterSpace(5), std::nullopt, std::nullopt);

		auto const linked_vertex_descriptor = linked_topology.add_vertex(linked_population);

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology));
	}

	auto const reference_topology = std::make_shared<Topology>();
	auto const reference_vertex_descriptor = reference_topology->add_vertex(population);

	LinkedTopology linked_topology(reference_topology);
	auto const linked_vertex_descriptor_1 = linked_topology.add_vertex(population);
	auto const linked_vertex_descriptor_2 = linked_topology.add_vertex(population);

	EXPECT_TRUE(hyper_edge.valid(
	    {linked_vertex_descriptor_1, linked_vertex_descriptor_2}, {reference_vertex_descriptor},
	    linked_topology));

	DummyPortData dummy_port_data;

	EXPECT_THROW(
	    hyper_edge.map_input_data(
	        {{dummy_port_data, std::nullopt}},
	        {linked_vertex_descriptor_1, linked_vertex_descriptor_2}, {reference_vertex_descriptor},
	        linked_topology),
	    std::invalid_argument);

	Population linked_population_1(
	    dummy_cell, CuboidMultiIndexSequence({5}, {CellOnPopulationDimensionUnit()}),
	    DummyCell::ParameterSpace(5), std::nullopt, std::nullopt);

	Population linked_population_2(
	    dummy_cell,
	    CuboidMultiIndexSequence({5}, MultiIndex({5}), {CellOnPopulationDimensionUnit()}),
	    DummyCell::ParameterSpace(5), std::nullopt, std::nullopt);

	linked_topology.set(linked_vertex_descriptor_1, linked_population_1);
	linked_topology.set(linked_vertex_descriptor_2, linked_population_2);

	auto const mapped_input_data = hyper_edge.map_input_data(
	    {{dummy_parameterization, dummy_dynamics}},
	    {linked_vertex_descriptor_1, linked_vertex_descriptor_2}, {reference_vertex_descriptor},
	    linked_topology);

	EXPECT_EQ(mapped_input_data.size(), 2);
	EXPECT_EQ(mapped_input_data.at(0).size(), 2);
	EXPECT_EQ(mapped_input_data.at(1).size(), 2);
	EXPECT_TRUE(mapped_input_data.at(0).at(0));
	EXPECT_TRUE(mapped_input_data.at(0).at(1));
	EXPECT_EQ(
	    *mapped_input_data.at(0).at(0),
	    DummyCell::ParameterSpace::Parameterization({0, 1, 2, 3, 4}));
	EXPECT_EQ(*mapped_input_data.at(0).at(1), DummyCell::Dynamics({0, 1, 2, 3, 4}, 5));
	EXPECT_EQ(
	    *mapped_input_data.at(1).at(0),
	    DummyCell::ParameterSpace::Parameterization({5, 6, 7, 8, 9}));
	EXPECT_EQ(*mapped_input_data.at(1).at(1), DummyCell::Dynamics({5, 6, 7, 8, 9}, 5));
}
