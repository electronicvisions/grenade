#include "grenade/vx/signal_flow/vertex/transformation/spike_packing.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "hate/variant.h"
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

std::vector<Transformation::Function::Port> SpikePacking::get_input_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::TimedSpikeToChipSequence),
	    grenade::common::CuboidMultiIndexSequence({1}))};
}

std::vector<Transformation::Function::Port> SpikePacking::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::TimedSpikeToChipSequence),
	    grenade::common::CuboidMultiIndexSequence({1}))};
}

std::vector<Transformation::Results> SpikePacking::apply(
    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data) const
{
	assert(data.size() == 1);
	size_t const batch_size = std::visit(
	    [](auto const& v) { return v.size(); },
	    dynamic_cast<Transformation::Dynamics const&>(data.at(0).get()).value);

	std::vector<TimedSpikeToChipSequence> ret(batch_size);

	for (size_t b = 0; b < batch_size; ++b) {
		auto& local_ret = ret.at(b);
		auto const& local_value = dynamic_cast<Transformation::Dynamics const&>(data.at(0).get());

		auto const& local_input_events =
		    std::get<std::vector<TimedSpikeFromChipSequence>>(local_value.value).at(b);
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
	return {{ret}};
}

bool SpikePacking::is_equal_to(Transformation::Function const&) const
{
	return true;
}

std::unique_ptr<Transformation::Function> SpikePacking::copy() const
{
	return std::make_unique<SpikePacking>(*this);
}

std::unique_ptr<Transformation::Function> SpikePacking::move()
{
	return std::make_unique<SpikePacking>(std::move(*this));
}

std::ostream& SpikePacking::print(std::ostream& os) const
{
	return os << "SpikePacking()";
}

} // namespace grenade::vx::signal_flow::vertex::transformation
