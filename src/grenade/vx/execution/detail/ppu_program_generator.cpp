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
        neurons)
{
	m_plasticity_rules.push_back({descriptor, rule, synapses, neurons});
}

std::vector<std::string> PPUProgramGenerator::done()
{
	hate::Timer timer;
	log4cxx::LoggerPtr const logger =
	    log4cxx::Logger::getLogger("grenade.PPUProgramGenerator.done()");

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	std::vector<std::string> sources;
	// plasticity rules
	{
		for (auto const& [i, plasticity_rule, synapses, neurons] : m_plasticity_rules) {
			std::stringstream kernel;
			auto kernel_str = plasticity_rule.get_kernel();
			std::string const kernel_name("PLASTICITY_RULE_KERNEL");
			kernel_str.replace(
			    kernel_str.find(kernel_name), kernel_name.size(),
			    "plasticity_rule_kernel_" + std::to_string(i));

			// clang-format off
			std::string source_template = R"grenadeTemplate(
{{recorded_memory_definition}}

## if has_timed_recording
Recording recorded_scratchpad_memory_top_{{i}}[{{plasticity_rule.timer.num_periods}}]
    __attribute__((section("ext.data.keep")));
Recording recorded_scratchpad_memory_bot_{{i}}[{{plasticity_rule.timer.num_periods}}]
    __attribute__((section("ext.data.keep")));
## else if has_raw_recording
Recording recorded_scratchpad_memory_top_{{i}} __attribute__((section("ext.data.keep")));
Recording recorded_scratchpad_memory_bot_{{i}} __attribute__((section("ext.data.keep")));
## endif

{{kernel_str}}

namespace {

std::array<grenade::vx::ppu::SynapseArrayViewHandle, {{length(synapses)}}>
synapse_array_view_handle = {
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
neuron_view_handle = {
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

} // namespace

#include "libnux/vx/helper.h"
#include "libnux/vx/mailbox.h"
#include "libnux/scheduling/types.hpp"

extern volatile libnux::vx::PPUOnDLS ppu;

void plasticity_rule_{{i}}()
{
	libnux::vx::mailbox_write_string("plasticity rule {{i}}: b: ");
	auto const b = get_time();
	libnux::vx::mailbox_write_int(b);

## if has_raw_recording or has_timed_recording
## if has_raw_recording
	plasticity_rule_kernel_{{i}}(
	    synapse_array_view_handle, neuron_view_handle, ppu == libnux::vx::PPUOnDLS::top ?
	    recorded_scratchpad_memory_top_{{i}} : recorded_scratchpad_memory_bot_{{i}});
## else if has_timed_recording
	static size_t recorded_scratchpad_memory_period = 0;

	plasticity_rule_kernel_{{i}}(
	    synapse_array_view_handle, neuron_view_handle,
	    (ppu == libnux::vx::PPUOnDLS::top ? recorded_scratchpad_memory_top_{{i}} :
	    recorded_scratchpad_memory_bot_{{i}})[recorded_scratchpad_memory_period]);

	// TODO: reset by routine called prior to realtime section for each batch entry
	// instead of modulo
	recorded_scratchpad_memory_period++;
	recorded_scratchpad_memory_period %= {{plasticity_rule.timer.num_periods}};
## endif

	libnux::vx::do_not_optimize_away(recorded_scratchpad_memory_top_{{i}});
	libnux::vx::do_not_optimize_away(recorded_scratchpad_memory_bot_{{i}});
## else
	plasticity_rule_kernel_{{i}}(synapse_array_view_handle, neuron_view_handle);
## endif

	libnux::vx::mailbox_write_string(" d: ");
	libnux::vx::mailbox_write_int(get_time() - b);
	libnux::vx::mailbox_write_string("\n");
}

#include "libnux/scheduling/Service.hpp"

Service_Function<{{i}}, &plasticity_rule_{{i}}> service_{{i}};

#include "libnux/scheduling/Timer.hpp"

Timer timer_{{i}} = [](){
	Timer t;
	t.set_first_deadline({{plasticity_rule.timer.start}});
	t.set_num_periods({{plasticity_rule.timer.num_periods}});
	t.set_period({{plasticity_rule.timer.period}});
	t.set_service(service_{{i}});
	return t;
}();)grenadeTemplate";
			// clang-format on

			inja::json parameters;
			parameters["i"] = i;

			parameters["kernel_str"] = kernel_str;
			parameters["recorded_memory_definition"] =
			    plasticity_rule.get_recorded_memory_definition();

			parameters["has_raw_recording"] =
			    plasticity_rule.get_recording() &&
			    std::holds_alternative<signal_flow::vertex::PlasticityRule::RawRecording>(
			        *plasticity_rule.get_recording());
			parameters["has_timed_recording"] =
			    plasticity_rule.get_recording() &&
			    std::holds_alternative<signal_flow::vertex::PlasticityRule::TimedRecording>(
			        *plasticity_rule.get_recording());

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

			sources.push_back(inja::render(source_template, parameters));
			sources.push_back(kernel.str());
		}
	}
	// scheduler
	{
		// clang-format off
		std::string source_template = R"grenadeTemplate(
#include "libnux/scheduling/SchedulerSignaller.hpp"
#include "libnux/scheduling/Scheduler.hpp"
#include "libnux/scheduling/Timer.hpp"
#include "libnux/scheduling/Service.hpp"
#include "libnux/vx/mailbox.h"
#include "libnux/vx/dls.h"
#include "libnux/vx/time.h"

extern volatile libnux::vx::PPUOnDLS ppu;

volatile uint32_t runtime;
volatile uint32_t scheduler_event_drop_count;
uint64_t time_origin = 0;

## for i in plasticity_rules_i
extern Timer timer_{{i}};
volatile uint32_t timer_{{i}}_event_drop_count;
extern void plasticity_rule_{{i}}();
extern Service_Function<{{i}}, &plasticity_rule_{{i}}> service_{{i}};
## endfor

namespace {

Scheduler<32> scheduler;
auto timers = std::tie({% for i in plasticity_rules_i %}timer_{{i}}{% if not loop.is_last %}, {% endif %}{% endfor %});
auto services = std::tie({% for i in plasticity_rules_i %}service_{{i}}{% if not loop.is_last %}, {% endif %}{% endfor %});

} // namespace

void scheduling()
{
	static_cast<void>(ppu);
	static_cast<void>(runtime);

## if length(plasticity_rules_i) > 0
	auto current = get_time();
	SchedulerSignallerTimer timer(current, current + runtime);
## for i in plasticity_rules_i
	timer_{{i}}.set_first_deadline(current + timer_{{i}}.get_first_deadline());
## endfor
	libnux::vx::mailbox_write_string("time:");
	libnux::vx::mailbox_write_int(current);
	libnux::vx::mailbox_write_string("\n");

	scheduler.execute(timer, services, timers);

	scheduler_event_drop_count = scheduler.get_dropped_events_count();
## for i in plasticity_rules_i
	timer_{{i}}_event_drop_count = timer_{{i}}.get_missed_count();
## endfor
## endif
})grenadeTemplate";
		// clang-format on

		inja::json parameters;
		{
			std::vector<size_t> plasticity_rules_i;
			for (auto const& [i, _, __, ___] : m_plasticity_rules) {
				plasticity_rules_i.push_back(i);
			}
			parameters["plasticity_rules_i"] = plasticity_rules_i;
		}
		sources.push_back(inja::render(source_template, parameters));
	}
	// periodic CADC readout
	{
		// clang-format off
		std::string source_template = R"grenadeTemplate(
#include "grenade/vx/ppu/detail/status.h"
#include "libnux/vx/dls.h"
#include "libnux/vx/vector.h"
#include "libnux/vx/globaladdress.h"
#include "libnux/vx/time.h"
#include <array>
#include <tuple>

extern volatile libnux::vx::PPUOnDLS ppu;
extern volatile grenade::vx::ppu::detail::Status status;
std::tuple<std::array<std::pair<uint64_t, libnux::vx::vector_row_t>, {{num_samples}}>, uint32_t>
    periodic_cadc_samples_top __attribute__((section("ext.data")));
std::tuple<std::array<std::pair<uint64_t, libnux::vx::vector_row_t>, {{num_samples}}>, uint32_t>
    periodic_cadc_samples_bot __attribute__((section("ext.data")));

auto& local_periodic_cadc_samples = std::get<0>(ppu == libnux::vx::PPUOnDLS::bottom ?
    periodic_cadc_samples_bot : periodic_cadc_samples_top);

void perform_periodic_read_recording(size_t const offset)
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
	  [b0] "b" (GlobalAddress::from_global(0, reinterpret_cast<uint32_t>(&(local_periodic_cadc_samples[offset].second.even)) & 0x3fff'ffff).to_extmem().to_fxviox_addr()),
	  [b1] "b" (GlobalAddress::from_global(0, reinterpret_cast<uint32_t>(&(local_periodic_cadc_samples[offset].second.odd)) & 0x3fff'ffff).to_extmem().to_fxviox_addr()),
	  [ca_base] "r" (dls_causal_base),
	  [cab_base] "r" (dls_causal_base|dls_buffer_enable_mask),
	  [i] "r" (uint32_t(0)),
	  [j] "r" (uint32_t(1))
	: "qv0", "qv1"
	);
	// clang-format on

	local_periodic_cadc_samples[offset].first = time;
}

void perform_periodic_read()
{
	using namespace libnux::vx;

	uint32_t offset = 0;
	uint32_t size = 0;

	// instruction cache warming
	perform_periodic_read_recording(offset);
	status = grenade::vx::ppu::detail::Status::inside_periodic_read;
	while (status != grenade::vx::ppu::detail::Status::stop_periodic_read) {
		perform_periodic_read_recording(offset);
		offset++;
		size++;
	}

	std::get<1>(ppu == libnux::vx::PPUOnDLS::bottom ? periodic_cadc_samples_bot : periodic_cadc_samples_top) = size;
	asm volatile(
	    "fxvinx 0, %[b0], %[i]\n"
	    "sync\n"
	    :: [b0] "r"(dls_extmem_base), [i] "r"(uint32_t(0))
	    : "qv0", "memory"
	);
})grenadeTemplate";
		// clang-format on

		inja::json parameters;
		size_t const num_samples = has_periodic_cadc_readout ? num_cadc_samples_in_extmem : 0;
		parameters["num_samples"] = num_samples;
		sources.push_back(inja::render(source_template, parameters));
	}

	LOG4CXX_TRACE(
	    logger, "Generated PPU program sources:\n"
	                << hate::join_string(sources.begin(), sources.end(), "\n\n") << ".");
	LOG4CXX_TRACE(logger, "Generated PPU program sources in " << timer.print() << ".");
	return sources;
}

} // namespace grenade::vx::execution::detail
