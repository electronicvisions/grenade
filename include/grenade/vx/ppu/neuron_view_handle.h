#pragma once
#include "hate/bitset.h"
#ifdef __ppu__
#include "libnux/vx/dls.h"
#include "libnux/vx/vector.h"
#endif


namespace grenade::vx::ppu {

struct NeuronViewHandle
{
	/**
	 * Columns in neuron view.
	 * TODO: replace numbers by halco constants
	 */
	hate::bitset<256, uint32_t> columns;

#ifdef __ppu__
	/**
	 * Hemisphere location information.
	 */
	libnux::vx::PPUOnDLS hemisphere;

	/**
	 * Row values.
	 */
	typedef libnux::vx::vector_row_t Row;

	/**
	 * Get single neuron rate counter value.
	 * If column is not part of view exit(1).
	 * @param column Neuron column for which to get value
	 * @param reset Whether to reset the counters directly after read-out
	 */
	uint8_t get_rate_counter(size_t column, bool reset);

	/**
	 * Get neuron rate counter values.
	 * @param reset Whether to reset the counters directly after read-out
	 */
	Row get_rate_counters(bool reset);

	/**
	 * Get analog readouts via CADC.
	 * The source of the readout (membrane, adaptation, {exc,inh}_synin) depends on the per-neuron
	 * settings. Alters switch row such that neuron state can be recorded.
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere
	 * @returns Analog readout values without column masking
	 */
	Row get_analog_readouts();
#endif
};

} // namespace grenade::vx::ppu

#include "grenade/vx/ppu/neuron_view_handle.tcc"
