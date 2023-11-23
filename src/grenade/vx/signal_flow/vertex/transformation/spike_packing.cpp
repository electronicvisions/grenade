#include "grenade/vx/signal_flow/vertex/transformation/spike_packing.h"

#include "grenade/vx/signal_flow/event.h"
#include "hate/variant.h"
#include <algorithm>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

SpikePacking::~SpikePacking() {}

std::vector<Port> SpikePacking::inputs() const
{
	return {Port(1, ConnectionType::TimedSpikeToChipSequence)};
}

Port SpikePacking::output() const
{
	return Port(1, ConnectionType::TimedSpikeToChipSequence);
}

SpikePacking::Function::Value SpikePacking::apply(std::vector<Function::Value> const& value) const
{
	assert(value.size() == 1);
	size_t const batch_size = std::visit([](auto const& v) { return v.size(); }, value.at(0));

	std::vector<TimedSpikeToChipSequence> ret(batch_size);

	for (size_t b = 0; b < batch_size; ++b) {
		auto& local_ret = ret.at(b);
		auto const& local_value = value.at(0);

		auto const& local_input_events =
		    std::get<std::vector<TimedSpikeFromChipSequence>>(local_value).at(b);
		for (size_t i = 0; i < local_input_events.size(); ++i) {
			auto const& event_0 = local_input_events.at(i);
			if (i + 1 != local_input_events.size()) {
				auto const& event_1 = local_input_events.at(i + 1);
				if (event_0.time == event_1.time) {
					local_ret.push_back(TimedSpikeToChip(
					    event_0.time,
					    haldls::vx::v3::SpikePack2ToChip({event_0.data, event_1.data})));
					i++;
				} else {
					local_ret.push_back(TimedSpikeToChip(
					    event_0.time, haldls::vx::v3::SpikePack1ToChip({event_0.data})));
				}
			} else {
				local_ret.push_back(TimedSpikeToChip(
				    event_0.time, haldls::vx::v3::SpikePack1ToChip({event_0.data})));
			}
		}
	}
	return ret;
}

bool SpikePacking::equal(vertex::Transformation::Function const& other) const
{
	SpikePacking const* o = dynamic_cast<SpikePacking const*>(&other);
	if (o == nullptr) {
		return false;
	}
	return true;
}

} // namespace grenade::vx::signal_flow::vertex::transformation
