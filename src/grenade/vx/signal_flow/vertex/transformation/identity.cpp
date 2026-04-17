#include "grenade/vx/signal_flow/vertex/transformation/identity.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"

#include <functional>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

Identity::Identity(std::vector<Port> output_ports) : m_output_ports(output_ports) {}

std::vector<Transformation::Results> Identity::apply(
    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data) const
{
	std::vector<Transformation::Results> ret;
	for (auto const& v : data) {
		ret.push_back(dynamic_cast<Transformation::Dynamics const&>(v.get()).value);
	}

	return ret;
}

bool Identity::valid_input_port_data(
    size_t input_port_on_function, grenade::common::PortData const& data) const
{
	if (input_port_on_function != 0) {
		return false;
	}
	auto const port_type =
	    dynamic_cast<VertexPortType const&>(get_input_ports().at(input_port_on_function).get_type())
	        .type;
	if (auto const transformation_inputs = dynamic_cast<Transformation::Dynamics const*>(&data);
	    transformation_inputs) {
		if ((port_type == signal_flow::ConnectionType::DataUInt32) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataUInt5) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataInt8) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataTimedSpikeToChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedSpikeToChipSequence>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataTimedSpikeFromChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::DataTimedMADCSampleFromChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedMADCSampleFromChipSequence>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::UInt32) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::UInt5) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::Int8) &&
		    !std::holds_alternative<
		        std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::TimedSpikeToChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedSpikeToChipSequence>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::TimedSpikeFromChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
		        transformation_inputs->value)) {
			return false;
		} else if (
		    (port_type == signal_flow::ConnectionType::TimedMADCSampleFromChipSequence) &&
		    !std::holds_alternative<std::vector<signal_flow::TimedMADCSampleFromChipSequence>>(
		        transformation_inputs->value)) {
			return false;
		}
		return true;
	}
	return false;
}

std::vector<Transformation::Function::Port> Identity::get_input_ports() const
{
	return m_output_ports;
}

std::vector<Transformation::Function::Port> Identity::get_output_ports() const
{
	return m_output_ports;
}

std::unique_ptr<Transformation::Function> Identity::copy() const
{
	return std::make_unique<Identity>(*this);
}

std::unique_ptr<Transformation::Function> Identity::move()
{
	return std::make_unique<Identity>(std::move(*this));
}

bool Identity::is_equal_to(Function const& other) const
{
	return m_output_ports == static_cast<Identity const&>(other).m_output_ports;
}

std::ostream& Identity::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Identity(\n";
	ios << hate::Indentation("\t");
	for (auto const& port : m_output_ports) {
		ios << port << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::signal_flow::vertex::transformation
