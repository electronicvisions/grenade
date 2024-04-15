#include "grenade/vx/ppu/synapse_array_view_handle.h"

#include "halco/hicann-dls/vx/v3/synapse.h"
#include "libnux/vx/correlation.h"
#include "libnux/vx/dls.h"
#include "libnux/vx/vector.h"

extern volatile libnux::vx::PPUOnDLS ppu;

namespace grenade::vx::ppu {

SynapseArrayViewHandle::Row SynapseArrayViewHandle::get_weights(size_t index_row) const
{
	if (::ppu != hemisphere || !rows.test(index_row)) {
		exit(1);
	}
	using namespace libnux::vx;
	return get_row_via_vector(index_row, dls_weight_base);
}

void SynapseArrayViewHandle::set_weights(Row const& value, size_t index_row) const
{
	if (::ppu != hemisphere || !rows.test(index_row)) {
		exit(1);
	}
	static_cast<void>(value);
	using namespace libnux::vx;
	set_row_via_vector_masked(value, column_mask, index_row, dls_weight_base);
}

SynapseArrayViewHandle::Row SynapseArrayViewHandle::get_causal_correlation(size_t index_row) const
{
	if (::ppu != hemisphere || !rows.test(index_row)) {
		exit(1);
	}
	// set switch row to synapse CADC readout on causal channel
	libnux::vx::set_row_via_vector(
	    libnux::vx::VectorRowMod8(0b00100000), halco::hicann_dls::vx::v3::SynapseRowOnSynram::size,
	    libnux::vx::dls_weight_base);

	Row ret;
	libnux::vx::get_causal_correlation(&ret.even.data, &ret.odd.data, index_row);
	return ret;
}

SynapseArrayViewHandle::Row SynapseArrayViewHandle::get_acausal_correlation(size_t index_row) const
{
	if (::ppu != hemisphere || !rows.test(index_row)) {
		exit(1);
	}
	// set switch row to synapse CADC readout on acausal channel
	libnux::vx::set_row_via_vector(
	    libnux::vx::VectorRowMod8(0b00100000), halco::hicann_dls::vx::v3::SynapseRowOnSynram::size,
	    libnux::vx::dls_decoder_base);

	Row ret;
	libnux::vx::get_acausal_correlation(&ret.even.data, &ret.odd.data, index_row);
	return ret;
}

SynapseArrayViewHandle::CorrelationRow SynapseArrayViewHandle::get_correlation(
    size_t index_row, bool reset) const
{
	if (::ppu != hemisphere || !rows.test(index_row)) {
		exit(1);
	}
	// set switch row to synapse CADC readout on causal and acausal channel
	libnux::vx::set_row_via_vector(
	    libnux::vx::VectorRowMod8(0b00100000), halco::hicann_dls::vx::v3::SynapseRowOnSynram::size,
	    libnux::vx::dls_weight_base);
	libnux::vx::set_row_via_vector(
	    libnux::vx::VectorRowMod8(0b00100000), halco::hicann_dls::vx::v3::SynapseRowOnSynram::size,
	    libnux::vx::dls_decoder_base);

	CorrelationRow ret;
	libnux::vx::get_correlation(
	    &ret.causal.even.data, &ret.causal.odd.data, &ret.acausal.even.data, &ret.acausal.odd.data,
	    index_row);

	if (reset) {
		Row const reset(libnux::vx::dls_correlation_reset);
		libnux::vx::set_row_via_vector_masked(
		    reset, column_mask, index_row, libnux::vx::dls_causal_base);
	}

	return ret;
}

void SynapseArrayViewHandle::reset_correlation(size_t index_row) const
{
	if (::ppu != hemisphere || !rows.test(index_row)) {
		exit(1);
	}
	Row const reset(libnux::vx::dls_correlation_reset);
	libnux::vx::set_row_via_vector_masked(
	    reset, column_mask, index_row, libnux::vx::dls_causal_base);
}

} // namespace grenade::vx::ppu
