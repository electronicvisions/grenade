#include "grenade/common/inter_topology_hyper_edge/projection.h"

#include "cereal/types/halco/common/geometry.h"
#include "dapr/property_holder.h"
#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/projection.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <stdexcept>
#include <vector>
#include <cereal/archives/json.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace {

struct DummySynapse : public Projection::Synapse
{
	DummySynapse() = default;

	struct ParameterSpace : public Projection::Synapse::ParameterSpace
	{
		struct Parameterization : public Projection::Synapse::ParameterSpace::Parameterization
		{
			Parameterization(std::vector<size_t> values) : values(values) {}

			std::vector<size_t> values;

			virtual size_t size() const override
			{
				return values.size();
			}

			virtual std::unique_ptr<Projection::Synapse::ParameterSpace::Parameterization>
			get_section(MultiIndexSequence const& sequence) const override
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

		std::vector<size_t> values;

		virtual bool valid(
		    size_t input_port_on_synapse,
		    Projection::Synapse::ParameterSpace::Parameterization const&) const override
		{
			return input_port_on_synapse == 2;
		}

		virtual size_t size() const override
		{
			return m_size;
		}

		virtual std::unique_ptr<Projection::Synapse::ParameterSpace> get_section(
		    MultiIndexSequence const&) const override
		{
			return nullptr;
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
			return values == static_cast<ParameterSpace const&>(other).values;
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

		virtual std::unique_ptr<Projection::Synapse::Dynamics> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			std::vector<size_t> new_values;
			for (auto const& element : sequence.get_elements()) {
				new_values.push_back(values.at(element.value.at(0)));
			}

			return std::make_unique<Dynamics>(std::move(new_values), m_batch_size);
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

	virtual bool valid(Projection::Synapse::ParameterSpace const&) const override
	{
		return true;
	}

	virtual bool valid(
	    size_t input_port_on_synapse, Projection::Synapse::Dynamics const&) const override
	{
		return input_port_on_synapse == 3;
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

	virtual std::unique_ptr<Projection::Synapse> copy() const override
	{
		return std::make_unique<DummySynapse>(*this);
	}

	virtual std::unique_ptr<Projection::Synapse> move() override
	{
		return std::make_unique<DummySynapse>(std::move(*this));
	}

	virtual bool is_equal_to(Projection::Synapse const&) const override
	{
		return true;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};


/**
 * Dummy connector implementing All-To-All connectivity.
 */
struct DummyConnector : public Projection::Connector
{
	DummyConnector(
	    MultiIndexSequence const& input_sequence, MultiIndexSequence const& output_sequence) :
	    m_input_sequence(input_sequence), m_output_sequence(output_sequence)
	{
	}

	struct Parameterization : public Projection::Connector::Parameterization
	{
		std::vector<size_t> values;

		Parameterization(std::vector<size_t> values) : values(values) {}

		virtual std::unique_ptr<Projection::Connector::Parameterization> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			std::vector<size_t> new_values;
			for (auto const& element : sequence.get_elements()) {
				new_values.push_back(values.at(element.value.at(0)));
			}

			return std::make_unique<Parameterization>(std::move(new_values));
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
			return values == static_cast<Parameterization const&>(other).values;
		}

		virtual std::ostream& print(std::ostream& os) const override
		{
			return os;
		}
	};

	struct Dynamics : public Projection::Connector::Dynamics
	{
		std::vector<size_t> values;

		Dynamics(std::vector<size_t> values, size_t batch_size) :
		    values(values), m_batch_size(batch_size)
		{
		}

		virtual size_t batch_size() const override
		{
			return m_batch_size;
		}

		virtual std::unique_ptr<Projection::Connector::Dynamics> get_section(
		    MultiIndexSequence const& sequence) const override
		{
			std::vector<size_t> new_values;
			for (auto const& element : sequence.get_elements()) {
				new_values.push_back(values.at(element.value.at(0)));
			}

			return std::make_unique<Dynamics>(std::move(new_values), m_batch_size);
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

	struct OutputPortType : public EmptyVertexPortType<OutputPortType>
	{};

	virtual bool valid(
	    size_t input_port_on_connector,
	    Projection::Connector::Parameterization const&) const override
	{
		return input_port_on_connector == 0;
	}

	virtual bool valid(
	    size_t input_port_on_connector, Projection::Connector::Dynamics const&) const override
	{
		return input_port_on_connector == 1;
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
		return 0;
	}

	virtual std::unique_ptr<MultiIndexSequence> get_synapse_parameterizations(
	    MultiIndexSequence const& sequence) const override
	{
		return CuboidMultiIndexSequence(
		           {m_input_sequence->cartesian_product(*m_output_sequence)->size()})
		    .related_sequence_subset_restriction(
		        *m_input_sequence->cartesian_product(*m_output_sequence), sequence)
		    ->copy();
	}

	virtual size_t get_num_synapse_parameterizations(
	    MultiIndexSequence const& sequence) const override
	{
		return get_synapse_parameterizations(sequence)->size();
	}

	virtual std::unique_ptr<Connector> get_section(
	    MultiIndexSequence const& sequence) const override
	{
		return std::make_unique<DummyConnector>(
		    *sequence.distinct_projection({0}), *sequence.distinct_projection({1}));
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
		       m_output_sequence == static_cast<DummyConnector const&>(other).m_output_sequence;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}

private:
	dapr::PropertyHolder<MultiIndexSequence> m_input_sequence;
	dapr::PropertyHolder<MultiIndexSequence> m_output_sequence;
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

TEST(ProjectionInterTopologyHyperEdge, General)
{
	DummySynapse dummy_synapse;

	DummySynapse::ParameterSpace dummy_parameter_space(10);
	DummySynapse::ParameterSpace::Parameterization dummy_parameterization(
	    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
	DummySynapse::Dynamics dummy_dynamics({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 5);

	DummyConnector dummy_connector(CuboidMultiIndexSequence({10}), CuboidMultiIndexSequence({1}));
	DummyConnector::Parameterization dummy_connector_parameterization(
	    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
	DummyConnector::Dynamics dummy_connector_dynamics({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 5);

	Projection projection(
	    dummy_synapse, dummy_parameter_space, dummy_connector, std::nullopt, std::nullopt);

	ProjectionInterTopologyHyperEdge hyper_edge;

	// reference vertex count not matching
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor_1 = reference_topology->add_vertex(DummyVertex());
		auto const reference_vertex_descriptor_2 = reference_topology->add_vertex(DummyVertex());

		LinkedTopology linked_topology(reference_topology);
		auto const linked_vertex_descriptor = linked_topology.add_vertex(projection);

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor},
		    {reference_vertex_descriptor_1, reference_vertex_descriptor_2}, linked_topology));
	}

	// reference vertex type not matching
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor = reference_topology->add_vertex(DummyVertex());

		LinkedTopology linked_topology(reference_topology);
		auto const linked_vertex_descriptor = linked_topology.add_vertex(projection);

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology));
	}

	// linked vertex type not matching
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor = reference_topology->add_vertex(projection);

		LinkedTopology linked_topology(reference_topology);
		auto const linked_vertex_descriptor = linked_topology.add_vertex(DummyVertex());

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology));
	}

	// linked population shape not included in reference population shape
	{
		auto const reference_topology = std::make_shared<Topology>();
		auto const reference_vertex_descriptor = reference_topology->add_vertex(projection);

		LinkedTopology linked_topology(reference_topology);

		Projection linked_projection(
		    dummy_synapse, DummySynapse::ParameterSpace(5),
		    DummyConnector(
		        CuboidMultiIndexSequence({5}, MultiIndex({6})), CuboidMultiIndexSequence({1})),
		    std::nullopt, std::nullopt);

		auto const linked_vertex_descriptor = linked_topology.add_vertex(linked_projection);

		EXPECT_FALSE(hyper_edge.valid(
		    {linked_vertex_descriptor}, {reference_vertex_descriptor}, linked_topology));
	}

	auto const reference_topology = std::make_shared<Topology>();
	auto const reference_vertex_descriptor = reference_topology->add_vertex(projection);

	LinkedTopology linked_topology(reference_topology);
	auto const linked_vertex_descriptor_1 = linked_topology.add_vertex(projection);
	auto const linked_vertex_descriptor_2 = linked_topology.add_vertex(projection);

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

	Projection linked_projection_1(
	    dummy_synapse, DummySynapse::ParameterSpace(5),
	    DummyConnector(CuboidMultiIndexSequence({5}), CuboidMultiIndexSequence({1})), std::nullopt,
	    std::nullopt);

	Projection linked_projection_2(
	    dummy_synapse, DummySynapse::ParameterSpace(5),
	    DummyConnector(
	        CuboidMultiIndexSequence({5}, MultiIndex({5})), CuboidMultiIndexSequence({1})),
	    std::nullopt, std::nullopt);

	linked_topology.set(linked_vertex_descriptor_1, linked_projection_1);
	linked_topology.set(linked_vertex_descriptor_2, linked_projection_2);

	auto const mapped_input_data = hyper_edge.map_input_data(
	    {{std::nullopt, std::nullopt, dummy_parameterization, dummy_dynamics,
	      dummy_connector_parameterization, dummy_connector_dynamics}},
	    {linked_vertex_descriptor_1, linked_vertex_descriptor_2}, {reference_vertex_descriptor},
	    linked_topology);

	EXPECT_EQ(mapped_input_data.size(), 2);
	EXPECT_EQ(mapped_input_data.at(0).size(), 6);
	EXPECT_EQ(mapped_input_data.at(1).size(), 6);
	EXPECT_FALSE(mapped_input_data.at(0).at(0));
	EXPECT_FALSE(mapped_input_data.at(0).at(1));
	EXPECT_TRUE(mapped_input_data.at(0).at(2));
	EXPECT_TRUE(mapped_input_data.at(0).at(3));
	EXPECT_TRUE(mapped_input_data.at(0).at(4));
	EXPECT_TRUE(mapped_input_data.at(0).at(5));
	EXPECT_EQ(
	    *mapped_input_data.at(0).at(2),
	    DummySynapse::ParameterSpace::Parameterization({0, 1, 2, 3, 4}));
	EXPECT_EQ(*mapped_input_data.at(0).at(3), DummySynapse::Dynamics({0, 1, 2, 3, 4}, 5));
	EXPECT_EQ(*mapped_input_data.at(0).at(4), DummyConnector::Parameterization({0, 1, 2, 3, 4}));
	EXPECT_EQ(*mapped_input_data.at(0).at(5), DummyConnector::Dynamics({0, 1, 2, 3, 4}, 5));
	EXPECT_FALSE(mapped_input_data.at(1).at(0));
	EXPECT_FALSE(mapped_input_data.at(1).at(1));
	EXPECT_TRUE(mapped_input_data.at(1).at(2));
	EXPECT_TRUE(mapped_input_data.at(1).at(3));
	EXPECT_TRUE(mapped_input_data.at(1).at(4));
	EXPECT_TRUE(mapped_input_data.at(1).at(5));
	EXPECT_EQ(
	    *mapped_input_data.at(1).at(2),
	    DummySynapse::ParameterSpace::Parameterization({5, 6, 7, 8, 9}));
	EXPECT_EQ(*mapped_input_data.at(1).at(3), DummySynapse::Dynamics({5, 6, 7, 8, 9}, 5));
	EXPECT_EQ(*mapped_input_data.at(1).at(4), DummyConnector::Parameterization({5, 6, 7, 8, 9}));
	EXPECT_EQ(*mapped_input_data.at(1).at(5), DummyConnector::Dynamics({5, 6, 7, 8, 9}, 5));
}
