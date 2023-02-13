#include "grenade/vx/ppu/synapse_array_view_handle.h"
#ifdef __ppu__
#include "libnux/vx/dls.h"
#include "libnux/vx/vector.h"

namespace grenade::vx::ppu {

inline SynapseArrayViewHandle::Row SynapseArrayViewHandle::get_weights(size_t index_row)
{
	using namespace libnux::vx;
	return get_row_via_vector(index_row, dls_weight_base);
}

inline void SynapseArrayViewHandle::set_weights(Row const& value, size_t index_row)
{
	if (!rows.test(index_row)) {
		return;
	}
	static_cast<void>(value);
	using namespace libnux::vx;
	set_row_via_vector_masked(value, column_mask, index_row, dls_weight_base);
}

} // namespace grenade::vx::ppu
#endif
