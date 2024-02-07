#include "grenade/vx/signal_flow/pack_spikes.h"
#include "grenade/vx/signal_flow/vertex/transformation/spike_packing.h"

namespace grenade::vx::signal_flow {

void pack_spikes(Data& data)
{
	vertex::transformation::SpikePacking transformation;
	for (auto& [_, d] : data.data) {
		if (std::holds_alternative<std::vector<TimedSpikeToChipSequence>>(d)) {
			d = transformation.apply({d});
		}
	}
}

} // namespace grenade::vx::signal_flow
