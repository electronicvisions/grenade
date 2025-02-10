#include "dapr/property_holder.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/projection.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
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
	bool valid_for_synapse;

	DummyTimeDomainRuntimes(bool valid_for_synapse, size_t batch_size) :
	    valid_for_synapse(valid_for_synapse), m_batch_size(batch_size)
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
		return valid_for_synapse ==
		           static_cast<DummyTimeDomainRuntimes const&>(other).valid_for_synapse &&
		       m_batch_size == static_cast<DummyTimeDomainRuntimes const&>(other).m_batch_size;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}

private:
	size_t m_batch_size;
};


struct DummySynapse : public Projection::Synapse
{
	DummySynapse(
	    bool is_partitionable, bool requires_time_domain, size_t matching_parameter_space) :
	    m_is_partitionable(is_partitionable),
	    m_requires_time_domain(requires_time_domain),
	    m_matching_parameter_space(matching_parameter_space)
	{
	}

	struct ParameterSpace : public Projection::Synapse::ParameterSpace
	{
		struct Parameterization : public Projection::Synapse::ParameterSpace::Parameterization
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

			virtual std::unique_ptr<Projection::Synapse::ParameterSpace::Parameterization>
			get_section(MultiIndexSequence const& sequence) const override
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
		    size_t input_port_on_synapse,
		    Projection::Synapse::ParameterSpace::Parameterization const& parameterization)
		    const override
		{
			return input_port_on_synapse == 2 &&
			       dynamic_cast<Parameterization const&>(parameterization)
			           .valid_for_parameter_space;
		}

		virtual size_t size() const override
		{
			return m_size;
		}

		virtual std::unique_ptr<Projection::Synapse::ParameterSpace> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			return std::make_unique<ParameterSpace>(value, sequence.size());
		}

		virtual std::unique_ptr<Projection::Synapse::ParameterSpace> copy() const override
		{
			return std::make_unique<ParameterSpace>(*this);
		}

		virtual std::unique_ptr<Projection::Synapse::ParameterSpace> move() override
		{
			return std::make_unique<ParameterSpace>(std::move(*this));
		}

		virtual bool is_equal_to(Projection::Synapse::ParameterSpace const& other) const override
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

	struct Dynamics : public Projection::Synapse::Dynamics
	{
		bool valid_for_synapse;

		Dynamics(bool valid_for_synapse, size_t size, size_t batch_size) :
		    valid_for_synapse(valid_for_synapse), m_size(size), m_batch_size(batch_size)
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

		virtual std::unique_ptr<Projection::Synapse::Dynamics> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			return std::make_unique<Dynamics>(valid_for_synapse, sequence.size(), m_batch_size);
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
			return valid_for_synapse == static_cast<Dynamics const&>(other).valid_for_synapse &&
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

	struct ProjectionInputPortType : public EmptyVertexPortType<ProjectionInputPortType>
	{};

	struct SynapseInputPortType : public EmptyVertexPortType<SynapseInputPortType>
	{};

	struct ProjectionOutputPortType : public EmptyVertexPortType<ProjectionOutputPortType>
	{};

	struct SynapseOutputPortType : public EmptyVertexPortType<SynapseOutputPortType>
	{};

	struct SynapseParameterizationOutputPortType
	    : public EmptyVertexPortType<SynapseParameterizationOutputPortType>
	{};

	virtual bool valid(Projection::Synapse::ParameterSpace const& parameter_space) const override
	{
		return dynamic_cast<ParameterSpace const&>(parameter_space).value ==
		       m_matching_parameter_space;
	}

	virtual bool valid(
	    size_t input_port_on_synapse, Projection::Synapse::Dynamics const& dynamics) const override
	{
		return input_port_on_synapse == 3 &&
		       dynamic_cast<Dynamics const&>(dynamics).valid_for_synapse;
	}

	virtual Ports get_input_ports() const override
	{
		return {
		    {Vertex::Port(
		        ProjectionInputPortType(), Vertex::Port::SumOrSplitSupport::no,
		        Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		        Vertex::Port::RequiresOrGeneratesData::yes,
		        ListMultiIndexSequence({MultiIndex({0})}))},
		    {Vertex::Port(
		        SynapseInputPortType(), Vertex::Port::SumOrSplitSupport::no,
		        Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		        Vertex::Port::RequiresOrGeneratesData::yes,
		        ListMultiIndexSequence({MultiIndex({1})}))},
		    {Vertex::Port(
		         ParameterizationPortType(), Vertex::Port::SumOrSplitSupport::no,
		         Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		         Vertex::Port::RequiresOrGeneratesData::yes,
		         ListMultiIndexSequence({MultiIndex({2})})),
		     Vertex::Port(
		         DynamicsPortType(), Vertex::Port::SumOrSplitSupport::no,
		         Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		         Vertex::Port::RequiresOrGeneratesData::yes,
		         ListMultiIndexSequence({MultiIndex({3})}))}};
	}

	virtual Ports get_output_ports() const override
	{
		return {
		    {Vertex::Port(
		        ProjectionOutputPortType(), Vertex::Port::SumOrSplitSupport::yes,
		        Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
		        Vertex::Port::RequiresOrGeneratesData::no,
		        ListMultiIndexSequence({MultiIndex({4})}))},
		    {Vertex::Port(
		        SynapseOutputPortType(), Vertex::Port::SumOrSplitSupport::yes,
		        Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
		        Vertex::Port::RequiresOrGeneratesData::no,
		        ListMultiIndexSequence({MultiIndex({5})}))},
		    {Vertex::Port(
		        SynapseParameterizationOutputPortType(), Vertex::Port::SumOrSplitSupport::yes,
		        Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
		        Vertex::Port::RequiresOrGeneratesData::no,
		        ListMultiIndexSequence({MultiIndex({6})}))}};
	}

	virtual bool requires_time_domain() const override
	{
		return m_requires_time_domain;
	}

	virtual bool valid(TimeDomainRuntimes const& time_domain_runtimes) const override
	{
		return dynamic_cast<DummyTimeDomainRuntimes const&>(time_domain_runtimes).valid_for_synapse;
	}

	virtual bool is_partitionable() const override
	{
		return m_is_partitionable;
	}

	virtual std::unique_ptr<Projection::Synapse> copy() const override
	{
		return std::make_unique<DummySynapse>(*this);
	}

	virtual std::unique_ptr<Projection::Synapse> move() override
	{
		return std::make_unique<DummySynapse>(std::move(*this));
	}

	virtual bool is_equal_to(Projection::Synapse const& other) const override
	{
		return m_is_partitionable == static_cast<DummySynapse const&>(other).m_is_partitionable &&
		       m_requires_time_domain ==
		           static_cast<DummySynapse const&>(other).m_requires_time_domain &&
		       m_matching_parameter_space ==
		           static_cast<DummySynapse const&>(other).m_matching_parameter_space;
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


struct DummyConnector : public Projection::Connector
{
	DummyConnector(
	    MultiIndexSequence const& input_sequence,
	    MultiIndexSequence const& output_sequence,
	    size_t num_synapses,
	    size_t num_synapse_parameterizations) :
	    m_input_sequence(input_sequence),
	    m_output_sequence(output_sequence),
	    m_num_synapses(num_synapses),
	    m_num_synapse_parameterizations(num_synapse_parameterizations)
	{
	}

	struct Parameterization : public Projection::Connector::Parameterization
	{
		bool valid_for_connector;

		Parameterization(bool valid_for_connector) : valid_for_connector(valid_for_connector) {}

		virtual std::unique_ptr<Projection::Connector::Parameterization> get_section(
		    MultiIndexSequence const&) const override
		{
			return std::make_unique<Parameterization>(*this);
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
			return valid_for_connector ==
			       static_cast<Parameterization const&>(other).valid_for_connector;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}
	};

	struct Dynamics : public Projection::Connector::Dynamics
	{
		bool valid_for_connector;

		Dynamics(bool valid_for_connector, size_t batch_size) :
		    valid_for_connector(valid_for_connector), m_batch_size(batch_size)
		{
		}

		virtual size_t batch_size() const override
		{
			return m_batch_size;
		}

		virtual std::unique_ptr<Projection::Connector::Dynamics> get_section(
		    MultiIndexSequence const&) const override
		{
			return std::make_unique<Dynamics>(valid_for_connector, m_batch_size);
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
			return valid_for_connector == static_cast<Dynamics const&>(other).valid_for_connector &&
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

	struct OutputPortType : public EmptyVertexPortType<OutputPortType>
	{};

	virtual bool valid(
	    size_t input_port_on_connector,
	    Projection::Connector::Parameterization const& parameterization) const override
	{
		return input_port_on_connector == 0 &&
		       dynamic_cast<Parameterization const&>(parameterization).valid_for_connector;
	}

	virtual bool valid(
	    size_t input_port_on_connector,
	    Projection::Connector::Dynamics const& dynamics) const override
	{
		return input_port_on_connector == 1 &&
		       dynamic_cast<Dynamics const&>(dynamics).valid_for_connector;
	}

	virtual std::unique_ptr<MultiIndexSequence> get_input_sequence() const override
	{
		return m_input_sequence->copy();
	}

	virtual std::unique_ptr<MultiIndexSequence> get_output_sequence() const override
	{
		return m_output_sequence->copy();
	}

	virtual size_t get_num_synapses(MultiIndexSequence const&) const override
	{
		return m_num_synapses;
	}

	virtual std::unique_ptr<MultiIndexSequence> get_synapse_parameterizations(
	    MultiIndexSequence const&) const override
	{
		return nullptr;
	}

	virtual size_t get_num_synapse_parameterizations(MultiIndexSequence const&) const override
	{
		return m_num_synapse_parameterizations;
	}

	virtual std::unique_ptr<Connector> get_section(MultiIndexSequence const&) const override
	{
		return copy();
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

	/**
	 * Output ports this connector exposes at the projection.
	 */
	virtual std::vector<Vertex::Port> get_output_ports() const override
	{
		return {Vertex::Port(
		    OutputPortType(), Vertex::Port::SumOrSplitSupport::yes,
		    Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
		    Vertex::Port::RequiresOrGeneratesData::no, ListMultiIndexSequence({MultiIndex({0})}))};
	}

	virtual std::unique_ptr<Projection::Connector> copy() const override
	{
		return std::make_unique<DummyConnector>(*this);
	}

	virtual std::unique_ptr<Projection::Connector> move() override
	{
		return std::make_unique<DummyConnector>(std::move(*this));
	}

	virtual bool is_equal_to(Projection::Connector const& other) const override
	{
		return m_input_sequence == static_cast<DummyConnector const&>(other).m_input_sequence &&
		       m_output_sequence == static_cast<DummyConnector const&>(other).m_output_sequence &&
		       m_num_synapses == static_cast<DummyConnector const&>(other).m_num_synapses &&
		       m_num_synapse_parameterizations ==
		           static_cast<DummyConnector const&>(other).m_num_synapse_parameterizations;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}

private:
	dapr::PropertyHolder<MultiIndexSequence> m_input_sequence;
	dapr::PropertyHolder<MultiIndexSequence> m_output_sequence;
	size_t m_num_synapses;
	size_t m_num_synapse_parameterizations;
};


} // namespace

TEST(Projection, General)
{
	DummySynapse dummy_synapse(false, true, 13);
	DummySynapse partitionable_dummy_synapse(true, true, 13);

	DummySynapse::ParameterSpace dummy_parameter_space(0, 10);
	DummySynapse::ParameterSpace::Parameterization dummy_parameterization(false, 17);
	DummySynapse::Dynamics dummy_dynamics(false, 17, 5);
	DummyTimeDomainRuntimes dummy_time_domain_runtimes(false, 5);

	DummyConnector dummy_connector(
	    CuboidMultiIndexSequence({10}), CuboidMultiIndexSequence({7}), 20, 17);
	DummyConnector::Parameterization dummy_connector_parameterization(false);
	DummyConnector::Dynamics dummy_connector_dynamics(false, 5);

	// invalid parameter space
	EXPECT_THROW(
	    Projection(dummy_synapse, dummy_parameter_space, dummy_connector, TimeDomainOnTopology()),
	    std::invalid_argument);

	dummy_parameter_space.value = 13;

	// invalid parameter space size
	EXPECT_THROW(
	    Projection(dummy_synapse, dummy_parameter_space, dummy_connector, TimeDomainOnTopology()),
	    std::invalid_argument);

	dummy_parameter_space = DummySynapse::ParameterSpace(13, 17);

	// invalid time domain optionality
	EXPECT_THROW(
	    Projection(dummy_synapse, dummy_parameter_space, dummy_connector, std::nullopt),
	    std::invalid_argument);

	// invalid partitionability
	EXPECT_THROW(
	    Projection(
	        dummy_synapse, dummy_parameter_space, dummy_connector, TimeDomainOnTopology(),
	        ExecutionInstanceOnExecutor()),
	    std::invalid_argument);

	Projection projection(
	    partitionable_dummy_synapse, dummy_parameter_space, dummy_connector,
	    TimeDomainOnTopology(42), ExecutionInstanceOnExecutor());

	EXPECT_EQ(projection.get_synapse(), partitionable_dummy_synapse);
	EXPECT_EQ(projection.get_synapse_parameter_space(), dummy_parameter_space);
	EXPECT_EQ(projection.get_connector(), dummy_connector);

	EXPECT_EQ(projection.size(), 20);

	EXPECT_EQ(projection.get_input_ports().size(), 6);
	EXPECT_EQ(
	    projection.get_input_ports().at(0).get_type(), DummySynapse::ProjectionInputPortType());
	EXPECT_EQ(projection.get_input_ports().at(1).get_type(), DummySynapse::SynapseInputPortType());
	EXPECT_EQ(
	    projection.get_input_ports().at(2).get_type(), DummySynapse::ParameterizationPortType());
	EXPECT_EQ(projection.get_input_ports().at(3).get_type(), DummySynapse::DynamicsPortType());
	EXPECT_EQ(
	    projection.get_input_ports().at(4).get_type(), DummyConnector::ParameterizationPortType());
	EXPECT_EQ(projection.get_input_ports().at(5).get_type(), DummyConnector::DynamicsPortType());
	EXPECT_EQ(
	    projection.get_input_ports().at(0).get_channels(),
	    *dummy_connector.get_input_sequence()->cartesian_product(
	        dummy_synapse.get_input_ports().projection.at(0).get_channels()));
	EXPECT_EQ(
	    projection.get_input_ports().at(1).get_channels(),
	    *CuboidMultiIndexSequence(
	         {dummy_connector.get_num_synapses(CuboidMultiIndexSequence() /* ignored */)})
	         .cartesian_product(dummy_synapse.get_input_ports().synapse.at(0).get_channels()));
	EXPECT_EQ(
	    projection.get_input_ports().at(2).get_channels(),
	    *CuboidMultiIndexSequence({dummy_connector.get_num_synapse_parameterizations(
	                                  CuboidMultiIndexSequence() /* ignored */)})
	         .cartesian_product(
	             dummy_synapse.get_input_ports().synapse_parameterization.at(0).get_channels()));
	EXPECT_EQ(
	    projection.get_input_ports().at(3).get_channels(),
	    *CuboidMultiIndexSequence({dummy_connector.get_num_synapse_parameterizations(
	                                  CuboidMultiIndexSequence() /* ignored */)})
	         .cartesian_product(
	             dummy_synapse.get_input_ports().synapse_parameterization.at(1).get_channels()));
	EXPECT_EQ(projection.get_input_ports().at(4), dummy_connector.get_input_ports().at(0));
	EXPECT_EQ(projection.get_input_ports().at(5), dummy_connector.get_input_ports().at(1));

	EXPECT_EQ(projection.get_output_ports().size(), 4);
	EXPECT_EQ(
	    projection.get_output_ports().at(0).get_type(), DummySynapse::ProjectionOutputPortType());
	EXPECT_EQ(
	    projection.get_output_ports().at(1).get_type(), DummySynapse::SynapseOutputPortType());
	EXPECT_EQ(
	    projection.get_output_ports().at(2).get_type(),
	    DummySynapse::SynapseParameterizationOutputPortType());
	EXPECT_EQ(projection.get_output_ports().at(3).get_type(), DummyConnector::OutputPortType());
	EXPECT_EQ(
	    projection.get_output_ports().at(0).get_channels(),
	    *dummy_connector.get_output_sequence()->cartesian_product(
	        dummy_synapse.get_output_ports().projection.at(0).get_channels()));
	EXPECT_EQ(
	    projection.get_output_ports().at(1).get_channels(),
	    *CuboidMultiIndexSequence(
	         {dummy_connector.get_num_synapses(CuboidMultiIndexSequence() /* ignored */)})
	         .cartesian_product(dummy_synapse.get_output_ports().synapse.at(0).get_channels()));
	EXPECT_EQ(
	    projection.get_output_ports().at(2).get_channels(),
	    *CuboidMultiIndexSequence({dummy_connector.get_num_synapse_parameterizations(
	                                  CuboidMultiIndexSequence() /* ignored */)})
	         .cartesian_product(
	             dummy_synapse.get_output_ports().synapse_parameterization.at(0).get_channels()));
	EXPECT_EQ(projection.get_output_ports().at(3), dummy_connector.get_output_ports().at(0));

	EXPECT_EQ(projection.get_time_domain(), TimeDomainOnTopology(42));
	EXPECT_EQ(projection.get_execution_instance_on_executor(), ExecutionInstanceOnExecutor());

	EXPECT_FALSE(projection.valid(dummy_time_domain_runtimes));
	dummy_time_domain_runtimes.valid_for_synapse = true;
	EXPECT_TRUE(projection.valid(dummy_time_domain_runtimes));

	EXPECT_FALSE(projection.valid_input_port_data(2, dummy_parameterization));
	dummy_parameterization.valid_for_parameter_space = true;
	EXPECT_FALSE(projection.valid_input_port_data(3, dummy_parameterization));
	EXPECT_TRUE(projection.valid_input_port_data(2, dummy_parameterization));

	EXPECT_FALSE(projection.valid_input_port_data(3, dummy_dynamics));
	dummy_dynamics.valid_for_synapse = true;
	EXPECT_FALSE(projection.valid_input_port_data(4, dummy_dynamics));
	EXPECT_TRUE(projection.valid_input_port_data(3, dummy_dynamics));

	EXPECT_FALSE(projection.valid_input_port_data(4, dummy_connector_parameterization));
	dummy_connector_parameterization.valid_for_connector = true;
	EXPECT_FALSE(projection.valid_input_port_data(5, dummy_connector_parameterization));
	EXPECT_TRUE(projection.valid_input_port_data(4, dummy_connector_parameterization));

	EXPECT_FALSE(projection.valid_input_port_data(5, dummy_connector_dynamics));
	dummy_connector_dynamics.valid_for_connector = true;
	EXPECT_FALSE(projection.valid_input_port_data(4, dummy_connector_dynamics));
	EXPECT_TRUE(projection.valid_input_port_data(5, dummy_connector_dynamics));

	EXPECT_THROW(
	    projection.set_connector(
	        DummyConnector(CuboidMultiIndexSequence({10}), CuboidMultiIndexSequence({7}), 20, 18)),
	    std::invalid_argument);
	EXPECT_NO_THROW(projection.set_connector(dummy_connector));

	dummy_parameter_space.value = 0;
	EXPECT_THROW(
	    projection.set_synapse_parameter_space(dummy_parameter_space), std::invalid_argument);
	EXPECT_THROW(
	    projection.set_synapse_parameter_space(DummySynapse::ParameterSpace(true, 11)),
	    std::invalid_argument);
	dummy_parameter_space.value = 13;
	EXPECT_NO_THROW(projection.set_synapse_parameter_space(dummy_parameter_space));

	EXPECT_THROW(projection.set_synapse(dummy_synapse), std::invalid_argument);
	EXPECT_THROW(projection.set_synapse(DummySynapse(true, false, 13)), std::invalid_argument);
	EXPECT_THROW(projection.set_synapse(DummySynapse(true, true, 0)), std::invalid_argument);
	projection.set_synapse(DummySynapse(true, true, 13));

	EXPECT_THROW(projection.set_time_domain(std::nullopt), std::invalid_argument);
	EXPECT_THROW(
	    Projection(
	        DummySynapse(true, false, 13), dummy_parameter_space, dummy_connector, std::nullopt)
	        .set_time_domain(TimeDomainOnTopology(42)),
	    std::invalid_argument);
}
