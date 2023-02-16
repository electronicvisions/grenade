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
	 * @param index_row Index of row to get weights for
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 * @returns Weight values of requested row without column masking
	 */
	Row get_weights(size_t index_row);

	/**
	 * Set weight values of specified row.
	 * Only the columns present in the handle are updated.
	 * @param value Weight values to set
	 * @param index_row Index of row to set weights for
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in rows.
	 */
	void set_weights(Row const& value, size_t index_row);
#endif
};

} // namespace grenade::vx::ppu

#include "grenade/vx/ppu/synapse_array_view_handle.tcc"
