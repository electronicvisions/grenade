#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/population.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/common/edge.h"
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

struct DummyTimeDomainRuntimes : TimeDomainRuntimes
{
	bool valid_for_cell;

	DummyTimeDomainRuntimes(bool valid_for_cell, size_t batch_size) :
	    valid_for_cell(valid_for_cell), m_batch_size(batch_size)
	{
	}

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

	virtual bool is_equal_to(TimeDomainRuntimes const& other) const override
	{
		return valid_for_cell ==
		           static_cast<DummyTimeDomainRuntimes const&>(other).valid_for_cell &&
		       m_batch_size == static_cast<DummyTimeDomainRuntimes const&>(other).m_batch_size;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}

private:
	size_t m_batch_size;
};


struct DummyCell : public Population::Cell
{
	DummyCell(bool is_partitionable, bool requires_time_domain, size_t matching_parameter_space) :
	    m_is_partitionable(is_partitionable),
	    m_requires_time_domain(requires_time_domain),
	    m_matching_parameter_space(matching_parameter_space)
	{
	}

	struct ParameterSpace : public Population::Cell::ParameterSpace
	{
		struct Parameterization : public Population::Cell::ParameterSpace::Parameterization
		{
			bool valid_for_parameter_space;

			Parameterization(bool valid_for_parameter_space, size_t size) :
			    valid_for_parameter_space(valid_for_parameter_space), m_size(size)
			{
			}

			virtual size_t size() const override
			{
				return m_size;
			}

			virtual std::unique_ptr<Population::Cell::ParameterSpace::Parameterization> get_section(
			    MultiIndexSequence const& sequence) const override
			{
				return std::make_unique<Parameterization>(
				    valid_for_parameter_space, sequence.size());
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
				return valid_for_parameter_space ==
				           static_cast<Parameterization const&>(other).valid_for_parameter_space &&
				       m_size == static_cast<Parameterization const&>(other).m_size;
			}

			virtual std::ostream& print(std::ostream& os) const override
			{
				return os;
			}

		private:
			size_t m_size;
		};

		size_t value;

		ParameterSpace(size_t value, size_t size) : value(value), m_size(size) {}

		virtual bool valid(
		    size_t input_port_on_cell,
		    Population::Cell::ParameterSpace::Parameterization const& parameterization)
		    const override
		{
			return input_port_on_cell == 0 &&
			       dynamic_cast<Parameterization const&>(parameterization)
			           .valid_for_parameter_space;
		}

		virtual size_t size() const override
		{
			return m_size;
		}

		virtual std::unique_ptr<Population::Cell::ParameterSpace> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			return std::make_unique<ParameterSpace>(value, sequence.size());
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
			return value == static_cast<ParameterSpace const&>(other).value &&
			       m_size == static_cast<ParameterSpace const&>(other).m_size;
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
		bool valid_for_cell;

		Dynamics(bool valid_for_cell, size_t size, size_t batch_size) :
		    valid_for_cell(valid_for_cell), m_size(size), m_batch_size(batch_size)
		{
		}

		virtual size_t size() const override
		{
			return m_size;
		}

		virtual size_t batch_size() const override
		{
			return m_batch_size;
		}

		virtual std::unique_ptr<Population::Cell::Dynamics> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			return std::make_unique<Dynamics>(valid_for_cell, sequence.size(), m_batch_size);
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
			return valid_for_cell == static_cast<Dynamics const&>(other).valid_for_cell &&
			       m_size == static_cast<Dynamics const&>(other).m_size &&
			       m_batch_size == static_cast<Dynamics const&>(other).m_batch_size;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}

	private:
		size_t m_size;
		size_t m_batch_size;
	};

	struct ParameterizationPortType : public EmptyVertexPortType<ParameterizationPortType>
	{};

	struct DynamicsPortType : public EmptyVertexPortType<DynamicsPortType>
	{};

	struct OutputPortType : public EmptyVertexPortType<OutputPortType>
	{};

	virtual bool valid(Population::Cell::ParameterSpace const& parameter_space) const override
	{
		return dynamic_cast<ParameterSpace const&>(parameter_space).value ==
		       m_matching_parameter_space;
	}

	virtual bool valid(
	    size_t input_port_on_cell, Population::Cell::Dynamics const& dynamics) const override
	{
		return input_port_on_cell == 1 && dynamic_cast<Dynamics const&>(dynamics).valid_for_cell;
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
		return {Vertex::Port(
		    OutputPortType(), Vertex::Port::SumOrSplitSupport::yes,
		    Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
		    Vertex::Port::RequiresOrGeneratesData::no, ListMultiIndexSequence({MultiIndex({0})}))};
	}

	virtual bool requires_time_domain() const override
	{
		return m_requires_time_domain;
	}

	virtual bool valid(TimeDomainRuntimes const& time_domain_runtimes) const override
	{
		return dynamic_cast<DummyTimeDomainRuntimes const&>(time_domain_runtimes).valid_for_cell;
	}

	virtual bool is_partitionable() const override
	{
		return m_is_partitionable;
	}

	virtual std::unique_ptr<Population::Cell> copy() const override
	{
		return std::make_unique<DummyCell>(*this);
	}

	virtual std::unique_ptr<Population::Cell> move() override
	{
		return std::make_unique<DummyCell>(std::move(*this));
	}

	virtual bool is_equal_to(Population::Cell const& other) const override
	{
		return m_is_partitionable == static_cast<DummyCell const&>(other).m_is_partitionable &&
		       m_requires_time_domain ==
		           static_cast<DummyCell const&>(other).m_requires_time_domain &&
		       m_matching_parameter_space ==
		           static_cast<DummyCell const&>(other).m_matching_parameter_space;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}

private:
	bool m_is_partitionable;
	bool m_requires_time_domain;
	size_t m_matching_parameter_space;
};


} // namespace

TEST(Population, General)
{
	DummyCell dummy_cell(false, true, 13);
	DummyCell partitionable_dummy_cell(true, true, 13);

	DummyCell::ParameterSpace dummy_parameter_space(0, 10);
	DummyCell::ParameterSpace::Parameterization dummy_parameterization(false, 10);
	DummyCell::Dynamics dummy_dynamics(false, 10, 5);
	DummyTimeDomainRuntimes dummy_time_domain_runtimes(false, 5);

	CuboidMultiIndexSequence shape_10({10}, {CellOnPopulationDimensionUnit()});
	CuboidMultiIndexSequence shape_11({11}, {CellOnPopulationDimensionUnit()});

	// invalid parameter space
	EXPECT_THROW(
	    Population(dummy_cell, shape_10, dummy_parameter_space, TimeDomainOnTopology()),
	    std::invalid_argument);

	dummy_parameter_space.value = 13;

	// invalid shape
	EXPECT_THROW(
	    Population(dummy_cell, shape_11, dummy_parameter_space, TimeDomainOnTopology()),
	    std::invalid_argument);

	// invalid time domain optionality
	EXPECT_THROW(
	    Population(dummy_cell, shape_10, dummy_parameter_space, std::nullopt),
	    std::invalid_argument);

	// invalid partitionability
	EXPECT_THROW(
	    Population(
	        dummy_cell, shape_10, dummy_parameter_space, TimeDomainOnTopology(),
	        ExecutionInstanceOnExecutor()),
	    std::invalid_argument);

	Population population(
	    partitionable_dummy_cell, shape_10, dummy_parameter_space, TimeDomainOnTopology(42),
	    ExecutionInstanceOnExecutor());

	EXPECT_EQ(population.get_cell(), partitionable_dummy_cell);
	EXPECT_EQ(population.get_parameter_space(), dummy_parameter_space);
	EXPECT_EQ(population.get_shape(), shape_10);

	EXPECT_EQ(population.size(), 10);

	EXPECT_EQ(population.get_input_ports().size(), 2);
	EXPECT_EQ(population.get_input_ports().at(0).get_type(), DummyCell::ParameterizationPortType());
	EXPECT_EQ(population.get_input_ports().at(1).get_type(), DummyCell::DynamicsPortType());
	EXPECT_EQ(
	    population.get_input_ports().at(0).get_channels(),
	    *shape_10.cartesian_product(
	        partitionable_dummy_cell.get_input_ports().at(0).get_channels()));
	EXPECT_EQ(
	    population.get_input_ports().at(1).get_channels(),
	    *shape_10.cartesian_product(
	        partitionable_dummy_cell.get_input_ports().at(0).get_channels()));

	EXPECT_EQ(population.get_output_ports().size(), 1);
	EXPECT_EQ(population.get_output_ports().at(0).get_type(), DummyCell::OutputPortType());
	EXPECT_EQ(
	    population.get_output_ports().at(0).get_channels(),
	    *shape_10.cartesian_product(
	        partitionable_dummy_cell.get_output_ports().at(0).get_channels()));

	EXPECT_EQ(population.get_time_domain(), TimeDomainOnTopology(42));
	EXPECT_EQ(population.get_execution_instance_on_executor(), ExecutionInstanceOnExecutor());

	EXPECT_FALSE(population.valid(dummy_time_domain_runtimes));
	dummy_time_domain_runtimes.valid_for_cell = true;
	EXPECT_TRUE(population.valid(dummy_time_domain_runtimes));

	EXPECT_FALSE(population.valid_input_port_data(0, dummy_parameterization));
	dummy_parameterization.valid_for_parameter_space = true;
	EXPECT_FALSE(population.valid_input_port_data(1, dummy_parameterization));
	EXPECT_TRUE(population.valid_input_port_data(0, dummy_parameterization));

	EXPECT_FALSE(population.valid_input_port_data(1, dummy_dynamics));
	dummy_dynamics.valid_for_cell = true;
	EXPECT_FALSE(population.valid_input_port_data(0, dummy_dynamics));
	EXPECT_TRUE(population.valid_input_port_data(1, dummy_dynamics));

	EXPECT_THROW(population.set_shape(shape_11), std::invalid_argument);
	EXPECT_NO_THROW(population.set_shape(shape_10));
	EXPECT_THROW(population.set_shape(CuboidMultiIndexSequence({10})), std::invalid_argument);

	dummy_parameter_space.value = 0;
	EXPECT_THROW(population.set_parameter_space(dummy_parameter_space), std::invalid_argument);
	EXPECT_THROW(
	    population.set_parameter_space(DummyCell::ParameterSpace(true, 11)), std::invalid_argument);
	dummy_parameter_space.value = 13;
	EXPECT_NO_THROW(population.set_parameter_space(dummy_parameter_space));

	EXPECT_THROW(population.set_cell(dummy_cell), std::invalid_argument);
	EXPECT_THROW(population.set_cell(DummyCell(true, false, 13)), std::invalid_argument);
	EXPECT_THROW(population.set_cell(DummyCell(true, true, 0)), std::invalid_argument);
	population.set_cell(DummyCell(true, true, 13));

	EXPECT_THROW(population.set_time_domain(std::nullopt), std::invalid_argument);
	EXPECT_THROW(
	    Population(DummyCell(true, false, 13), shape_10, dummy_parameter_space, std::nullopt)
	        .set_time_domain(TimeDomainOnTopology(42)),
	    std::invalid_argument);
}
