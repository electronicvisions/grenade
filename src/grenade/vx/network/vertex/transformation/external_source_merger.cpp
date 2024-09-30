#include "grenade/vx/network/vertex/transformation/external_source_merger.h"

#include "grenade/vx/signal_flow/event.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <algorithm>
#include <stdexcept>
#include <log4cxx/logger.h>

namespace grenade::vx::network::vertex::transformation {

bool ExternalSourceMerger::InterExecutionInstanceInput::operator==(
    InterExecutionInstanceInput const& other) const
{
	return translation == other.translation;
}

bool ExternalSourceMerger::ExternalInput::operator==(ExternalInput const&) const
{
	return true;
}

ExternalSourceMerger::ExternalSourceMerger(std::vector<Input> const& inputs) : m_inputs(inputs) {}

ExternalSourceMerger::~ExternalSourceMerger() {}

std::vector<signal_flow::Port> ExternalSourceMerger::inputs() const
{
	std::vector<signal_flow::Port> ports;
	for (auto const& input : m_inputs) {
		ports.push_back(
		    signal_flow::Port(1, std::visit([](auto const& in) { return in.type; }, input)));
	}
	return ports;
}

signal_flow::Port ExternalSourceMerger::output() const
{
	return signal_flow::Port(1, signal_flow::ConnectionType::TimedSpikeToChipSequence);
}

ExternalSourceMerger::Function::Value ExternalSourceMerger::apply(
    std::vector<Function::Value> const& value) const
{
	size_t batch_size = 0;
	if (!value.empty()) {
		batch_size = std::visit([](auto const& v) { return v.size(); }, value.at(0));
	}

	std::vector<signal_flow::TimedSpikeToChipSequence> ret(batch_size);

	std::chrono::microseconds log_inter_execution_instance_input_duration{0};

	for (size_t b = 0; b < batch_size; ++b) {
		auto& local_ret = ret.at(b);
		for (size_t i = 0; i < m_inputs.size(); ++i) {
			auto const& local_value = value.at(i);

			auto const add_external_input = [b, local_value, &local_ret](ExternalInput const&) {
				auto const& local_events =
				    std::get<std::vector<signal_flow::TimedSpikeToChipSequence>>(local_value).at(b);
				local_ret.insert(local_ret.end(), local_events.begin(), local_events.end());
			};

			auto const add_inter_execution_instance_input =
			    [b, local_value, &local_ret, &log_inter_execution_instance_input_duration](
			        InterExecutionInstanceInput const& input) {
				    hate::Timer timer;
				    auto const& local_input_events =
				        std::get<std::vector<signal_flow::TimedSpikeFromChipSequence>>(local_value)
				            .at(b);
				    for (auto const& local_input_event : local_input_events) {
					    if (input.translation.contains(local_input_event.data))
						    for (auto const& [label, delay] :
						         input.translation.at(local_input_event.data)) {
							    local_ret.push_back(signal_flow::TimedSpikeToChip(
							        local_input_event.time + delay,
							        haldls::vx::v3::SpikePack1ToChip({label})));
						    }
				    }
				    log_inter_execution_instance_input_duration +=
				        std::chrono::microseconds{timer.get_us()};
			    };

			std::visit(
			    hate::overloaded{add_external_input, add_inter_execution_instance_input},
			    m_inputs.at(i));
		}
		if (m_inputs.size() > 1) {
			hate::Timer timer;
			std::stable_sort(local_ret.begin(), local_ret.end(), [](auto const& a, auto const& b) {
				return a.time < b.time;
			});
			// only triggered if we have such an input
			log_inter_execution_instance_input_duration +=
			    std::chrono::microseconds{timer.get_us()};
		}
	}

	auto logger = log4cxx::Logger::getLogger("grenade.network.ExternalSourceMerger");
	LOG4CXX_TRACE(
	    logger, "apply(): Merging of inter-execution-instance events performed in "
	                << hate::to_string(log_inter_execution_instance_input_duration) << ".");
	return ret;
}

bool ExternalSourceMerger::equal(signal_flow::vertex::Transformation::Function const& other) const
{
	ExternalSourceMerger const* o = dynamic_cast<ExternalSourceMerger const*>(&other);
	if (o == nullptr) {
		return false;
	}
	return m_inputs == o->m_inputs;
}

std::unique_ptr<signal_flow::vertex::Transformation::Function> ExternalSourceMerger::clone() const
{
	return std::make_unique<ExternalSourceMerger>(*this);
}

} // namespace grenade::vx::network::vertex::transformation
