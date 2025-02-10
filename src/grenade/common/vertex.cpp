#include "grenade/common/vertex.h"

#include "grenade/common/edge.h"
#include "grenade/common/port_data.h"
#include "hate/join.h"
#include <memory>
#include <ostream>
#include <set>
#include <stdexcept>
#include <log4cxx/logger.h>

namespace grenade::common {

std::ostream& operator<<(
    std::ostream& os, Vertex::Port::ExecutionInstanceTransitionConstraint const& value)
{
	switch (value) {
		case Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported: {
			os << "not_supported";
			break;
		}
		case Vertex::Port::ExecutionInstanceTransitionConstraint::supported: {
			os << "yes";
			break;
		}
		case Vertex::Port::ExecutionInstanceTransitionConstraint::required: {
			os << "required";
			break;
		}
		default: {
			throw std::logic_error("ExecutionInstanceTransitionConstraint not implemented.");
		}
	}
	return os;
}


std::ostream& operator<<(std::ostream& os, Vertex::Port::SumOrSplitSupport const& value)
{
	switch (value) {
		case Vertex::Port::SumOrSplitSupport::no: {
			os << "no";
			break;
		}
		case Vertex::Port::SumOrSplitSupport::yes: {
			os << "yes";
			break;
		}
		default: {
			throw std::logic_error("SumOrSplitSupport not implemented.");
		}
	}
	return os;
}


std::ostream& operator<<(std::ostream& os, Vertex::Port::RequiresOrGeneratesData const& value)
{
	switch (value) {
		case Vertex::Port::RequiresOrGeneratesData::no: {
			os << "no";
			break;
		}
		case Vertex::Port::RequiresOrGeneratesData::yes: {
			os << "yes";
			break;
		}
		default: {
			throw std::logic_error("RequiresOrGeneratesData not implemented.");
		}
	}
	return os;
}


Vertex::Port::Port(
    Type const& type,
    SumOrSplitSupport sum_or_split_support,
    ExecutionInstanceTransitionConstraint execution_instance_transition_constraint,
    RequiresOrGeneratesData requires_or_generates_data,
    MultiIndexSequence const& channels) :
    sum_or_split_support(sum_or_split_support),
    execution_instance_transition_constraint(execution_instance_transition_constraint),
    requires_or_generates_data(requires_or_generates_data),
    m_type(type),
    m_channels()
{
	if (!channels.is_injective()) {
		throw std::invalid_argument("Vertex port requires channels to be injective.");
	}
	m_channels = channels;
}

Vertex::Port::Port(
    Type const& type,
    SumOrSplitSupport sum_or_split_support,
    ExecutionInstanceTransitionConstraint execution_instance_transition_constraint,
    RequiresOrGeneratesData requires_or_generates_data,
    MultiIndexSequence&& channels) :
    sum_or_split_support(sum_or_split_support),
    execution_instance_transition_constraint(execution_instance_transition_constraint),
    requires_or_generates_data(requires_or_generates_data),
    m_type(type),
    m_channels()
{
	if (!channels.is_injective()) {
		throw std::invalid_argument("Vertex port requires channels to be injective.");
	}
	m_channels = std::move(channels);
}

Vertex::Port::Type const& Vertex::Port::get_type() const
{
	return *m_type;
}

void Vertex::Port::set_type(Type const& value)
{
	m_type = value;
}

MultiIndexSequence const& Vertex::Port::get_channels() const
{
	return *m_channels;
}

void Vertex::Port::set_channels(MultiIndexSequence const& value)
{
	if (!value.is_injective()) {
		throw std::invalid_argument("Vertex port requires space to be injective.");
	}
	m_channels = value;
}

void Vertex::Port::set_channels(MultiIndexSequence&& value)
{
	if (!value.is_injective()) {
		throw std::invalid_argument("Vertex port requires space to be injective.");
	}
	m_channels = std::move(value);
}

std::ostream& operator<<(std::ostream& os, Vertex::Port const& value)
{
	os << "Port(type: " << *value.m_type << ", sum_or_split_support: " << value.sum_or_split_support
	   << ", execution_instance_transition_constraint: "
	   << value.execution_instance_transition_constraint
	   << ", requires_or_generates_data: " << value.requires_or_generates_data
	   << ", channels: " << value.get_channels() << ")";
	return os;
}


Vertex::StrongComponentInvariant::~StrongComponentInvariant() {}

std::unique_ptr<Vertex::StrongComponentInvariant> Vertex::StrongComponentInvariant::copy() const
{
	return std::make_unique<StrongComponentInvariant>(*this);
}

std::unique_ptr<Vertex::StrongComponentInvariant> Vertex::StrongComponentInvariant::move()
{
	return std::make_unique<StrongComponentInvariant>(std::move(*this));
}

std::ostream& Vertex::StrongComponentInvariant::print(std::ostream& os) const
{
	return os << "StrongComponentInvariant(unique)";
}

bool Vertex::StrongComponentInvariant::is_equal_to(
    StrongComponentInvariant const& /* other */) const
{
	return false;
}


Vertex::~Vertex() {}

std::unique_ptr<Vertex::StrongComponentInvariant> Vertex::get_strong_component_invariant() const
{
	return std::make_unique<StrongComponentInvariant>();
}

bool Vertex::valid_input_port_data(
    size_t /* input_port_on_vertex */, PortData const& /* data */) const
{
	return false;
}

#define GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(...)                                              \
	LOG4CXX_ERROR(                                                                                 \
	    log4cxx::Logger::getLogger("grenade.common.Vertex"), "valid_edge_from(): " << __VA_ARGS__)

bool Vertex::valid_edge_from(Vertex const& source, Edge const& edge) const
{
	/**
	 * We check both source and target properties here in order to not unnecessarily create both the
	 * target input and source output ports in both methods.
	 */
	// check port index exists on target (us)
	auto const target_input_ports = get_input_ports();
	if (edge.port_on_target >= target_input_ports.size()) {
		GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
		    "Port on target out of range of ports on target vertex: ("
		    << edge.port_on_target << ") vs. (" << target_input_ports.size() << ").");
		return false;
	}
	// check port index exists on source
	auto const source_output_ports = source.get_output_ports();
	if (edge.port_on_source >= source_output_ports.size()) {
		GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
		    "Port on source out of range of ports on source vertex: ("
		    << edge.port_on_source << ") vs. (" << source_output_ports.size() << ").");
		return false;
	}
	// check port types match
	auto const& target_input_port = target_input_ports.at(edge.port_on_target);
	auto const& source_output_port = source_output_ports.at(edge.port_on_source);
	if (typeid(target_input_port.get_type()) != typeid(source_output_port.get_type())) {
		GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
		    "Port types don't match: source(" << source_output_port.get_type() << ") vs. target("
		                                      << target_input_port.get_type() << ").");
		return false;
	}
	// check execution instance transition constraint combination of source and target port is valid
	if ((target_input_port.execution_instance_transition_constraint ==
	         Port::ExecutionInstanceTransitionConstraint::not_supported &&
	     source_output_port.execution_instance_transition_constraint ==
	         Port::ExecutionInstanceTransitionConstraint::required) ||
	    (target_input_port.execution_instance_transition_constraint ==
	         Port::ExecutionInstanceTransitionConstraint::required &&
	     source_output_port.execution_instance_transition_constraint ==
	         Port::ExecutionInstanceTransitionConstraint::not_supported)) {
		GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
		    "Execution instance transition constraint isn't compatible: source("
		    << source_output_port.execution_instance_transition_constraint << ") vs. target("
		    << target_input_port.execution_instance_transition_constraint << ").");
		return false;
	}
	// check port requiring and generating data matches between source and target port
	if (target_input_port.requires_or_generates_data !=
	    source_output_port.requires_or_generates_data) {
		GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
		    "Source and target port generating or requiring data isn't compatible: source("
		    << source_output_port.requires_or_generates_data << ") vs. target("
		    << target_input_port.requires_or_generates_data << ").");
		return false;
	}
	// check connections lie within target input port (ours)
	auto const& channels_on_target = edge.get_channels_on_target();
	if (target_input_port.get_channels().get_dimension_units() !=
	    channels_on_target.get_dimension_units()) {
		GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
		    "Target vertex input port dimension units don't match channels on target dimension "
		    "units: ["
		    << hate::join(target_input_port.get_channels().get_dimension_units(), ", ") << "] vs. ["
		    << hate::join(channels_on_target.get_dimension_units(), ", ") << "].");
		return false;
	}
	if (!target_input_port.get_channels().includes(channels_on_target)) {
		GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
		    "Channels on target not included in target vertex input port: "
		    << channels_on_target << " vs. " << target_input_port.get_channels() << ".");
		return false;
	}
	// check connections lie within source output port
	auto const& channels_on_source = edge.get_channels_on_source();
	if (source_output_port.get_channels().get_dimension_units() !=
	    channels_on_source.get_dimension_units()) {
		GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
		    "Source vertex output port dimension units don't match channels on source dimension "
		    "units: ["
		    << hate::join(source_output_port.get_channels().get_dimension_units(), ", ")
		    << "] vs. [" << hate::join(channels_on_source.get_dimension_units(), ", ") << "].");
		return false;
	}
	if (!source_output_port.get_channels().includes(channels_on_source)) {
		GRENADE_VERTEX_VALID_EDGE_FROM_LOG_ERROR(
		    "Channels on source not included in source vertex output port: "
		    << channels_on_source << " vs. " << source_output_port.get_channels() << ".");
		return false;
	}
	return true;
}

bool Vertex::valid_edge_to(Vertex const& /* target */, Edge const& /* edge */) const
{
	/**
	 * We check both source and target properties in valid_edge_from in order to not unnecessarily
	 * create both the target input and source output ports in both methods.
	 */
	return true;
}

} // namespace grenade::common
