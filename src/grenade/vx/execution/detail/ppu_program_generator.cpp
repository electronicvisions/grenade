#include "grenade/vx/execution/detail/ppu_program_generator.h"

#include "grenade/vx/ppu.h"
#include "grenade/vx/signal_flow/vertex.h"
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

namespace {

/**
 * Merge periodic timer which have the same period and are adjacent to each other.
 *
 * Some timers might not span the whole experiment runtime. Here, these timers are merged if
 * possible. I.e. if two times share the same period and are sequential in time, they are merged
 * into a single timer which spans the range.
 *
 * Reduce collection of periodic timers by one pass over all timers while retaining execution
 * schedule.
 */
auto merge_timers_single_pass(std::vector<signal_flow::vertex::PlasticityRule::Timer>& timers)
{
	std::vector<signal_flow::vertex::PlasticityRule::Timer> merged_timers;
	for (auto const& timer : timers) {
		bool merged = false;
		for (auto& merged_timer : merged_timers) {
			if (timer.period != merged_timer.period) {
				continue;
			}
			if (timer.start == merged_timer.get_last()) {
				merged_timer.num_periods += timer.num_periods;
				merged = true;
				break;
			}
			if (merged_timer.start == timer.get_last()) {
				merged_timer.num_periods += timer.num_periods;
				merged_timer.start = timer.start;
				merged = true;
				break;
			}
		}
		if (!merged) {
			merged_timers.push_back(timer);
		}
	}
	return merged_timers;
}

/**
 * Merge periodic timer which have the same period and are adjacent to each other.
 *
 * Some timers might not span the whole experiment runtime. Here, these timers are merged if
 * possible. I.e. if two times share the same period and are sequential in time, they are merged
 * into a single timer which spans the range.
 *
 * Merges collection of periodic timers such that the execution timing is not changed, but
 * potentially the amount of timers necessary to describe the complete timing.
 */
void merge_timers(std::vector<signal_flow::vertex::PlasticityRule::Timer>& timers)
{
	size_t num = 0;
	do {
		num = timers.size();
		timers = merge_timers_single_pass(timers);
	} while (num != timers.size());
}

} // namespace

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

	std::map<signal_flow::vertex::PlasticityRule::ID, size_t> timed_recording_num_periods;
	for (auto const& [i, plasticity_rule, synapses, neurons, realtime_column_index] :
	     m_plasticity_rules) {
		timed_recording_num_periods[plasticity_rule.get_id()] +=
		    plasticity_rule.get_timer().num_periods;
	}

	std::vector<std::string> sources;

	std::map<signal_flow::vertex::PlasticityRule::ID, std::string> kernel_sources;
	std::map<signal_flow::vertex::PlasticityRule::ID, std::string> recording_sources;
	{
		for (auto const& [_, plasticity_rule, synapses, neurons, realtime_column_index] :
		     m_plasticity_rules) {
			std::stringstream kernel;
			auto kernel_str = plasticity_rule.get_kernel();
			std::string const kernel_name("PLASTICITY_RULE_KERNEL");
			kernel_str.replace(
			    kernel_str.find(kernel_name), kernel_name.size(),
			    "plasticity_rule_kernel_" + std::to_string(plasticity_rule.get_id().value()));

			// clang-format off
			std::string kernel_source_template = R"grenadeTemplate(
{{recorded_memory_declaration}}

{{kernel_str}}

## if has_raw_recording or has_timed_recording
void plasticity_rule_{{id}}(std::array<grenade::vx::ppu::SynapseArrayViewHandle, {{num_synapses}}>& synapses, std::array<grenade::vx::ppu::NeuronViewHandle, {{num_neurons}}>& neurons, Recording& recording)
{
	plasticity_rule_kernel_{{id}}(synapses, neurons, recording);
}
## else
void plasticity_rule_{{id}}(std::array<grenade::vx::ppu::SynapseArrayViewHandle, {{num_synapses}}>& synapses, std::array<grenade::vx::ppu::NeuronViewHandle, {{num_neurons}}>& neurons)
{
	plasticity_rule_kernel_{{id}}(synapses, neurons);
}
## endif
)grenadeTemplate";
			// clang-format on

			// clang-format off
			std::string recording_source_template = R"grenadeTemplate(
{{recorded_memory_declaration}}

## if has_timed_recording
Recording recorded_scratchpad_memory_{{id}}[{{plasticity_rule.timer.total_num_periods}}]
    __attribute__((section("{{recording_placement}}.data.keep")));
## else if has_raw_recording
Recording recorded_scratchpad_memory_{{id}} __attribute__((section("{{recording_placement}}.data.keep")));
## endif
)grenadeTemplate";
			// clang-format on

			inja::json parameters;
			parameters["id"] = plasticity_rule.get_id().value();

			parameters["kernel_str"] = kernel_str;
			parameters["has_raw_recording"] =
			    plasticity_rule.get_recording() &&
			    std::holds_alternative<signal_flow::vertex::PlasticityRule::RawRecording>(
			        *plasticity_rule.get_recording());
			parameters["has_timed_recording"] =
			    plasticity_rule.get_recording() &&
			    std::holds_alternative<signal_flow::vertex::PlasticityRule::TimedRecording>(
			        *plasticity_rule.get_recording());

			parameters["recorded_memory_declaration"] =
			    plasticity_rule.get_recorded_memory_declaration();

			parameters["num_synapses"] = synapses.size();
			parameters["num_neurons"] = neurons.size();

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

			parameters["plasticity_rule"]["timer"]["total_num_periods"] =
			    timed_recording_num_periods.at(plasticity_rule.get_id());

			auto const rendered_kernel_source = inja::render(kernel_source_template, parameters);
			if (!kernel_sources.contains(plasticity_rule.get_id())) {
				kernel_sources.emplace(plasticity_rule.get_id(), rendered_kernel_source);
			} else {
				if (kernel_sources.at(plasticity_rule.get_id()) != rendered_kernel_source) {
					throw std::runtime_error("Plasticity rules marked as same with an equal ID "
					                         "have different kernel sources.");
				}
			}

			auto const rendered_recording_source =
			    inja::render(recording_source_template, parameters);
			if (!recording_sources.contains(plasticity_rule.get_id())) {
				recording_sources.emplace(plasticity_rule.get_id(), rendered_recording_source);
			} else {
				if (recording_sources.at(plasticity_rule.get_id()) != rendered_recording_source) {
					throw std::runtime_error("Plasticity rules marked same with equal ID feature "
					                         "different recording sources.");
				}
			}
		}
	}
	for (auto const& [_, kernel_source] : kernel_sources) {
		sources.push_back(kernel_source);
	}
	for (auto const& [_, recording_source] : recording_sources) {
		sources.push_back(recording_source);
	}
	// plasticity rules
	// contains: kernel source, set(pair(plasticty vertex descriptor, realtime column index))
	std::map<std::string, std::set<std::pair<size_t, size_t>>> handles_sources;
	{
		for (auto const& [i, plasticity_rule, synapses, neurons, realtime_column_index] :
		     m_plasticity_rules) {
			// clang-format off
			std::string handles_source_template = R"grenadeTemplate(
#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/ppu/neuron_view_handle.h"
#include <array>
std::array<grenade::vx::ppu::SynapseArrayViewHandle, {{length(synapses)}}>
synapse_array_view_handle_INDEX = {
## for synapse in synapses
	[](){
		grenade::vx::ppu::SynapseArrayViewHandle synapse_array_view_handle;
		synapse_array_view_handle.hemisphere = static_cast<libnux::vx::PPUOnDLS>({{synapse.hemisphere}});
		synapse_array_view_handle.column_mask = 0;
## for column in synapse.enabled_columns
		synapse_array_view_handle.columns.push_back({{column}});
		synapse_array_view_handle.column_mask[{{column}}] = 1;
## endfor
## for row in synapse.enabled_rows
		synapse_array_view_handle.rows.push_back({{row}});
## endfor
		return synapse_array_view_handle;
	}(){% if not loop.is_last %},{% endif %}
## endfor
};
std::array<grenade::vx::ppu::NeuronViewHandle, {{length(neurons)}}>
neuron_view_handle_INDEX = {
## for neuron in neurons
	[](){
		grenade::vx::ppu::NeuronViewHandle neuron_view_handle;
		neuron_view_handle.hemisphere = static_cast<libnux::vx::PPUOnDLS>({{neuron.hemisphere}});
## for column in neuron.enabled_columns
		neuron_view_handle.columns.push_back({{column}});
## endfor
		return neuron_view_handle;
	}(){% if not loop.is_last %},{% endif %}
## endfor
};
)grenadeTemplate";
			// clang-format on
			inja::json parameters;
			parameters["synapses"] = inja::json::array();
			parameters["neurons"] = inja::json::array();
			for (auto const& [synram, synapse_array_view_handle] : synapses) {
				std::vector<size_t> enabled_columns;
				for (auto const column : synapse_array_view_handle.columns) {
					enabled_columns.push_back(column);
				}
				std::vector<size_t> enabled_rows;
				for (auto const row : synapse_array_view_handle.rows) {
					enabled_rows.push_back(row);
				}
				parameters["synapses"].push_back(
				    {{"hemisphere", synram.toHemisphereOnDLS().value()},
				     {"enabled_columns", enabled_columns},
				     {"enabled_rows", enabled_rows}});
			}
			for (auto const& [row, neuron_view_handle] : neurons) {
				std::vector<size_t> enabled_columns;
				for (auto const column : neuron_view_handle.columns) {
					enabled_columns.push_back(column);
				}
				parameters["neurons"].push_back(
				    {{"hemisphere", row.toHemisphereOnDLS().value()},
				     {"enabled_columns", enabled_columns}});
			}
			handles_sources[inja::render(handles_source_template, parameters)].insert(
			    {i, realtime_column_index});
		}
	}
	std::map<std::pair<size_t, size_t>, size_t> handles_indices;
	for (size_t i = 0; auto const& [handles_source, indices] : handles_sources) {
		std::string handles_source_copy = handles_source;
		std::string const index("INDEX");
		// twice due to synapse and neuron handles
		handles_source_copy.replace(
		    handles_source_copy.find(index), index.size(), std::to_string(i));
		handles_source_copy.replace(
		    handles_source_copy.find(index), index.size(), std::to_string(i));

		sources.push_back(handles_source_copy);
		for (auto const& index : indices) {
			handles_indices.emplace(index, i);
		}
		i++;
	}
	std::map<
	    std::pair<size_t, signal_flow::vertex::PlasticityRule::ID>,
	    std::vector<signal_flow::vertex::PlasticityRule::Timer>>
	    timers;
	{
		std::set<std::string> kernel_sources;
		for (auto const& [i, plasticity_rule, synapses, neurons, realtime_column_index] :
		     m_plasticity_rules) {
			// clang-format off
			std::string service_function_source_template = R"grenadeTemplate(
{{recorded_memory_declaration}}

#include "grenade/vx/ppu/synapse_array_view_handle.h"
#include "grenade/vx/ppu/neuron_view_handle.h"
#include <array>

extern std::array<grenade::vx::ppu::SynapseArrayViewHandle, {{length(synapses)}}>
synapse_array_view_handle_{{handles_index}};
extern std::array<grenade::vx::ppu::NeuronViewHandle, {{length(neurons)}}>
neuron_view_handle_{{handles_index}};
## if has_timed_recording
extern Recording recorded_scratchpad_memory_{{id}}[{{plasticity_rule.timer.total_num_periods}}];
## else if has_raw_recording
extern Recording recorded_scratchpad_memory_{{id}};
## endif
#include "libnux/vx/helper.h"
#include "libnux/vx/mailbox.h"
#include "libnux/scheduling/types.hpp"

## if has_raw_recording or has_timed_recording
void plasticity_rule_{{id}}(std::array<grenade::vx::ppu::SynapseArrayViewHandle, {{length(synapses)}}>& synapses, std::array<grenade::vx::ppu::NeuronViewHandle, {{length(neurons)}}>& neurons, Recording& recording);
## else
void plasticity_rule_{{id}}(std::array<grenade::vx::ppu::SynapseArrayViewHandle, {{length(synapses)}}>& synapses, std::array<grenade::vx::ppu::NeuronViewHandle, {{length(neurons)}}>& neurons);
## endif

size_t recorded_scratchpad_memory_period_{{id}} = 0;

void plasticity_rule_{{id}}_{{handles_index}}()
{
	libnux::vx::mailbox_write_string("plasticity rule {{id}}_{{handles_index}}: b: ");
	auto const b = get_time();
	libnux::vx::mailbox_write_int(b);
## if has_raw_recording or has_timed_recording
## if has_raw_recording
	plasticity_rule_{{id}}(
	    synapse_array_view_handle_{{handles_index}},
	    neuron_view_handle_{{handles_index}},
	    recorded_scratchpad_memory_{{id}});
## else if has_timed_recording
	plasticity_rule_{{id}}(
	    synapse_array_view_handle_{{handles_index}},
	    neuron_view_handle_{{handles_index}},
	    recorded_scratchpad_memory_{{id}}[recorded_scratchpad_memory_period_{{id}}]);
	// TODO: reset by routine called prior to realtime section for each batch entry
	// instead of modulo
	recorded_scratchpad_memory_period_{{id}}++;
	recorded_scratchpad_memory_period_{{id}} %= {{plasticity_rule.timer.total_num_periods}};
## endif
	libnux::vx::do_not_optimize_away(recorded_scratchpad_memory_{{id}});
## else
	plasticity_rule_{{id}}(
	    synapse_array_view_handle_{{handles_index}},
	    neuron_view_handle_{{handles_index}});
## endif
	libnux::vx::mailbox_write_string(" d: ");
	libnux::vx::mailbox_write_int(get_time() - b);
	libnux::vx::mailbox_write_string("\n");
})grenadeTemplate";
			// clang-format on
			inja::json parameters;
			parameters["i"] = i;
			parameters["max_realtime_column_index"] = max_realtime_column_index;
			parameters["realtime_column_index"] = realtime_column_index;
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
			parameters["plasticity_rule"]["timer"]["total_num_periods"] =
			    timed_recording_num_periods.at(plasticity_rule.get_id());
			parameters["synapses"] = inja::json::array();
			parameters["neurons"] = inja::json::array();
			for (auto const& [synram, synapse_array_view_handle] : synapses) {
				parameters["synapses"].push_back(
				    {{"hemisphere", synram.value()},
				     {"enabled_columns", synapse_array_view_handle.columns},
				     {"enabled_rows", synapse_array_view_handle.rows}});
			}
			for (auto const& [row, neuron_view_handle] : neurons) {
				parameters["neurons"].push_back(
				    {{"hemisphere", row.value()}, {"enabled_columns", neuron_view_handle.columns}});
			}
			parameters["id"] = plasticity_rule.get_id().value();
			auto const handles_index = handles_indices.at({i, realtime_column_index});
			parameters["handles_index"] = handles_indices.at({i, realtime_column_index});
			timers[std::make_pair(handles_index, plasticity_rule.get_id())].push_back(
			    plasticity_rule.get_timer());
			kernel_sources.insert(inja::render(service_function_source_template, parameters));
		}
		std::move(kernel_sources.begin(), kernel_sources.end(), std::back_inserter(sources));
		for (auto& [_, local_timers] : timers) {
			merge_timers(local_timers);
		}
		std::set<std::string> scheduling_sources;
		for (auto const& [i, plasticity_rule, synapses, neurons, realtime_column_index] :
		     m_plasticity_rules) {
			auto const handles_index = handles_indices.at({i, realtime_column_index});
			for (size_t j = 0; auto const& timer :
			                   timers.at(std::pair{handles_index, plasticity_rule.get_id()})) {
				// clang-format off
			std::string scheduling_source_template = R"grenadeTemplate(
extern void plasticity_rule_{{id}}_{{handles_index}}();
#include "libnux/scheduling/Timer.hpp"
Timer timer_{{handles_index}}_{{id}}_{{i}} = [](){
	Timer t(&plasticity_rule_{{id}}_{{handles_index}});
	t.set_first_deadline({{plasticity_rule.timer.start}});
	t.set_num_periods({{plasticity_rule.timer.num_periods}});
	t.set_period({{plasticity_rule.timer.period}});
	return t;
}();)grenadeTemplate";
				// clang-format on
				inja::json parameters;
				parameters["i"] = j;
				parameters["max_realtime_column_index"] = max_realtime_column_index;
				parameters["realtime_column_index"] = realtime_column_index;
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
				parameters["plasticity_rule"]["timer"]["start"] = timer.start.value();
				parameters["plasticity_rule"]["timer"]["num_periods"] = timer.num_periods;
				parameters["plasticity_rule"]["timer"]["period"] = timer.period.value();
				parameters["id"] = plasticity_rule.get_id().value();
				parameters["handles_index"] = handles_indices.at({i, realtime_column_index});
				scheduling_sources.insert(inja::render(scheduling_source_template, parameters));
				++j;
			}
		}
		std::move(
		    scheduling_sources.begin(), scheduling_sources.end(), std::back_inserter(sources));
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
#include "grenade/vx/ppu/detail/time.h"

extern volatile libnux::vx::PPUOnDLS ppu;

volatile uint64_t runtime;
volatile uint32_t scheduler_event_drop_count;

## for i in plasticity_rules_i
extern Timer timer_{{i.0}}_{{i.1}}_{{i.2}};
volatile uint32_t timer_{{i.0}}_{{i.1}}_{{i.2}}_event_drop_count;
## endfor

namespace {

Scheduler<32> scheduler;
auto timers = std::tie({% for i in plasticity_rules_i %}timer_{{i.0}}_{{i.1}}_{{i.2}}{% if not loop.is_last %}, {% endif %}{% endfor %});

} // namespace

void scheduling()
{
	static_cast<void>(ppu);
	static_cast<void>(runtime);

## if length(plasticity_rules_i) > 0
	auto current = get_time();
	grenade::vx::ppu::detail::initialize_time_origin();
	SchedulerSignallerTimer timer(current, current + runtime);
## for i in plasticity_rules_i
	timer_{{i.0}}_{{i.1}}_{{i.2}}.set_first_deadline(current + timer_{{i.0}}_{{i.1}}_{{i.2}}.get_first_deadline());
## endfor
	libnux::vx::mailbox_write_string("time:");
	libnux::vx::mailbox_write_int(current);
	libnux::vx::mailbox_write_string("\n");

	scheduler.execute(timer, timers);

	scheduler_event_drop_count = scheduler.get_dropped_events_count();
## for i in plasticity_rules_i
	timer_{{i.0}}_{{i.1}}_{{i.2}}_event_drop_count = timer_{{i.0}}_{{i.1}}_{{i.2}}.get_missed_count();
## endfor
## endif
})grenadeTemplate";
		// clang-format on

		inja::json parameters;
		{
			std::vector<std::array<size_t, 3>> plasticity_rules_i;
			for (auto const& [i, local_timers] : timers) {
				for (size_t j = 0; j < local_timers.size(); ++j) {
					plasticity_rules_i.emplace_back(std::array<size_t, 3>{i.first, i.second, j});
				}
			}
			parameters["plasticity_rules_i"] = plasticity_rules_i;
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
