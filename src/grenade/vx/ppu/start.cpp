#include "grenade/vx/ppu/extmem.h"
#include "grenade/vx/ppu/status.h"
#include "libnux/vx/v2/correlation.h"
#include "libnux/vx/v2/dls.h"
#include "libnux/vx/v2/mailbox.h"
#include "libnux/vx/v2/reset_neurons.h"
#include "libnux/vx/v2/time.h"
#include "libnux/vx/v2/vector.h"
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
// input: PPU location
volatile PPUOnDLS ppu;

void scheduling();

int start()
{
	mailbox_write_string("inside start\n");
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
	mailbox_write_string("after init\n");

	while (status != Status::stop) {
		switch (status) {
			case Status::reset_neurons: {
				mailbox_write_string("reset\n");
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
			case Status::periodic_read: {
				static_cast<void>(ppu);
				uint32_t const storage_base_scalar =
				    extmem_data_base + ((ppu == PPUOnDLS::bottom)
				                            ? cadc_recording_storage_base_bottom
				                            : cadc_recording_storage_base_top);
				uint32_t const storage_base_vector =
				    dls_extmem_base +
				    (((ppu == PPUOnDLS::bottom) ? cadc_recording_storage_base_bottom
				                                : cadc_recording_storage_base_top) >>
				     4);
				uint32_t offset = 0;
				uint32_t volatile* size_ptr = (uint32_t*) (storage_base_scalar + offset);
				uint32_t size = 0;
				*size_ptr = size;
				offset += 16;
				status = Status::inside_periodic_read;
				while (status != Status::stop_periodic_read) {
					if (offset > cadc_recording_storage_size - 256 /* samples */ - 16 /* time */ -
					                 16 /* size */) {
						continue;
					}
					uint64_t const time = now();
					uint64_t volatile* time_ptr = (uint64_t*) (storage_base_scalar + offset);
					*time_ptr = time;
					offset += 16;
					// clang-format off
					asm volatile(
					"fxvinx %[d0], %[ca_base], %[i]\n"
					"fxvinx %[d1], %[cab_base], %[j]\n"
					"fxvoutx %[d0], %[b0], %[i]\n"
					"fxvoutx %[d1], %[b1], %[i]\n"
					:
					  [d0] "=&qv" (read[0]),
					  [d1] "=&qv" (read[1])
					: [b0] "b" (storage_base_vector + (offset >> 4)),
					  [b1] "b" (storage_base_vector + ((offset + sizeof(read[0])) >> 4)),
					  [ca_base] "r" (dls_causal_base),
					  [cab_base] "r" (dls_causal_base|dls_buffer_enable_mask),
					  [i] "r" (uint32_t(0)),
					  [j] "r" (uint32_t(1))
					: /* no clobber */
					);
					// clang-format on
					offset += sizeof(read[0]) + sizeof(read[1]);
					size++;
					*size_ptr = size;
					asm volatile("sync");
				}
				asm volatile("fxvinx %[d0], %[b0], %[i]\n"
				             "sync\n"
				             : [d0] "=qv"(read[0])
				             : [b0] "b"(storage_base_vector), [i] "r"(uint32_t(0))
				             : "memory");
				break;
			}
			case Status::scheduler: {
				mailbox_write_string("bef\n");
				scheduling();
				status = Status::idle;
				break;
			}
			default: {
			}
		}
	}

	status = Status::idle;
	return 0;
}
