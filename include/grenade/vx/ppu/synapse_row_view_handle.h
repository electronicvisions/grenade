#pragma once
#include "grenade/vx/ppu/synapse_array_view_handle.h"

namespace grenade::vx::ppu {

/**
 * Handle to synapse row.
 */
struct SynapseRowViewHandle
{
	SynapseRowViewHandle(SynapseArrayViewHandle const& synapses, size_t row) :
	    m_synapses(synapses), m_row(row)
	{}

#ifdef __ppu__
	/**
	 * Signed row values.
	 */
	typedef libnux::vx::VectorRowFracSat8 SignedRow;

	/**
	 * Unsigned row values.
	 */
	typedef libnux::vx::VectorRowMod8 UnsignedRow;


	/**
	 * Get weight values.
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 * @returns Weight values without column masking
	 */
	template <typename Row = SignedRow>
	Row get_weights() const;

	/**
	 * Set weight values.
	 * Only the columns present in the handle are updated.
	 * @param value Weight values to set
	 * @param saturate Whether to saturate given values to the weight range [0, 64)
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 */
	template <typename Row = SignedRow>
	void set_weights(Row const& value, bool saturate = true) const;

	/**
	 * Get causal correlation of specified row.
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 * @returns Correlation values of requested row without column masking
	 */
	UnsignedRow get_causal_correlation() const;

	/**
	 * Get acausal correlation of specified row.
	 * @param source Source synapses (exc/inh) to get values from
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 * @returns Correlation values of requested row without column masking
	 */
	UnsignedRow get_acausal_correlation() const;

	/**
	 * Correlation values of a synapse row.
	 */
	typedef SynapseArrayViewHandle::CorrelationRow CorrelationRow;

	/**
	 * Get both causal and acausal correlation of specified row and optionally reset correlation.
	 * @param reset Whether to reset the correlation measurement of the columns present in the
	 * handle after the read-out operation
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 * @returns Correlation values of requested row without column masking
	 */
	CorrelationRow get_correlation(bool reset) const;

	/**
	 * Reset correlation measurement of specified row.
	 * Only the measurements for the columns present in the handle are reset.
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 */
	void reset_correlation() const;
#endif

private:
	SynapseArrayViewHandle const& m_synapses;
	size_t m_row;
};


/**
 * Handle to signed synapse row (consisting of an excitatory and an inhibitory row).
 */
struct SynapseRowViewHandleSigned
{
	SynapseRowViewHandleSigned(SynapseRowViewHandle const& exc, SynapseRowViewHandle const& inh) :
	    m_exc(exc), m_inh(inh)
	{}

#ifdef __ppu__
	/**
	 * Signed row values.
	 */
	typedef libnux::vx::VectorRowFracSat8 SignedRow;

	/**
	 * Unsigned row values.
	 */
	typedef libnux::vx::VectorRowMod8 UnsignedRow;


	/**
	 * Get weight values.
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 * @returns Weight values without column masking
	 */
	SignedRow get_weights() const;

	/**
	 * Set weight values.
	 * Only the columns present in the handle are updated.
	 * @param value Weight values to set
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 */
	void set_weights(SignedRow const& value) const;

	enum class CorrelationSource : uint32_t
	{
		exc,
		inh
	};

	/**
	 * Get causal correlation of specified row.
	 * @param source Source synapses (exc/inh) to get values from (defaults arbitrarily to exc)
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 * @returns Correlation values of requested row without column masking
	 */
	UnsignedRow get_causal_correlation(CorrelationSource source = CorrelationSource::exc) const;

	/**
	 * Get acausal correlation of specified row.
	 * @param source Source synapses (exc/inh) to get values from (defaults arbitrarily to exc)
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 * @returns Correlation values of requested row without column masking
	 */
	UnsignedRow get_acausal_correlation(CorrelationSource source = CorrelationSource::exc) const;

	/**
	 * Correlation values of a synapse row.
	 */
	typedef SynapseRowViewHandle::CorrelationRow CorrelationRow;

	/**
	 * Get both causal and acausal correlation of specified row and optionally reset correlation.
	 * @param reset Whether to reset the correlation measurement of the columns present in the
	 * handle after the read-out operation
	 * @param source Source synapses (exc/inh) to get values from (defaults arbitrarily to exc)
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 * @returns Correlation values of requested row without column masking
	 */
	CorrelationRow get_correlation(
	    bool reset, CorrelationSource source = CorrelationSource::exc) const;

	/**
	 * Reset correlation measurement of specified row.
	 * Only the measurements for the columns present in the handle are reset.
	 * @param source Source synapses (exc/inh) to reset values for (defaults arbitrarily to exc)
	 * @throws Exit with exit code 1 on access from wrong PPU compared to hemisphere or wrong row
	 * not present in the handle.
	 */
	void reset_correlation(CorrelationSource source = CorrelationSource::exc) const;
#endif

private:
	SynapseRowViewHandle m_exc;
	SynapseRowViewHandle m_inh;
};

} // namespace grenade::vx::ppu

#include "grenade/vx/ppu/synapse_row_view_handle.tcc"
