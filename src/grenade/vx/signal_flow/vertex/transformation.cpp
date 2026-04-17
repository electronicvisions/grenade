#include "grenade/vx/signal_flow/vertex/transformation.h"

#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "hate/indent.h"
#include "hate/variant.h"
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex {

Transformation::Dynamics::Dynamics(Value value) : value(std::move(value)) {}

size_t Transformation::Dynamics::batch_size() const
{
	return std::visit([](auto const& v) { return v.size(); }, value);
}

std::unique_ptr<grenade::common::PortData> Transformation::Dynamics::copy() const
{
	return std::make_unique<Dynamics>(*this);
}

std::unique_ptr<grenade::common::PortData> Transformation::Dynamics::move()
{
	return std::make_unique<Dynamics>(std::move(*this));
}

bool Transformation::Dynamics::is_equal_to(grenade::common::PortData const& other) const
{
	return value == static_cast<Dynamics const&>(other).value;
}

std::ostream& Transformation::Dynamics::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Dynamics(\n";
	ios << hate::Indentation("\t");
	// FIXME: implement print
	// ios << hate::join(spikes, "\n");
	ios << hate::Indentation() << ")";
	return os;
}


Transformation::Function::Port::Port(
    grenade::common::Vertex::Port::Type const& type,
    grenade::common::MultiIndexSequence const& channels) :
    m_type(type), m_channels(channels)
{
}

grenade::common::Vertex::Port::Type const& Transformation::Function::Port::get_type() const
{
	return *m_type;
}

void Transformation::Function::Port::set_type(grenade::common::Vertex::Port::Type const& value)
{
	m_type = value;
}

grenade::common::MultiIndexSequence const& Transformation::Function::Port::get_channels() const
{
	return *m_channels;
}

void Transformation::Function::Port::set_channels(grenade::common::MultiIndexSequence const& value)
{
	m_channels = value;
}

std::ostream& operator<<(std::ostream& os, Transformation::Function::Port const& value)
{
	return os << "Port(type: " << value.m_type << ", channels: " << value.m_channels << ")";
}


Transformation::Function::~Function() {}

bool Transformation::Function::valid_input_port_data(size_t, grenade::common::PortData const&) const
{
	return false;
}


Transformation::Results::Results(Value value) : value(std::move(value)) {}

size_t Transformation::Results::batch_size() const
{
	std::set<size_t> sizes;
	sizes.insert(std::visit([](auto const& d) { return d.size(); }, value));
	if (sizes.size() != 1) {
		throw std::runtime_error("Results batch size not unique or empty values.");
	}
	return *sizes.begin();
}

std::unique_ptr<Transformation::Results> Transformation::Results::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	return std::make_unique<Results>(std::visit(
	    hate::overloaded{
	        [&](std::vector<common::TimedDataSequence<std::vector<UInt5>>> const& val) -> Value {
		        std::vector<common::TimedDataSequence<std::vector<UInt5>>> ret(val.size());
		        auto const sequence_elements = sequence.get_elements();
		        for (size_t b = 0; b < ret.size(); ++b) {
			        ret.at(b).reserve(val.at(b).size());
			        for (auto const& entry : val.at(b)) {
				        std::vector<UInt5> local_data;
				        local_data.reserve(sequence_elements.size());
				        for (auto const& element : sequence_elements) {
					        local_data.push_back(entry.data.at(element.value.at(0)));
				        }
				        ret.at(b).push_back(common::TimedData<std::vector<UInt5>>(
				            entry.time, std::move(local_data)));
			        }
		        }
		        return ret;
	        },
	        [&](std::vector<common::TimedDataSequence<std::vector<Int8>>> const& val) -> Value {
		        std::vector<common::TimedDataSequence<std::vector<Int8>>> ret(val.size());
		        auto const sequence_elements = sequence.get_elements();
		        for (size_t b = 0; b < ret.size(); ++b) {
			        ret.at(b).reserve(val.at(b).size());
			        for (auto const& entry : val.at(b)) {
				        std::vector<Int8> local_data;
				        local_data.reserve(sequence_elements.size());
				        for (auto const& element : sequence_elements) {
					        local_data.push_back(entry.data.at(element.value.at(0)));
				        }
				        ret.at(b).push_back(common::TimedData<std::vector<Int8>>(
				            entry.time, std::move(local_data)));
			        }
		        }
		        return ret;
	        },
	        [&](std::vector<common::TimedDataSequence<std::vector<UInt32>>> const& val) -> Value {
		        std::vector<common::TimedDataSequence<std::vector<UInt32>>> ret(val.size());
		        auto const sequence_elements = sequence.get_elements();
		        for (size_t b = 0; b < ret.size(); ++b) {
			        ret.at(b).reserve(val.at(b).size());
			        for (auto const& entry : val.at(b)) {
				        std::vector<UInt32> local_data;
				        local_data.reserve(sequence_elements.size());
				        for (auto const& element : sequence_elements) {
					        local_data.push_back(entry.data.at(element.value.at(0)));
				        }
				        ret.at(b).push_back(common::TimedData<std::vector<UInt32>>(
				            entry.time, std::move(local_data)));
			        }
		        }
		        return ret;
	        },
	        [](auto const& val) -> Value { return val; }},
	    value));
}

std::unique_ptr<grenade::common::PortData> Transformation::Results::copy() const
{
	return std::make_unique<Results>(*this);
}

std::unique_ptr<grenade::common::PortData> Transformation::Results::move()
{
	return std::make_unique<Results>(std::move(*this));
}

std::ostream& Transformation::Results::print(std::ostream& os) const
{
	return os << "Results(TODO)";
}

bool Transformation::Results::is_equal_to(grenade::common::PortData const& other) const
{
	return value == static_cast<Results const&>(other).value;
}


Transformation::Transformation(
    Function const& function,
    std::optional<grenade::common::ExecutionInstanceOnExecutor> const&
        execution_instance_on_executor) :
    PartitionedVertex(execution_instance_on_executor), m_function(function)
{
}

Transformation::Function const& Transformation::get_function() const
{
	return *m_function;
}

void Transformation::set_function(Function const& value)
{
	m_function = value;
}

bool Transformation::valid_output_port_data(
    size_t output_port_on_vertex, grenade::common::PortData const& data) const
{
	if (output_port_on_vertex != 0) {
		return false;
	}
	auto const port_type = dynamic_cast<VertexPortType const&>(
	                           m_function->get_output_ports().at(output_port_on_vertex).get_type())
	                           .type;
	if (auto const transformation_results = dynamic_cast<Results const*>(&data);
	    transformation_results) {
		if ((port_type == signal_flow::ConnectionType::DataUInt32) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataUInt5) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataInt8) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataTimedSpikeToChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedSpikeToChipSequence>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataTimedSpikeFromChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataTimedMADCSampleFromChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedMADCSampleFromChipSequence>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::UInt32) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::UInt5) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::Int8) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::TimedSpikeToChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedSpikeToChipSequence>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::TimedSpikeFromChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
		        transformation_results->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::TimedMADCSampleFromChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedMADCSampleFromChipSequence>>(
		        transformation_results->value)) {
			return false;
		}
		return true;
	}
	return false;
}

bool Transformation::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	return m_function->valid_input_port_data(input_port_on_vertex, data);
}

std::vector<grenade::common::Vertex::Port> Transformation::get_input_ports() const
{
	auto const function_ports = m_function->get_input_ports();
	std::vector<Port> ports;
	for (auto& function_port : function_ports) {
		ports.emplace_back(Port(
		    function_port.get_type(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
		    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
		    function_port.get_channels()));
	}
	return ports;
}

std::vector<grenade::common::Vertex::Port> Transformation::get_output_ports() const
{
	auto const function_ports = m_function->get_output_ports();
	std::vector<Port> ports;
	for (auto& function_port : function_ports) {
		ports.emplace_back(Port(
		    function_port.get_type(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
		    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
		    function_port.get_channels()));
	}
	return ports;
}

std::unique_ptr<grenade::common::Vertex> Transformation::copy() const
{
	return std::make_unique<Transformation>(*this);
}

std::unique_ptr<grenade::common::Vertex> Transformation::move()
{
	return std::make_unique<Transformation>(*this);
}

bool Transformation::is_equal_to(grenade::common::Vertex const& other) const
{
	auto const& o = static_cast<Transformation const&>(other);
	return PartitionedVertex::is_equal_to(other) && m_function == o.m_function;
}

std::ostream& Transformation::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Transformation(\n";
	ios << hate::Indentation("\t");
	PartitionedVertex::print(ios) << "\n";
	ios << m_function;
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::signal_flow::vertex
