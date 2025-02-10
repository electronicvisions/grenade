#include "grenade/common/projection.h"

#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "hate/indent.h"
#include <memory>

namespace grenade::common {

Projection::Synapse::ParameterSpace::Parameterization::~Parameterization() {}


Projection::Synapse::ParameterSpace::~ParameterSpace() {}


Projection::Synapse::Dynamics::~Dynamics() {}


Projection::Synapse::~Synapse() {}


Projection::Connector::Parameterization::~Parameterization() {}


Projection::Connector::Dynamics::~Dynamics() {}


Projection::Connector::~Connector() {}

namespace {

void check_valid(
    Projection::Synapse const& synapse,
    Projection::Synapse::ParameterSpace const& parameter_space,
    Projection::Connector const& connector,
    std::optional<TimeDomainOnTopology> const& time_domain,
    std::optional<ExecutionInstanceOnExecutor> const& execution_instance_on_executor)
{
	auto const connector_shape =
	    connector.get_input_sequence()->cartesian_product(*connector.get_output_sequence());
	if (connector.get_num_synapse_parameterizations(*connector_shape) != parameter_space.size()) {
		throw std::invalid_argument(
		    "Connector expects different number of synapse "
		    "parameterizations (" +
		    std::to_string(connector.get_num_synapse_parameterizations(*connector_shape)) +
		    ") as provided by parameter space (" + std::to_string(parameter_space.size()) + ").");
	}
	if (!synapse.valid(parameter_space)) {
		throw std::invalid_argument("Parameter space not valid for given synapse type.");
	}
	if (synapse.requires_time_domain() != static_cast<bool>(time_domain)) {
		throw std::invalid_argument(
		    "Synapse time domain requirement doesn't match supplied time domain value.");
	}
	if (!synapse.is_partitionable() && static_cast<bool>(execution_instance_on_executor)) {
		throw std::invalid_argument(
		    "Supplied execution instance on executor, but synapse is not partitionable.");
	}
}

} // namespace

Projection::Projection(
    Synapse const& synapse,
    Synapse::ParameterSpace const& parameter_space,
    Connector const& connector,
    std::optional<TimeDomainOnTopology> const& time_domain,
    std::optional<ExecutionInstanceOnExecutor> const& execution_instance_on_executor) :
    PartitionedVertex(), m_synapse(), m_parameter_space(), m_connector(), m_time_domain()
{
	check_valid(synapse, parameter_space, connector, time_domain, execution_instance_on_executor);
	m_synapse = synapse;
	m_parameter_space = parameter_space;
	m_connector = connector;
	m_time_domain = time_domain;
	set_execution_instance_on_executor(execution_instance_on_executor);
}

bool Projection::valid_input_port_data(size_t input_port_on_vertex, PortData const& data) const
{
	auto const num_input_ports = get_input_ports().size();
	if (input_port_on_vertex >= num_input_ports) {
		return false;
	}
	auto const num_connector_input_ports = m_connector->get_input_ports().size();
	auto const num_synapse_input_ports = num_input_ports - num_connector_input_ports;
	if (input_port_on_vertex < num_synapse_input_ports) {
		if (auto const synapse_dynamics = dynamic_cast<Synapse::Dynamics const*>(&data);
		    synapse_dynamics) {
			auto const connector_shape = m_connector->get_input_sequence()->cartesian_product(
			    *m_connector->get_output_sequence());
			return m_synapse->valid(input_port_on_vertex, *synapse_dynamics) &&
			       synapse_dynamics->size() ==
			           m_connector->get_num_synapse_parameterizations(*connector_shape);
		}
	} else {
		if (auto const connector_dynamics = dynamic_cast<Connector::Dynamics const*>(&data);
		    connector_dynamics) {
			return m_connector->valid(
			    input_port_on_vertex - num_synapse_input_ports, *connector_dynamics);
		}
	}
	if (input_port_on_vertex < num_synapse_input_ports) {
		if (auto const synapse_parameterization =
		        dynamic_cast<Synapse::ParameterSpace::Parameterization const*>(&data);
		    synapse_parameterization) {
			auto const connector_shape = m_connector->get_input_sequence()->cartesian_product(
			    *m_connector->get_output_sequence());
			return m_parameter_space->valid(input_port_on_vertex, *synapse_parameterization) &&
			       synapse_parameterization->size() ==
			           m_connector->get_num_synapse_parameterizations(*connector_shape);
		}
	} else {
		if (auto const connector_parameterization =
		        dynamic_cast<Connector::Parameterization const*>(&data);
		    connector_parameterization) {
			return m_connector->valid(
			    input_port_on_vertex - num_synapse_input_ports, *connector_parameterization);
		}
	}
	// should not be reached
	return false;
}

bool Projection::valid(TimeDomainRuntimes const& time_domain_runtimes) const
{
	return m_synapse->valid(time_domain_runtimes);
}

Projection::Synapse const& Projection::get_synapse() const
{
	return *m_synapse;
}

void Projection::set_synapse(Synapse const& synapse)
{
	check_valid(
	    synapse, *m_parameter_space, *m_connector, m_time_domain,
	    get_execution_instance_on_executor());
	m_synapse = synapse;
}

Projection::Synapse::ParameterSpace const& Projection::get_synapse_parameter_space() const
{
	return *m_parameter_space;
}

void Projection::set_synapse_parameter_space(Synapse::ParameterSpace const& parameter_space)
{
	check_valid(
	    *m_synapse, parameter_space, *m_connector, m_time_domain,
	    get_execution_instance_on_executor());
	m_parameter_space = parameter_space;
}

size_t Projection::size() const
{
	auto const connector_shape = get_connector().get_input_sequence()->cartesian_product(
	    *get_connector().get_output_sequence());
	return m_connector->get_num_synapses(*connector_shape);
}

Projection::Connector const& Projection::get_connector() const
{
	return *m_connector;
}

void Projection::set_connector(Connector const& connector)
{
	check_valid(
	    *m_synapse, *m_parameter_space, connector, m_time_domain,
	    get_execution_instance_on_executor());
	m_connector = connector;
}

std::optional<TimeDomainOnTopology> Projection::get_time_domain() const
{
	return m_time_domain;
}

void Projection::set_time_domain(std::optional<TimeDomainOnTopology> value)
{
	check_valid(
	    *m_synapse, *m_parameter_space, *m_connector, value, get_execution_instance_on_executor());
	m_time_domain = value;
}

void Projection::set_execution_instance_on_executor(
    std::optional<ExecutionInstanceOnExecutor> const& value)
{
	check_valid(*m_synapse, *m_parameter_space, *m_connector, m_time_domain, value);
	PartitionedVertex::set_execution_instance_on_executor(value);
}

std::vector<Projection::Port> Projection::get_input_ports() const
{
	auto synapse_ports = get_synapse().get_input_ports();
	auto const& connector = get_connector();
	std::vector<Port> ports;
	auto connector_input_sequence = connector.get_input_sequence();
	for (auto&& port : synapse_ports.projection) {
		port.set_channels(*connector_input_sequence->cartesian_product(port.get_channels()));
		ports.push_back(std::move(port));
	}
	auto const connector_shape =
	    connector_input_sequence->cartesian_product(*connector.get_output_sequence());
	auto const num_synapses = connector.get_num_synapses(*connector_shape);
	for (auto&& port : synapse_ports.synapse) {
		port.set_channels(
		    *CuboidMultiIndexSequence({num_synapses}).cartesian_product(port.get_channels()));
		ports.push_back(std::move(port));
	}
	auto const num_synapse_parameterizations =
	    connector.get_num_synapse_parameterizations(*connector_shape);
	for (auto&& port : synapse_ports.synapse_parameterization) {
		port.set_channels(*CuboidMultiIndexSequence({num_synapse_parameterizations})
		                       .cartesian_product(port.get_channels()));
		ports.push_back(std::move(port));
	}
	auto connector_ports = get_connector().get_input_ports();
	for (auto&& port : connector_ports) {
		ports.push_back(std::move(port));
	}
	return ports;
}

std::vector<Projection::Port> Projection::get_output_ports() const
{
	auto synapse_ports = get_synapse().get_output_ports();
	auto const& connector = get_connector();
	std::vector<Port> ports;
	auto connector_output_sequence = connector.get_output_sequence();
	for (auto&& port : synapse_ports.projection) {
		port.set_channels(*connector_output_sequence->cartesian_product(port.get_channels()));
		ports.push_back(std::move(port));
	}
	auto const connector_shape =
	    connector.get_input_sequence()->cartesian_product(*connector_output_sequence);
	auto const num_synapses = connector.get_num_synapses(*connector_shape);
	for (auto&& port : synapse_ports.synapse) {
		port.set_channels(
		    *CuboidMultiIndexSequence({num_synapses}).cartesian_product(port.get_channels()));
		ports.push_back(std::move(port));
	}
	auto const num_synapse_parameterizations =
	    connector.get_num_synapse_parameterizations(*connector_shape);
	for (auto&& port : synapse_ports.synapse_parameterization) {
		port.set_channels(*CuboidMultiIndexSequence({num_synapse_parameterizations})
		                       .cartesian_product(port.get_channels()));
		ports.push_back(std::move(port));
	}
	auto connector_ports = get_connector().get_output_ports();
	for (auto&& port : connector_ports) {
		ports.push_back(std::move(port));
	}
	return ports;
}

std::ostream& Projection::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Projection(\n";
	ios << hate::Indentation("\t");
	PartitionedVertex::print(ios) << "\n";
	ios << m_synapse << "\n";
	ios << m_parameter_space << "\n";
	ios << m_connector << "\n";
	if (m_time_domain) {
		ios << *m_time_domain << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

std::unique_ptr<Vertex> Projection::copy() const
{
	return std::make_unique<Projection>(*this);
}

std::unique_ptr<Vertex> Projection::move()
{
	return std::make_unique<Projection>(std::move(*this));
}

bool Projection::is_equal_to(Vertex const& other) const
{
	auto const& other_projection = static_cast<Projection const&>(other);
	return PartitionedVertex::is_equal_to(other) && m_synapse == other_projection.m_synapse &&
	       m_parameter_space == other_projection.m_parameter_space &&
	       m_connector == other_projection.m_connector &&
	       m_time_domain == other_projection.m_time_domain;
}

} // namespace grenade::common
