#include "grenade/common/population.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence_dimension_unit/cell_on_population.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/port_data.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "hate/indent.h"
#include <memory>
#include <stdexcept>

namespace grenade::common {

Population::Cell::ParameterSpace::~ParameterSpace() {}


Population::Cell::~Cell() {}


Population::Population(
    Cell const& cell,
    MultiIndexSequence const& shape,
    Cell::ParameterSpace const& parameter_space,
    std::optional<TimeDomainOnTopology> const& time_domain,
    std::optional<ExecutionInstanceOnExecutor> const& execution_instance_on_executor) :
    PartitionedVertex(), m_cell(), m_parameter_space(), m_time_domain()
{
	if (!cell.valid(parameter_space)) {
		throw std::invalid_argument("Supplied parameter space invalid for supplied cell.");
	}
	if (shape.size() != parameter_space.size()) {
		throw std::invalid_argument(
		    "Supplied parameter space size doesn't match supplied population shape size.");
	}
	auto const dimension_units = shape.get_dimension_units();
	if (std::any_of(dimension_units.begin(), dimension_units.end(), [](auto const& dimension_unit) {
		    return !dimension_unit || *dimension_unit != CellOnPopulationDimensionUnit();
	    })) {
		throw std::invalid_argument(
		    "Supplied population shape requires all dimensions to have CellOnPopulation unit.");
	}
	if (cell.requires_time_domain() != static_cast<bool>(time_domain)) {
		throw std::invalid_argument(
		    "Cell time domain requirement doesn't match supplied time domain value.");
	}
	if (!cell.is_partitionable() && execution_instance_on_executor) {
		throw std::invalid_argument(
		    "Supplied cell is not partitionable but an execution instance on executor is given.");
	}
	m_cell = cell;
	m_shape = shape;
	m_parameter_space = parameter_space;
	m_time_domain = time_domain;
	set_execution_instance_on_executor(execution_instance_on_executor);
}

bool Population::valid_input_port_data(size_t input_port_on_vertex, PortData const& data) const
{
	if (auto const ptr = dynamic_cast<Cell::Dynamics const*>(&data); ptr) {
		if (ptr->size() != size()) {
			return false;
		}
		return m_cell->valid(input_port_on_vertex, *ptr);
	}
	if (auto const ptr = dynamic_cast<Cell::ParameterSpace::Parameterization const*>(&data); ptr) {
		if (ptr->size() != size()) {
			return false;
		}
		return m_parameter_space->valid(input_port_on_vertex, *ptr);
	}
	return false;
}

bool Population::valid(TimeDomainRuntimes const& time_domain_runtimes) const
{
	return m_cell->valid(time_domain_runtimes);
}

size_t Population::size() const
{
	return m_shape->size();
}

Population::Cell const& Population::get_cell() const
{
	return *m_cell;
}

void Population::set_cell(Cell const& cell)
{
	if (!cell.valid(*m_parameter_space)) {
		throw std::invalid_argument("Supplied cell invalid for parameter space.");
	}
	if (cell.requires_time_domain() != m_cell->requires_time_domain()) {
		throw std::invalid_argument("Exchanging cell of population while changing required time "
		                            "domain to true not supported.");
	}
	if (!cell.is_partitionable() && get_execution_instance_on_executor()) {
		throw std::invalid_argument(
		    "Supplied cell is not partitionable but an execution instance on executor is set.");
	}
	m_cell = cell;
}

Population::Cell::ParameterSpace const& Population::get_parameter_space() const
{
	return *m_parameter_space;
}

void Population::set_parameter_space(Cell::ParameterSpace const& parameter_space)
{
	if (!m_cell->valid(parameter_space)) {
		throw std::invalid_argument("Supplied parameter space invalid for cell.");
	}
	if (parameter_space.size() != m_shape->size()) {
		throw std::invalid_argument(
		    "Supplied parameter space size doesn't match population shape size.");
	}
	m_parameter_space = parameter_space;
}

MultiIndexSequence const& Population::get_shape() const
{
	return *m_shape;
}

void Population::set_shape(MultiIndexSequence const& value)
{
	if (value.size() != m_parameter_space->size()) {
		throw std::invalid_argument(
		    "Supplied population shape size doesn't match parameter space size.");
	}
	auto const dimension_units = value.get_dimension_units();
	if (dimension_units.empty() ||
	    std::any_of(dimension_units.begin(), dimension_units.end(), [](auto const& dimension_unit) {
		    return !dimension_unit || *dimension_unit != CellOnPopulationDimensionUnit();
	    })) {
		throw std::invalid_argument(
		    "Supplied population shape requires all dimensions to have CellOnPopulation unit.");
	}
	m_shape = value;
}

std::optional<TimeDomainOnTopology> Population::get_time_domain() const
{
	return m_time_domain;
}

void Population::set_time_domain(std::optional<TimeDomainOnTopology> value)
{
	if (m_cell->requires_time_domain() != static_cast<bool>(value)) {
		throw std::invalid_argument(
		    "Cell time domain requirement doesn't match supplied time domain value.");
	}
	m_time_domain = value;
}

void Population::set_execution_instance_on_executor(
    std::optional<ExecutionInstanceOnExecutor> const& value)
{
	if (!m_cell->is_partitionable() && value) {
		throw std::invalid_argument(
		    "Cell is not partitionable but an execution instance on executor is given.");
	}
	PartitionedVertex::set_execution_instance_on_executor(value);
}

std::vector<Vertex::Port> Population::get_input_ports() const
{
	std::vector<Port> ret;
	for (auto&& port : get_cell().get_input_ports()) {
		port.set_channels(*m_shape->cartesian_product(port.get_channels()));
		ret.emplace_back(std::move(port));
	}
	return ret;
}

std::vector<Vertex::Port> Population::get_output_ports() const
{
	std::vector<Port> ret;
	for (auto&& port : get_cell().get_output_ports()) {
		port.set_channels(*m_shape->cartesian_product(port.get_channels()));
		ret.emplace_back(std::move(port));
	}
	return ret;
}

std::unique_ptr<Vertex> Population::copy() const
{
	return std::make_unique<Population>(*this);
}

std::unique_ptr<Vertex> Population::move()
{
	return std::make_unique<Population>(std::move(*this));
}

std::ostream& Population::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Population(\n";
	ios << hate::Indentation("\t");
	PartitionedVertex::print(ios) << "\n";
	ios << "cell: " << m_cell << "\n";
	ios << "shape: " << m_shape << "\n";
	ios << "parameter_space: " << m_parameter_space << "\n";
	ios << "time_domain: ";
	if (m_time_domain) {
		ios << *m_time_domain << "\n";
	} else {
		ios << "nullopt\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool Population::is_equal_to(Vertex const& other) const
{
	auto const& other_population = static_cast<Population const&>(other);
	return PartitionedVertex::is_equal_to(other) && m_cell == other_population.m_cell &&
	       m_shape == other_population.m_shape &&
	       m_parameter_space == other_population.m_parameter_space &&
	       m_time_domain == other_population.m_time_domain;
}

} // namespace grenade::common
