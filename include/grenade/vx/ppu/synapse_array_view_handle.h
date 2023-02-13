#pragma once
#include "hate/bitset.h"
#ifdef __ppu__
#include "libnux/vx/vector.h"
#endif

namespace grenade::vx::ppu {

struct SynapseArrayViewHandle
{
	/**
	 * Columns in synapse array of view.
	 * TODO: replace numbers by halco constants
	 */
	hate::bitset<256, uint32_t> columns;
	/**
	 * Rows in synapse array of view.
	 */
	hate::bitset<256, uint32_t> rows;

#ifdef __ppu__
	/**
	 * Hemisphere location information.
	 */
	libnux::vx::PPUOnDLS hemisphere;

	/**
	 * Row values with even values in [0,128), odd values in [128,256).
	 */
	typedef libnux::vx::vector_row_t Row;

	Row column_mask;

	/**
	 * Get weight values of specified row.
	 */
	Row get_weights(size_t index_row);

	/**
	 * Set weight values of specified row.
	 */
	void set_weights(Row const& value, size_t index_row);
#endif
};

} // namespace grenade::vx::ppu

#include "grenade/vx/ppu/synapse_array_view_handle.tcc"
