#pragma once
#include "hate/bitset.h"

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

	typedef std::array<uint8_t, 256> Row;

	/**
	 * Get weight values of specified row.
	 */
	Row get_weights(size_t index_row);
	/**
	 * Set weight values of specified row.
	 */
	void set_weights(Row const& value, size_t index_row);
};

} // namespace grenade::vx::ppu
