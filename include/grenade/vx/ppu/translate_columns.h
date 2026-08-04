#pragma once
#include "libnux/vx/vector_row.h"

namespace grenade::vx::ppu {

/**
 * Translate vector row values from columns of input handle
 * to columns of output handle.
 * Translates sequentially, if there are less active columns in input
 * than in output broadcasts last value, if there are more active
 * columns in input than in output drops the additional values.
 * @param input Input values
 * @param input_handle Input handle for active column lookup
 * @param output_handle Output handle for active column lookup
 */
template <typename T, typename HandleI, typename HandleO>
libnux::vx::VectorRow<T> translate_columns(
    libnux::vx::VectorRow<T> const& input,
    HandleI const& input_handle,
    HandleO const& output_handle)
{
	libnux::vx::VectorRow<T> ret;
	for (size_t i = 0; i < output_handle.columns.size(); ++i) {
		ret[output_handle.columns.at(i)] =
		    input[input_handle.columns.at(std::min(i + 1, input_handle.columns.size()) - 1)];
	}
	return ret;
}

} // namespace grenade::vx::ppu
