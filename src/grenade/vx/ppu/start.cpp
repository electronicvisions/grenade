#include "grenade/vx/ppu/status.h"
#include "libnux/vx/v2/correlation.h"
#include "libnux/vx/v2/dls.h"
#include "libnux/vx/v2/reset_neurons.h"
#include "libnux/vx/v2/time.h"
#include "libnux/vx/v2/vector_convert.h"
#include "libnux/vx/v2/vector_math.h"
#include <s2pp.h>

using namespace grenade::vx::ppu;
using namespace libnux::vx::v2;

// output: cadc measurement
volatile __vector int8_t cadc_result[dls_num_vectors_per_row];
// input: mask-selecting neurons to be reset
volatile __vector uint8_t neuron_reset_mask[dls_num_vectors_per_row];
// input and output: state in state machine
volatile Status status = Status::idle;

int start()
{
	__vector int8_t baseline_read[dls_num_vectors_per_row];
	for (size_t i = 0; i < dls_num_vectors_per_row; ++i) {
		// set so that when baseline reads are never used, the value subtracted is correct
		baseline_read[i] = vec_splat_s8(0);
	}

	__vector uint8_t read[dls_num_vectors_per_row];
	for (size_t i = 0; i < dls_num_vectors_per_row; ++i) {
		// set arbitrarily to mitigate use without initialize warnings, {} wants to invoke memset
		read[i] = vec_splat_u8(0);
	}

	while (status != Status::stop) {
		switch (status) {
			case Status::reset_neurons: {
				reset_neurons(neuron_reset_mask[0], neuron_reset_mask[1]);
				status = Status::idle;
				break;
			}
			case Status::baseline_read: {
				__vector uint8_t baseline_read_u[dls_num_vectors_per_row];
				reset_neurons(neuron_reset_mask[0], neuron_reset_mask[1]);
				sleep_cycles(default_ppu_cycles_per_us * 5 /* us */);
				get_causal_correlation(&(baseline_read_u[0]), &(baseline_read_u[1]), 0);
				for (size_t i = 0; i < dls_num_vectors_per_row; ++i) {
					baseline_read[i] = uint8_to_int8(baseline_read_u[i]);
				}
				status = Status::idle;
				break;
			}
			case Status::read: {
				get_causal_correlation(&read[0], &read[1], 0);
				for (size_t i = 0; i < dls_num_vectors_per_row; ++i) {
					cadc_result[i] = saturating_subtract(uint8_to_int8(read[i]), baseline_read[i]);
				}
				status = Status::idle;
				break;
			}
			default: {
			}
		}
	}

	return 0;
}
