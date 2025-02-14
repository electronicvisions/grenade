#include "grenade/vx/execution/detail/ppu_program_generator.h"

#include "grenade/vx/ppu.h"
#include "hate/join.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"
#include <vector>
#include <inja/inja.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::execution::detail {

void PPUProgramGenerator::add(
    signal_flow::Graph::vertex_descriptor const descriptor,
    signal_flow::vertex::PlasticityRule const& rule,
    std::vector<
        std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>> const&
        synapses,
    std::vector<std::pair<halco::hicann_dls::vx::v3::NeuronRowOnDLS, ppu::NeuronViewHandle>> const&
        neurons,
    size_t realtime_column_index)
{
	m_plasticity_rules.push_back({descriptor, rule, synapses, neurons, realtime_column_index});
}

std::vector<std::string> PPUProgramGenerator::done()
{
	hate::Timer timer;
	log4cxx::LoggerPtr const logger =
	    log4cxx::Logger::getLogger("grenade.PPUProgramGenerator.done()");

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	size_t max_realtime_column_index = 0;
	for (auto const& [i, plasticity_rule, synapses, neurons, realtime_column_index] :
	     m_plasticity_rules) {
		max_realtime_column_index = std::max(max_realtime_column_index, realtime_column_index);
	}

	std::vector<std::string> sources;
	// plasticity rules
	{
		for (auto const& [i, plasticity_rule, synapses, neurons, realtime_column_index] :
		     m_plasticity_rules) {
			std::stringstream kernel;
			auto kernel_str = plasticity_rule.get_kernel();
			std::string const kernel_name("PLASTICITY_RULE_KERNEL");
			kernel_str.replace(
			    kernel_str.find(kernel_name), kernel_name.size(),
			    "plasticity_rule_kernel_" + std::to_string(i) + "_" +
			        std::to_string(realtime_column_index));

			// clang-format off
			std::string recording_source_template = R"grenadeTemplate(
{{recorded_memory_declaration}}

## if has_timed_recording
Recording recorded_scratchpad_memory_{{i}}_{{realtime_column_index}}[{{plasticity_rule.timer.num_periods}}]
    __attribute__((section("{{recording_placement}}.data.keep")));
## else if has_raw_recording
Recording recorded_scratchpad_memory_{{i}}_{{realtime_column_index}} __attribute__((section("{{recording_placement}}.data.keep")));
## endif
)grenadeTemplate";
			// clang-format on

			// clang-format off
			std::string handles_source_template = R"grenadeTemplate(
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/ppu/neuron_view_handle.h"
#include <array>

std::array<grenade::vx::ppu::SynapseArrayViewHandle, {{length(synapses)}}>
synapse_array_view_handle_{{i}}_{{realtime_column_index}} = {
## for synapse in synapses
	[](){
		grenade::vx::ppu::SynapseArrayViewHandle synapse_array_view_handle;
		synapse_array_view_handle.hemisphere = static_cast<libnux::vx::PPUOnDLS>({{synapse.hemisphere}});
		synapse_array_view_handle.column_mask = 0;
## for column in synapse.enabled_columns
		synapse_array_view_handle.columns.set({{column}});
		synapse_array_view_handle.column_mask[{{column}}] = 1;
## endfor
## for row in synapse.enabled_rows
		synapse_array_view_handle.rows.set({{row}});
## endfor
		return synapse_array_view_handle;
	}(){% if not loop.is_last %},{% endif %}
## endfor
};

std::array<grenade::vx::ppu::NeuronViewHandle, {{length(neurons)}}>
neuron_view_handle_{{i}}_{{realtime_column_index}} = {
## for neuron in neurons
	[](){
		grenade::vx::ppu::NeuronViewHandle neuron_view_handle;
		neuron_view_handle.hemisphere = static_cast<libnux::vx::PPUOnDLS>({{neuron.hemisphere}});
## for column in neuron.enabled_columns
		neuron_view_handle.columns.set({{column}});
## endfor
		return neuron_view_handle;
	}(){% if not loop.is_last %},{% endif %}
## endfor
};
)grenadeTemplate";
			// clang-format on

			// clang-format off
			std::string kernel_source_template = R"grenadeTemplate(
{{recorded_memory_declaration}}

{{kernel_str}}

extern std::array<grenade::vx::ppu::SynapseArrayViewHandle, {{length(synapses)}}>
synapse_array_view_handle_{{i}}_{{realtime_column_index}};

extern std::array<grenade::vx::ppu::NeuronViewHandle, {{length(neurons)}}>
neuron_view_handle_{{i}}_{{realtime_column_index}};

## if has_timed_recording
extern Recording recorded_scratchpad_memory_{{i}}_{{realtime_column_index}}[{{plasticity_rule.timer.num_periods}}];
## else if has_raw_recording
extern Recording recorded_scratchpad_memory_{{i}}_{{realtime_column_index}};
## endif

#include "libnux/vx/helper.h"
#include "libnux/vx/mailbox.h"
#include "libnux/scheduling/types.hpp"

void plasticity_rule_{{i}}_{{realtime_column_index}}()
{
	libnux::vx::mailbox_write_string("plasticity rule {{i}}_{{realtime_column_index}}: b: ");
	auto const b = get_time();
	libnux::vx::mailbox_write_int(b);

## if has_raw_recording or has_timed_recording
## if has_raw_recording
	plasticity_rule_kernel_{{i}}_{{realtime_column_index}}(
	    synapse_array_view_handle_{{i}}_{{realtime_column_index}},
	    neuron_view_handle_{{i}}_{{realtime_column_index}},
	    recorded_scratchpad_memory_{{i}}_{{realtime_column_index}});
## else if has_timed_recording
	static size_t recorded_scratchpad_memory_period = 0;

	plasticity_rule_kernel_{{i}}_{{realtime_column_index}}(
	    synapse_array_view_handle_{{i}}_{{realtime_column_index}},
	    neuron_view_handle_{{i}}_{{realtime_column_index}},
	    recorded_scratchpad_memory_{{i}}_{{realtime_column_index}}[recorded_scratchpad_memory_period]);

	// TODO: reset by routine called prior to realtime section for each batch entry
	// instead of modulo
	recorded_scratchpad_memory_period++;
	recorded_scratchpad_memory_period %= {{plasticity_rule.timer.num_periods}};
## endif

	libnux::vx::do_not_optimize_away(recorded_scratchpad_memory_{{i}}_{{realtime_column_index}});
## else
	plasticity_rule_kernel_{{i}}_{{realtime_column_index}}(
	    synapse_array_view_handle_{{i}}_{{realtime_column_index}},
	    neuron_view_handle_{{i}}_{{realtime_column_index}});
## endif

	libnux::vx::mailbox_write_string(" d: ");
	libnux::vx::mailbox_write_int(get_time() - b);
	libnux::vx::mailbox_write_string("\n");
})grenadeTemplate";
			// clang-format on

			// clang-format off
			std::string scheduling_source_template = R"grenadeTemplate(
extern void plasticity_rule_{{i}}_{{realtime_column_index}}();

#include "libnux/scheduling/Timer.hpp"

Timer timer_{{i}}_{{realtime_column_index}} = [](){
	Timer t(&plasticity_rule_{{i}}_{{realtime_column_index}});
	t.set_first_deadline({{plasticity_rule.timer.start}});
	t.set_num_periods({{plasticity_rule.timer.num_periods}});
	t.set_period({{plasticity_rule.timer.period}});
	return t;
}();)grenadeTemplate";
			// clang-format on

			inja::json parameters;
			parameters["i"] = i;

			parameters["max_realtime_column_index"] = max_realtime_column_index;

			parameters["realtime_column_index"] = realtime_column_index;

			parameters["kernel_str"] = kernel_str;
			parameters["recorded_memory_declaration"] =
			    plasticity_rule.get_recorded_memory_declaration();

			parameters["has_raw_recording"] =
			    plasticity_rule.get_recording() &&
			    std::holds_alternative<signal_flow::vertex::PlasticityRule::RawRecording>(
			        *plasticity_rule.get_recording());
			parameters["has_timed_recording"] =
			    plasticity_rule.get_recording() &&
			    std::holds_alternative<signal_flow::vertex::PlasticityRule::TimedRecording>(
			        *plasticity_rule.get_recording());

			std::string recording_placement;
			if (plasticity_rule.get_recording()) {
				recording_placement = std::visit(
				    [](auto const& recording) {
					    return recording.placement_in_dram ? std::string("ext_dram")
					                                       : std::string("ext");
				    },
				    *plasticity_rule.get_recording());
			}
			parameters["recording_placement"] = recording_placement;

			parameters["plasticity_rule"]["timer"]["start"] =
			    plasticity_rule.get_timer().start.value();
			parameters["plasticity_rule"]["timer"]["num_periods"] =
			    plasticity_rule.get_timer().num_periods;
			parameters["plasticity_rule"]["timer"]["period"] =
			    plasticity_rule.get_timer().period.value();

			parameters["synapses"] = inja::json::array();
			parameters["neurons"] = inja::json::array();
			for (auto const& [synram, synapse_array_view_handle] : synapses) {
				std::vector<size_t> enabled_columns;
				for (size_t column = 0; column < synapse_array_view_handle.columns.size; ++column) {
					if (synapse_array_view_handle.columns.test(column)) {
						enabled_columns.push_back(column);
					}
				}
				std::vector<size_t> enabled_rows;
				for (size_t row = 0; row < synapse_array_view_handle.rows.size; ++row) {
					if (synapse_array_view_handle.rows.test(row)) {
						enabled_rows.push_back(row);
					}
				}
				parameters["synapses"].push_back(
				    {{"hemisphere", synram.value()},
				     {"enabled_columns", enabled_columns},
				     {"enabled_rows", enabled_rows}});
			}

			for (auto const& [row, neuron_view_handle] : neurons) {
				std::vector<size_t> enabled_columns;
				for (size_t column = 0; column < neuron_view_handle.columns.size; ++column) {
					if (neuron_view_handle.columns.test(column)) {
						enabled_columns.push_back(column);
					}
				}
				parameters["neurons"].push_back(
				    {{"hemisphere", row.value()}, {"enabled_columns", enabled_columns}});
			}

			sources.push_back(inja::render(recording_source_template, parameters));
			sources.push_back(inja::render(handles_source_template, parameters));
			sources.push_back(inja::render(kernel_source_template, parameters));
			sources.push_back(inja::render(scheduling_source_template, parameters));
		}
	}
	// scheduler
	if (!m_plasticity_rules.empty()) {
		// clang-format off
		std::string source_template = R"grenadeTemplate(
#include "libnux/scheduling/SchedulerSignaller.hpp"
#include "libnux/scheduling/Scheduler.hpp"
#include "libnux/scheduling/Timer.hpp"
#include "libnux/vx/mailbox.h"
#include "libnux/vx/dls.h"
#include "libnux/vx/time.h"

extern volatile libnux::vx::PPUOnDLS ppu;

volatile uint32_t runtime;
volatile uint32_t scheduler_event_drop_count;
uint64_t time_origin = 0;

## for i in plasticity_rules_i
extern Timer timer_{{i.0}}_{{i.1}};
volatile uint32_t timer_{{i.0}}_{{i.1}}_event_drop_count;
## endfor

namespace {

Scheduler<32> scheduler;
auto timers = std::tie({% for i in plasticity_rules_i %}timer_{{i.0}}_{{i.1}}{% if not loop.is_last %}, {% endif %}{% endfor %});

} // namespace

void scheduling()
{
	static_cast<void>(ppu);
	static_cast<void>(runtime);

## if length(plasticity_rules_i) > 0
	auto current = get_time();
	time_origin = libnux::vx::now();
	SchedulerSignallerTimer timer(current, current + runtime);
## for i in plasticity_rules_i
	timer_{{i.0}}_{{i.1}}.set_first_deadline(current + timer_{{i.0}}_{{i.1}}.get_first_deadline());
## endfor
	libnux::vx::mailbox_write_string("time:");
	libnux::vx::mailbox_write_int(current);
	libnux::vx::mailbox_write_string("\n");

	scheduler.execute(timer, timers);

	scheduler_event_drop_count = scheduler.get_dropped_events_count();
## for i in plasticity_rules_i
	timer_{{i.0}}_{{i.1}}_event_drop_count = timer_{{i.0}}_{{i.1}}.get_missed_count();
## endfor
## endif
})grenadeTemplate";
		// clang-format on

		inja::json parameters;
		{
			std::vector<std::array<size_t, 2>> plasticity_rules_i;
			for (auto const& [i, _, __, ___, realtime_column_index] : m_plasticity_rules) {
				plasticity_rules_i.emplace_back(std::array<size_t, 2>{i, realtime_column_index});
			}
			parameters["plasticity_rules_i"] = plasticity_rules_i;

			parameters["max_realtime_column_index"] = max_realtime_column_index;
		}
		sources.push_back(inja::render(source_template, parameters));
	} else {
		sources.push_back("void scheduling() {}");
	}
	// periodic CADC readout
	if (has_periodic_cadc_readout || has_periodic_cadc_readout_on_dram) {
		// clang-format off
		std::string source_template = R"grenadeTemplate(
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/ppu/detail/uninitialized.h"
#include "libnux/vx/dls.h"
#include "libnux/vx/vector.h"
#include "libnux/vx/globaladdress.h"
#include "libnux/vx/time.h"
#include <array>
#include <tuple>

extern volatile libnux::vx::PPUOnDLS ppu;
extern volatile grenade::vx::ppu::detail::Status status;

typedef std::tuple<std::array<std::pair<uint64_t, libnux::vx::vector_row_t>, {{num_samples}}>, uint32_t> PeriodicCADCSamples;

grenade::vx::ppu::detail::uninitialized<PeriodicCADCSamples> periodic_cadc_samples_top __attribute__((section("{{recording_placement}}.data")));
grenade::vx::ppu::detail::uninitialized<PeriodicCADCSamples> periodic_cadc_samples_bot __attribute__((section("{{recording_placement}}.data")));

auto& local_periodic_cadc_samples = std::get<0>(grenade::vx::ppu::detail::uninitialized_cast<PeriodicCADCSamples>(ppu == libnux::vx::PPUOnDLS::bottom ?
    periodic_cadc_samples_bot : periodic_cadc_samples_top));

uint32_t periodic_cadc_readout_memory_offset = 0;

void perform_periodic_read_recording(size_t const offset, uint64_t time_origin) ATTRIB_LINK_TO_INTERNAL;

void perform_periodic_read_recording(size_t const offset, uint64_t time_origin)
{
	using namespace libnux::vx;

	if (offset >= local_periodic_cadc_samples.size()) {
		return;
	}

	// synchronize time-stamp and CADC readout
	asm volatile(
		"sync\n"
		:::
	);

	uint64_t const time = libnux::vx::now();

	// clang-format off
	asm volatile(
	"fxvinx 0, %[ca_base], %[i]\n"
	"fxvinx 1, %[cab_base], %[j]\n"
	"fxvoutx 0, %[b0], %[i]\n"
	"fxvoutx 1, %[b1], %[i]\n"
	::
	  [b0] "b" (GlobalAddress::from_global(0, reinterpret_cast<uint32_t>(&(local_periodic_cadc_samples[offset].second.even)) & 0x1fff'ffff).to_{{recording_global_address}}().to_fxviox_addr()),
	  [b1] "b" (GlobalAddress::from_global(0, reinterpret_cast<uint32_t>(&(local_periodic_cadc_samples[offset].second.odd)) & 0x1fff'ffff).to_{{recording_global_address}}().to_fxviox_addr()),
	  [ca_base] "r" (dls_causal_base),
	  [cab_base] "r" (dls_causal_base|dls_buffer_enable_mask),
	  [i] "r" (uint32_t(0)),
	  [j] "r" (uint32_t(1))
	: "qv0", "qv1"
	);
	// clang-format on

	local_periodic_cadc_samples[offset].first = time - time_origin;
}

void perform_periodic_read() ATTRIB_LINK_TO_INTERNAL;

void perform_periodic_read()
{
	using namespace libnux::vx;

	// instruction cache warming
	auto const time_before_cache_warming = libnux::vx::now();
	perform_periodic_read_recording(periodic_cadc_readout_memory_offset, time_before_cache_warming);
	while (libnux::vx::now() < time_before_cache_warming + {{periodic_cadc_ppu_wait_clock_cycles}}) {}
	while (status != grenade::vx::ppu::detail::Status::stop_periodic_read) {
		perform_periodic_read_recording(periodic_cadc_readout_memory_offset, time_before_cache_warming);
		periodic_cadc_readout_memory_offset++;
	}

	std::get<1>(grenade::vx::ppu::detail::uninitialized_cast<PeriodicCADCSamples>(ppu == libnux::vx::PPUOnDLS::bottom ? periodic_cadc_samples_bot : periodic_cadc_samples_top)) = periodic_cadc_readout_memory_offset;
	asm volatile(
	    "fxvinx 0, %[b0], %[i]\n"
	    "sync\n"
	    :: [b0] "r"(dls_{{recording_global_address}}_base), [i] "r"(uint32_t(0))
	    : "qv0", "memory"
	);
})grenadeTemplate";
		// clang-format on

		inja::json parameters;
		parameters["num_samples"] = num_periodic_cadc_samples;
		parameters["recording_placement"] =
		    has_periodic_cadc_readout_on_dram ? std::string("ext_dram") : std::string("ext");
		parameters["recording_global_address"] =
		    has_periodic_cadc_readout_on_dram ? std::string("extmem_dram") : std::string("extmem");
		parameters["periodic_cadc_ppu_wait_clock_cycles"] =
		    periodic_cadc_ppu_wait_clock_cycles.value();
		sources.push_back(inja::render(source_template, parameters));
	} else {
		sources.push_back("void perform_periodic_read() {}");
	}

	{
		std::ifstream fs(get_program_base_source());
		std::stringstream ss;
		ss << fs.rdbuf();
		sources.push_back(ss.str());
	}

	LOG4CXX_TRACE(
	    logger, "Generated PPU program sources:\n"
	                << hate::join(sources.begin(), sources.end(), "\n\n") << ".");
	LOG4CXX_TRACE(logger, "Generated PPU program sources in " << timer.print() << ".");
	return sources;
}

} // namespace grenade::vx::execution::detail
