#include "grenade/vx/ppu/neuron_view_handle.h"

#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "haldls/vx/v3/neuron.h"
#include "libnux/vx/correlation.h"
#include "stadls/vx/v3/ppu/read.h"
#include "stadls/vx/v3/ppu/write.h"

extern volatile libnux::vx::PPUOnDLS ppu;

namespace grenade::vx::ppu {

namespace detail {

// template here instead of runtime argument in order to only decide once for the row-wise function
// instead of branching for each neuron although the setting is homogeneous.
template <bool Reset>
uint8_t get_rate_counter(NeuronViewHandle const& handle, size_t column_index)
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;
	using namespace stadls::vx::v3::ppu;
	if (column_index >= handle.columns.size()) {
		exit(1);
	}
	NeuronColumnOnDLS const neuron_col(handle.columns[column_index]);
	auto const neuron_row = NeuronRowOnDLS(static_cast<uintmax_t>(handle.hemisphere));
	AtomicNeuronOnDLS const neuron = AtomicNeuronOnDLS(neuron_col, neuron_row);

	if constexpr (Reset) {
		SpikeCounterReadOnDLS const spike_counter_read_coord = neuron.toSpikeCounterReadOnDLS();
		SpikeCounterResetOnDLS const spike_counter_reset_coord = neuron.toSpikeCounterResetOnDLS();
		auto const spike_counter_read = read<SpikeCounterRead>(spike_counter_read_coord);
		uint8_t ret = spike_counter_read.get_overflow() ? SpikeCounterRead::Count::max
		                                                : spike_counter_read.get_count().value();
		write(spike_counter_reset_coord, SpikeCounterReset());
		return ret;
	} else {
		SpikeCounterReadOnDLS const spike_counter_read_coord = neuron.toSpikeCounterReadOnDLS();
		auto const spike_counter_read = read<SpikeCounterRead>(spike_counter_read_coord);
		return spike_counter_read.get_overflow() ? SpikeCounterRead::Count::max
		                                         : spike_counter_read.get_count().value();
	}
}

} // namespace detail

uint8_t NeuronViewHandle::get_rate_counter(size_t column_index, bool reset) const
{
	if (reset) {
		return detail::get_rate_counter<true>(*this, column_index);
	}
	return detail::get_rate_counter<false>(*this, column_index);
}

namespace detail {

template <bool Reset>
NeuronViewHandle::Row get_rate_counters(NeuronViewHandle const& handle)
{
	NeuronViewHandle::Row ret;
	for (size_t i = 0; i < handle.columns.size(); ++i) {
		ret[handle.columns[i]] = detail::get_rate_counter<Reset>(handle, i);
	}
	return ret;
}

} // namespace detail

NeuronViewHandle::Row NeuronViewHandle::get_rate_counters(bool reset) const
{
	if (reset) {
		return detail::get_rate_counters<true>(*this);
	}
	return detail::get_rate_counters<false>(*this);
}

NeuronViewHandle::Row NeuronViewHandle::get_analog_readouts() const
{
	if (::ppu != hemisphere) {
		exit(1);
	}
	// set switch row to neuron CADC readout on causal channel
	libnux::vx::set_row_via_vector(
	    libnux::vx::VectorRowMod8(0b00100000), halco::hicann_dls::vx::v3::SynapseRowOnSynram::size,
	    libnux::vx::dls_weight_base);

	Row ret;
	libnux::vx::get_causal_correlation(&ret.even.data, &ret.odd.data, 0);
	return ret;
}

} // namespace grenade::vx::ppu
