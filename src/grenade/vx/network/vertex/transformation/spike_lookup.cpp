#include "grenade/vx/network/vertex/transformation/spike_lookup.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "hate/indent.h"
#include "hate/timer.h"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <log4cxx/logger.h>

namespace grenade::vx::network::vertex::transformation {

SpikeLookup::Parameterization::Parameterization(Translation translation) :
    translation(std::move(translation))
{
}

std::unique_ptr<grenade::common::PortData> SpikeLookup::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> SpikeLookup::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

bool SpikeLookup::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	return translation == static_cast<Parameterization const&>(other).translation;
}

std::ostream& SpikeLookup::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	for (auto const& [source_label, targets] : translation) {
		ios << hate::Indentation("\t");
		ios << source_label << ": \n";
		ios << hate::Indentation("\t\t");
		for (auto const& [target_label, delay] : targets) {
			ios << target_label << ": " << delay << "\n";
		}
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}


bool SpikeLookup::valid_input_port_data(
    size_t input_port_on_function, grenade::common::PortData const& data) const
{
	return (input_port_on_function == 0 &&
	        dynamic_cast<signal_flow::vertex::Transformation::Dynamics const*>(&data) != nullptr) ||
	       (input_port_on_function == 1 && dynamic_cast<Parameterization const*>(&data) != nullptr);
}

std::vector<signal_flow::vertex::Transformation::Function::Port> SpikeLookup::get_input_ports()
    const
{
	return {
	    Port(
	        signal_flow::VertexPortType(signal_flow::ConnectionType::TimedSpikeFromChipSequence),
	        grenade::common::CuboidMultiIndexSequence({1})),
	    Port(
	        ParameterizationPortType(),
	        grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<signal_flow::vertex::Transformation::Function::Port> SpikeLookup::get_output_ports()
    const
{
	return {Port(
	    signal_flow::VertexPortType(signal_flow::ConnectionType::TimedSpikeToChipSequence),
	    grenade::common::CuboidMultiIndexSequence({1}))};
}

std::vector<signal_flow::vertex::Transformation::Results> SpikeLookup::apply(
    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& value) const
{
	if (value.size() != 2) {
		throw std::invalid_argument("SpikeLookup expects two input values.");
	}
	auto const& translation = dynamic_cast<Parameterization const&>(value.at(1).get()).translation;
	size_t batch_size = std::visit(
	    [](auto const& v) { return v.size(); },
	    dynamic_cast<signal_flow::vertex::Transformation::Dynamics const&>(value.at(0).get())
	        .value);

	std::vector<signal_flow::TimedSpikeToChipSequence> ret(batch_size);

	std::chrono::microseconds log_inter_execution_instance_input_duration{0};

	for (size_t b = 0; b < batch_size; ++b) {
		auto& local_ret = ret.at(b);
		auto const& local_value =
		    dynamic_cast<signal_flow::vertex::Transformation::Dynamics const&>(value.at(0).get())
		        .value;

		hate::Timer timer;
		auto const& local_input_events =
		    std::get<std::vector<signal_flow::TimedSpikeFromChipSequence>>(local_value).at(b);
		for (auto const& local_input_event : local_input_events) {
			if (translation.contains(local_input_event.data))
				for (auto const& [label, delay] : translation.at(local_input_event.data)) {
					local_ret.push_back(signal_flow::TimedSpikeToChip(
					    local_input_event.time + delay, haldls::vx::v3::SpikePack1ToChip({label})));
				}
			log_inter_execution_instance_input_duration +=
			    std::chrono::microseconds{timer.get_us()};
		}
	}

	auto logger = log4cxx::Logger::getLogger("grenade.network.SpikeLookup");
	LOG4CXX_TRACE(
	    logger, "apply(): Merging of inter-execution-instance events performed in "
	                << hate::to_string(log_inter_execution_instance_input_duration) << ".");
	return {signal_flow::vertex::Transformation::Results(std::move(ret))};
}

bool SpikeLookup::is_equal_to(signal_flow::vertex::Transformation::Function const&) const
{
	return true;
}

std::ostream& SpikeLookup::print(std::ostream& os) const
{
	return os << "SpikeLookup()";
}

std::unique_ptr<signal_flow::vertex::Transformation::Function> SpikeLookup::copy() const
{
	return std::make_unique<SpikeLookup>(*this);
}

std::unique_ptr<signal_flow::vertex::Transformation::Function> SpikeLookup::move()
{
	return std::make_unique<SpikeLookup>(std::move(*this));
}

} // namespace grenade::vx::network::vertex::transformation
