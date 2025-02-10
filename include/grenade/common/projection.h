#pragma once
#include "dapr/property.h"
#include "dapr/property_holder.h"
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/population.h"
#include "grenade/common/port_data.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/vertex.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Projection in topology.
 * A projection is a collection of synapses of the same type with potentially heterogeneous
 * parameter spaces.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    visible, holder_type("std::shared_ptr<grenade::common::Projection>")) Projection
    : public PartitionedVertex
{
	/**
	 * Synapse type of projection.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) Synapse : public dapr::Property<Synapse>
	{
		/**
		 * Parameter space matching synapse type.
		 * It constrains the possible parameterizations and might be heterogeneous across the
		 * projection. It is part of the topological description.
		 */
		struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) ParameterSpace
		    : public dapr::Property<ParameterSpace>
		{
			/**
			 * Parameterization of synapses within parameter space.
			 * It is supplied separately as parameterization of the topology.
			 */
			struct SYMBOL_VISIBLE GENPYBIND(visible) Parameterization : public PortData
			{
				virtual ~Parameterization();

				virtual size_t size() const = 0;

				/**
				 * Get section of parameterization.
				 * The sequence is required to be included in the interval [0, size()).
				 */
				virtual std::unique_ptr<Parameterization> get_section(
				    MultiIndexSequence const& sequence) const = 0;
			};

			virtual ~ParameterSpace();

			virtual size_t size() const = 0;

			virtual bool valid(
			    size_t input_port_on_synapse, Parameterization const& parameterization) const = 0;

			/**
			 * Get section of parameter space.
			 * The sequence is required to be included in the interval [0, size()).
			 */
			virtual std::unique_ptr<ParameterSpace> get_section(
			    MultiIndexSequence const& sequence) const = 0;
		};

		struct SYMBOL_VISIBLE GENPYBIND(visible) Dynamics : public BatchedPortData
		{
			virtual ~Dynamics();

			virtual size_t size() const = 0;
			virtual size_t batch_size() const = 0;

			/**
			 * Get section of dynamics.
			 * The sequence is required to be included in the interval [0, size()).
			 */
			virtual std::unique_ptr<Dynamics> get_section(
			    MultiIndexSequence const& sequence) const = 0;
		};

		/**
		 * Unsized ports this synapse exposes.
		 * Ports are constructed in the order [projection, synapse, synapse_parameterization].
		 */
		struct Ports
		{
			/**
			 * Ports on main data path to/from the projection.
			 * The corresponding port is sized according to the projection's input/output shape.
			 */
			std::vector<Port> projection;

			/**
			 * Ports exposed per synapse.
			 * The corresponding port is sized according to the projection's number of realized
			 * synapses.
			 */
			std::vector<Port> synapse;

			/**
			 * Ports exposed per synapse parameterization.
			 * The corresponding port is sized according to the projection's number of synapse
			 * parameterizations.
			 */
			std::vector<Port> synapse_parameterization;
		};

		virtual ~Synapse();

		virtual bool valid(ParameterSpace const& parameter_space) const = 0;

		virtual bool valid(size_t input_port_on_synapse, Dynamics const& dynamics) const = 0;

		/**
		 * Input ports this synapse exposes at the projection.
		 */
		virtual Ports get_input_ports() const = 0;

		/**
		 * Output ports this synapse exposes at the projection.
		 */
		virtual Ports get_output_ports() const = 0;

		virtual bool requires_time_domain() const = 0;

		virtual bool valid(TimeDomainRuntimes const& time_domain_runtimes) const = 0;

		virtual bool is_partitionable() const = 0;
	};

	/**
	 * Connector of projection.
	 * The connector determines the connections possible and present in the projection.
	 * Therefore, also the number of parameterized synapses as well as the number of connections is
	 * determined by the connector.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) Connector : public dapr::Property<Connector>
	{
		/**
		 * Parameterization of connector within parameter space.
		 * It is supplied separately as parameterization of the topology.
		 */
		struct SYMBOL_VISIBLE GENPYBIND(visible) Parameterization : public PortData
		{
			virtual ~Parameterization();

			/**
			 * Get section of parameterization.
			 * The sequence is required to be included in the connector shape (input_sequence x
			 * output_sequence).
			 */
			virtual std::unique_ptr<Parameterization> get_section(
			    MultiIndexSequence const& sequence) const = 0;
		};

		/**
		 * Dynamics of connector.
		 * It is supplied separately as dynamics of the topology.
		 */
		struct SYMBOL_VISIBLE GENPYBIND(visible) Dynamics : public BatchedPortData
		{
			virtual ~Dynamics();

			/**
			 * Get section of dynamics.
			 * The sequence is required to be included in the connector shape (input_sequence x
			 * output_sequence).
			 */
			virtual std::unique_ptr<Dynamics> get_section(
			    MultiIndexSequence const& sequence) const = 0;
		};

		virtual ~Connector();

		virtual bool valid(
		    size_t input_port_on_connector, Parameterization const& parameterization) const = 0;

		virtual bool valid(size_t input_port_on_connector, Dynamics const& dynamics) const = 0;

		/**
		 * Get sequence of (pre-synaptic) inputs generated by connector.
		 */
		virtual std::unique_ptr<MultiIndexSequence> get_input_sequence() const = 0;

		/**
		 * Get sequence of (post-synaptic) outputs generated by connector.
		 */
		virtual std::unique_ptr<MultiIndexSequence> get_output_sequence() const = 0;

		/**
		 * Get number of synapses generated by connector.
		 * @param sequence MultiIndexSequence for which to get number of synapses
		 */
		virtual size_t get_num_synapses(MultiIndexSequence const& sequence) const = 0;

		/**
		 * Get sequence of synapse parameterizations required by connector.
		 * This sequence describes the position in the 1d space of synapse parameterizations.
		 * @param sequence MultiIndexSequence for which to get synapse parameterizations
		 */
		virtual std::unique_ptr<MultiIndexSequence> get_synapse_parameterizations(
		    MultiIndexSequence const& sequence) const = 0;

		/**
		 * Get number of synapse parameterizations required by connector.
		 * @param sequence MultiIndexSequence for which to get the number of synapse
		 * parameterizations
		 */
		virtual size_t get_num_synapse_parameterizations(
		    MultiIndexSequence const& sequence) const = 0;

		/**
		 * Get section of connector.
		 * The sequence is required to be included in the connector shape (input_sequence x
		 * output_sequence).
		 */
		virtual std::unique_ptr<Connector> get_section(
		    MultiIndexSequence const& sequence) const = 0;

		/**
		 * Input ports this connector exposes at the projection.
		 */
		virtual std::vector<Port> get_input_ports() const = 0;

		/**
		 * Output ports this connector exposes at the projection.
		 */
		virtual std::vector<Port> get_output_ports() const = 0;
	};


	/**
	 * Construct projection.
	 *
	 * @param synapse Synapse type
	 * @param parameter_space Parameter space of synapses
	 * @param connector Connector value
	 * @param time_domain Time domain
	 * @param execution_instance_on_executor Optional execution instance on executor
	 * @throws std::invalid_argument On parameter space being invalid for synapse type.
	 * @throws std::invalid_argument On parameter space size not matching size of connector
	 * parameterization requirement.
	 * @throws std::invalid_argument On time domain given not matching requirement by synapse (value
	 * given but not required or no valud given but required).
	 * @throws std::invalid_argument On execution instance on executor being given but synapse not
	 * being partitionable.
	 */
	Projection(
	    Synapse const& synapse,
	    Synapse::ParameterSpace const& parameter_space,
	    Connector const& connector,
	    std::optional<TimeDomainOnTopology> const& time_domain,
	    std::optional<ExecutionInstanceOnExecutor> const& execution_instance_on_executor =
	        std::nullopt);

	/**
	 * Get whether the data is valid for the input port on the projection.
	 */
	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, PortData const& dynamics) const override;

	/**
	 * Get whether the time domain is valid for the projection.
	 * This matches the check defined for the synapse.
	 */
	virtual bool valid(TimeDomainRuntimes const& time_domain_runtimes) const override;

	/**
	 * Get synapse type property.
	 */
	Synapse const& get_synapse() const;

	/**
	 * Set synapse type property.
	 *
	 * @param synapse Synapse type to set
	 * @throws std::invalid_argument On parameter space not valid for synapse type.
	 * @throws std::invalid_argument On synapse type featuring different time domain requirement to
	 * previously contained synapse type.
	 * @throws std::invalid_argument On synapse type not being partitionable but an execution
	 * instance on the executor being assigned to projection.
	 */
	void set_synapse(Synapse const& synapse);

	/**
	 * Get parameter space of synapses on the projection.
	 */
	Synapse::ParameterSpace const& get_synapse_parameter_space() const;

	/**
	 * Set parameter space of synapses on the projection.
	 *
	 * @param parameter_space Parameter space to set
	 * @throws std::invalid_argument On parameter space not being valid for synapse type
	 * @throws std::invalid_argument On parameter space size not matching size of parameters on
	 * projection
	 */
	void set_synapse_parameter_space(Synapse::ParameterSpace const& parameter_space);

	/**
	 * Get the number of synapses of the projection.
	 */
	size_t size() const;

	/**
	 * Get connector property.
	 */
	Connector const& get_connector() const;

	/**
	 * Set connector property.
	 *
	 * @param connector Connector property to set
	 * @throws std::invalid_argument On connector synapse parameterization count not matching
	 * parameter space size.
	 */
	void set_connector(Connector const& connector);

	/**
	 * Set execution instance on the executor.
	 *
	 * @param value Value to set
	 * @throws std::invalid_argument On synapse type not being partitionable but value being given.
	 */
	virtual void set_execution_instance_on_executor(
	    std::optional<ExecutionInstanceOnExecutor> const& value) override;

	/**
	 * Get input ports to the projection.
	 * Ports are constructed in the order [Synapse::input_ports(), Connector::input_ports()].
	 */
	virtual std::vector<Port> get_input_ports() const override;

	/**
	 * Get input ports to the projection.
	 * Ports are constructed in the order [Synapse::output_ports(), Connector::output_ports()].
	 */
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::optional<TimeDomainOnTopology> get_time_domain() const override;

	/**
	 * Set time domain.
	 *
	 * @param value Value to set
	 * @throws std::invalid_argument On value not matching time domain requirement (given when not
	 * required or not given when required).
	 */
	void set_time_domain(std::optional<TimeDomainOnTopology> value);

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Vertex const& other) const override;

private:
	dapr::PropertyHolder<Synapse> m_synapse;
	dapr::PropertyHolder<Synapse::ParameterSpace> m_parameter_space;
	dapr::PropertyHolder<Connector> m_connector;
	std::optional<TimeDomainOnTopology> m_time_domain;
};

} // namespace common
} // namespace grenade
