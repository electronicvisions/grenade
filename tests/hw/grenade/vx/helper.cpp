#include "helper.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "hxcomm/common/hwdb_entry.h"
#include "lola/vx/v3/neuron.h"


using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace lola::vx::v3;
using namespace haldls::vx::v3;


lola::vx::v3::Chip get_chip_config_bypass_excitatory()
{
	Chip chip;
	for (auto const c : iter_all<CommonNeuronBackendConfigOnDLS>()) {
		chip.neuron_block.backends[c].set_enable_clocks(true);
	}
	for (auto const s : iter_all<SynapseOnSynapseRow>()) {
		for (auto& current_row : chip.neuron_block.current_rows) {
			auto& sw = current_row.values[s];
			sw.set_enable_synaptic_current_excitatory(true);
			sw.set_enable_synaptic_current_inhibitory(true);
		}
		for (auto const neuron : iter_all<AtomicNeuronOnDLS>()) {
			auto& config = chip.neuron_block.atomic_neurons[neuron];
			config.event_routing.enable_digital = true;
			config.event_routing.analog_output =
			    AtomicNeuron::EventRouting::AnalogOutputMode::normal;
			config.event_routing.enable_bypass_excitatory = true;
			config.threshold.enable = false;
		}
		for (auto const block : iter_all<CommonPADIBusConfigOnDLS>()) {
			auto& padi_config =
			    chip.synapse_driver_blocks[block.toSynapseDriverBlockOnDLS()].padi_bus;
			auto dacen = padi_config.get_dacen_pulse_extension();
			for (auto const block : iter_all<PADIBusOnPADIBusBlock>()) {
				dacen[block] = CommonPADIBusConfig::DacenPulseExtension(15);
			}
			padi_config.set_dacen_pulse_extension(dacen);
		}
		for (auto const drv : iter_all<SynapseDriverOnDLS>()) {
			auto& config = chip.synapse_driver_blocks[drv.toSynapseDriverBlockOnDLS()]
			                   .synapse_drivers[drv.toSynapseDriverOnSynapseDriverBlock()];
			config.set_enable_receiver(true);
			config.set_enable_address_out(true);
		}
		for (auto const block : iter_all<SynapseBlockOnDLS>()) {
			chip.synapse_blocks[block].i_bias_dac.fill(CapMemCell::Value(1022));
		}
	}
	return chip;
}

bool is_jboa_setup_of_size(grenade::vx::execution::JITGraphExecutor const& executor, size_t size)
{
	auto hwdb_entries = executor.get_hwdb_entry();
	bool all_jboa =
	    std::all_of(hwdb_entries.begin(), hwdb_entries.end(), [](auto const& key_value) {
		    for (auto const& hwdb_entry : key_value.second) {
			    if (!std::holds_alternative<hwdb4cpp::JboaSetupEntry>(hwdb_entry)) {
				    return false;
			    }
		    }
		    return true;
	    });
	if (size != 0) {
		for (auto const& [_, connection_size] : executor.connection_sizes()) {
			if (connection_size != size) {
				return false;
			}
		}
	}
	return all_jboa;
}