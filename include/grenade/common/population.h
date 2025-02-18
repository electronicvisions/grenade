#pragma once
#include "dapr/property.h"
#include "dapr/property_holder.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/port_data.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/genpybind.h"
#include "halco/common/geometry.h"
#include "hate/visibility.h"
#include <memory>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Population in topology.
 * A population is a collection of cells of the same type with potentially heterogeneous parameter
 * spaces.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    visible, holder_type("std::shared_ptr<grenade::common::Population>")) Population
    : public PartitionedVertex
{
	/**
	 * Cell type of population.
	 * The cell type defines the population cells' computation performed, its parameters and the
	 * ports accessible per cell.
	 */
	struct GENPYBIND(inline_base("*")) Cell : public dapr::Property<Cell>
	{
		/**
		 * Parameter space matching cell type.
		 * It constrains the possible parameterizations and might be heterogeneous across the
		 * population. It is part of the topological description.
		 */
		struct GENPYBIND(inline_base("*")) ParameterSpace : public dapr::Property<ParameterSpace>
		{
			/**
			 * Parameterization of cell within parameter space.
			 * It is supplied separately as parameterization of the topology.
			 */
			struct Parameterization : public PortData
			{
				/**
				 * Number of cell parameterizations.
				 */
				virtual size_t size() const = 0;

				/**
				 * Get section of parameterization.
				 * The sequence is required to be included in the interval [0, size()).
				 */
				virtual std::unique_ptr<Parameterization> get_section(
				    MultiIndexSequence const& sequence) const = 0;
			};

			virtual ~ParameterSpace();

			/**
			 * Number of cell parameter spaces.
			 */
			virtual size_t size() const = 0;

			/**
			 * Check whether the parameterization is valid for the parameter space.
			 */
			virtual bool valid(
			    size_t input_port_on_cell, Parameterization const& parameterization) const = 0;

			/**
			 * Get section of parameter space.
			 * The sequence is required to be included in the interval [0, size()).
			 */
			virtual std::unique_ptr<ParameterSpace> get_section(
			    MultiIndexSequence const& sequence) const = 0;
		};

		/**
		 * Dynamics for the cells.
		 * It is supplied separately as dynamics of the topology.
		 */
		struct Dynamics : public BatchedPortData
		{
			/**
			 * Number of cell dynamics.
			 */
			virtual size_t size() const = 0;

			/**
			 * Get section of dynamics.
			 * The sequence is required to be included in the interval [0, size()).
			 */
			virtual std::unique_ptr<Dynamics> get_section(
			    MultiIndexSequence const& sequence) const = 0;
		};

		virtual ~Cell();

		/**
		 * Get whether the parameter space is valid for the cell.
		 */
		virtual bool valid(ParameterSpace const& parameter_space) const = 0;

		/**
		 * Get whether the dynamics are valid for the cell.
		 */
		virtual bool valid(size_t input_port_on_cell, Dynamics const& dynamics) const = 0;

		/**
		 * Get the input ports this cell exposes.
		 */
		virtual std::vector<Port> get_input_ports() const = 0;

		/**
		 * Get the output ports this cell exposes.
		 */
		virtual std::vector<Port> get_output_ports() const = 0;

		/**
		 * Get whether the cell requires a time domain.
		 */
		virtual bool requires_time_domain() const = 0;

		/**
		 * Get whether the time domain runtimes are valid for the cell.
		 */
		virtual bool valid(TimeDomainRuntimes const& time_domain_runtimes) const = 0;

		/**
		 * Get whether the cell is partitionable.
		 * This means that it can be assigned an execution instance on the executor.
		 */
		virtual bool is_partitionable() const = 0;
	};

	/**
	 * Construct population.
	 *
	 * @param cell Cell type
	 * @param shape Shape of cells on population
	 * @param parameter_space Parameter space of cells
	 * @param time_domain Time domain
	 * @param execution_instance_on_executor Optional execution instance on executor
	 * @throws std::invalid_argument On parameter space being invalid for cell type.
	 * @throws std::invalid_argument On parameter space size not matching size of shape.
	 * @throws std::invalid_argument On shape dimension units not all being
	 * CellOnPopulationDimensionUnit.
	 * @throws std::invalid_argument On time domain given not matching requirement by cell (value
	 * given but not required or no valud given but required).
	 * @throws std::invalid_argument On execution instance on executor being given but cell not
	 * being partitionable.
	 */
	Population(
	    Cell const& cell,
	    MultiIndexSequence const& shape,
	    Cell::ParameterSpace const& parameter_space,
	    std::optional<TimeDomainOnTopology> const& time_domain,
	    std::optional<ExecutionInstanceOnExecutor> const& execution_instance_on_executor =
	        std::nullopt);

	/**
	 * Get whether the data is valid for the input port on population.
	 * This matches the checks for dynamics and parameterization defined for the cell and
	 * additionally checks for matching size.
	 */
	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, PortData const& data) const override;

	/**
	 * Get whether the time domain is valid for the population.
	 * This matches the check defined for the cell.
	 */
	virtual bool valid(TimeDomainRuntimes const& time_domain_runtimes) const override;

	/**
	 * Get number of cells in population.
	 * This equals the size of the parameter space as well as the shape.
	 * @return Population size
	 */
	size_t size() const;

	/**
	 * Get sequence of cells in population.
	 * Each cell is assigned a ``position'' in the space in which the shape is embedded.
	 * For the input and output ports, the shape is used to extend the channels given by the
	 * cell to the population.
	 * @return Population shape
	 */
	MultiIndexSequence const& get_shape() const GENPYBIND(hidden);

	GENPYBIND_MANUAL({
		parent.def(
		    "get_shape", [](GENPYBIND_PARENT_TYPE const& self) { return self.get_shape().copy(); });
	})

	/**
	 * Set sequence of cells in population.
	 *
	 * @param value Population shape
	 * @throws std::invalid_argument On size of shape not matching parameter space.
	 * @throws std::invalid_argument On not all dimension units being
	 * CellOnPopulationDimensionUnit.
	 */
	void set_shape(MultiIndexSequence const& value);

	/**
	 * Get cell type property.
	 * The cell type defines the population cells' computation performed, its parameters and the
	 * ports accessible per cell.
	 */
	Cell const& get_cell() const;

	/**
	 * Set cell type property.
	 * The cell type defines the population cells' computation performed, its parameters and the
	 * ports accessible per cell.
	 *
	 * @param cell Cell type to set
	 * @throws std::invalid_argument On parameter space not valid for cell type.
	 * @throws std::invalid_argument On cell type featuring different time domain requirement to
	 * previously contained cell type.
	 * @throws std::invalid_argument On cell type not being partitionable but an execution
	 * instance on the executor being assigned to population.
	 */
	void set_cell(Cell const& cell);

	/**
	 * Get parameter space of cell instances on the population.
	 */
	Cell::ParameterSpace const& get_parameter_space() const;

	/**
	 * Set parameter space of cell instances on the population.
	 *
	 * @param parameter_space Parameter space to set
	 * @throws On parameter space not being valid for cell type
	 * @throws On parameter space size not matching size of shape
	 */
	void set_parameter_space(Cell::ParameterSpace const& parameter_space);

	virtual std::optional<TimeDomainOnTopology> get_time_domain() const override;

	/**
	 * Set time domain.
	 *
	 * @param value Value to set
	 * @throws std::invalid_argument On value not matching time domain requirement (given when not
	 * required or not given when required).
	 */
	void set_time_domain(std::optional<TimeDomainOnTopology> value);

	/**
	 * Set execution instance on the executor.
	 *
	 * @param value Value to set
	 * @throws std::invalid_argument On cell type not being partitionable but value being given.
	 */
	void set_execution_instance_on_executor(
	    std::optional<ExecutionInstanceOnExecutor> const& value) override;

	/**
	 * Get the input ports of the population.
	 * They match the input ports of the cell with the channels of each port being the cross
	 * product with the shape.
	 */
	virtual std::vector<Port> get_input_ports() const override;

	/**
	 * Get the output ports of the population.
	 * They match the output ports of the cell with the channels of each port being the cross
	 * product with the shape.
	 */
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Vertex const& other) const override;

private:
	dapr::PropertyHolder<Cell> m_cell;
	dapr::PropertyHolder<MultiIndexSequence> m_shape;
	dapr::PropertyHolder<Cell::ParameterSpace> m_parameter_space;
	std::optional<TimeDomainOnTopology> m_time_domain;
};

} // namespace common
} // namespace grenade
