#include "grenade/vx/ppu_program_generator.h"

#include "grenade/vx/ppu.h"
#include "hate/join.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"
#include <vector>
#include <log4cxx/logger.h>

namespace grenade::vx {

void PPUProgramGenerator::add(
    vertex::PlasticityRule const& rule,
    std::vector<
        std::pair<halco::hicann_dls::vx::v3::SynramOnDLS, ppu::SynapseArrayViewHandle>> const&
        synapses)
{
	m_plasticity_rules.push_back({rule, synapses});
}

std::vector<std::string> PPUProgramGenerator::done()
{
	hate::Timer timer;
	static log4cxx::Logger* const logger =
	    log4cxx::Logger::getLogger("grenade.PPUProgramGenerator.done()");

	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	std::vector<std::string> sources;
	// plasticity rules
	{
		size_t i = 0;
		for (auto const& [plasticity_rule, synapses] : m_plasticity_rules) {
			std::stringstream kernel;
			auto kernel_str = plasticity_rule.get_kernel();
			std::string const kernel_name("PLASTICITY_RULE_KERNEL");

			kernel_str.replace(
			    kernel_str.find(kernel_name), kernel_name.size(),
			    "plasticity_rule_kernel_" + std::to_string(i));
			kernel << kernel_str;
			kernel << "\n";
			kernel << "namespace {\n";
			kernel << "std::array<grenade::vx::ppu::SynapseArrayViewHandle, " << synapses.size()
			       << "> synapse_array_view_handle = {\n";
			size_t l = 0;
			for (auto const& [synram, synapse_array_view_handle] : synapses) {
				kernel << "[](){\n";
				kernel << "grenade::vx::ppu::SynapseArrayViewHandle synapse_array_view_handle;\n";
				for (size_t j = 0; j < 256; ++j) {
					if (synapse_array_view_handle.columns.test(j)) {
						kernel << "synapse_array_view_handle.columns.set(" << j << ");\n";
					}
					if (synapse_array_view_handle.rows.test(j)) {
						kernel << "synapse_array_view_handle.rows.set(" << j << ");\n";
					}
				}
				kernel << "return synapse_array_view_handle;\n";
				kernel << "}()";
				if (l != synapses.size() - 1) {
					kernel << ",";
				}
				kernel << "\n";
				l++;
			}
			kernel << "};\n";
			kernel << "std::array<libnux::vx::PPUOnDLS, " << synapses.size() << "> synrams = {\n";
			l = 0;
			for (auto const& [synram, synapse_array_view_handle] : synapses) {
				kernel << "libnux::vx::PPUOnDLS(" << synram.value() << ")";
				if (l != synapses.size() - 1) {
					kernel << ",";
				}
				kernel << "\n";
				l++;
			}
			kernel << "};\n";
			kernel << "} // namespace\n";
			kernel << "#include \"libnux/vx/mailbox.h\"\n";
			kernel << "#include \"libnux/scheduling/types.hpp\"\n";
			kernel << "void plasticity_rule_" << i << "() {\n";
			kernel << "libnux::vx::mailbox_write_string(\"plasticity rule " << i << ": b: \");\n";
			kernel << "auto const b = get_time();\n";
			kernel << "libnux::vx::mailbox_write_int(b);\n";
			kernel << "plasticity_rule_kernel_" << i << "(synapse_array_view_handle, synrams);\n";
			kernel << "libnux::vx::mailbox_write_string(\" d: \");\n";
			kernel << "libnux::vx::mailbox_write_int(get_time() - b);\n";
			kernel << "libnux::vx::mailbox_write_string(\"\\n\");\n";
			kernel << "}\n";
			kernel << "#include \"libnux/scheduling/Service.hpp\"\n";
			kernel << "Service_Function<" << i << ", &plasticity_rule_" << i << "> service_" << i
			       << ";\n";
			kernel << "#include \"libnux/scheduling/Timer.hpp\"\n";
			kernel << "Timer timer_" << i << " = [](){ Timer t; t.set_first_deadline("
			       << plasticity_rule.get_timer().start.value() << "); t.set_num_periods("
			       << plasticity_rule.get_timer().num_periods << "); t.set_period("
			       << plasticity_rule.get_timer().period.value() << "); t.set_service(service_" << i
			       << "); return t; }();\n";
			sources.push_back(kernel.str());
			i++;
		}
	}
	// scheduler
	{
		std::stringstream source;
		source << "#include \"libnux/scheduling/SchedulerSignaller.hpp\"\n";
		source << "#include \"libnux/scheduling/Scheduler.hpp\"\n";
		source << "#include \"libnux/scheduling/Timer.hpp\"\n";
		source << "#include \"libnux/scheduling/Service.hpp\"\n";
		source << "#include \"libnux/vx/mailbox.h\"\n";
		source << "#include \"libnux/vx/dls.h\"\n";
		source << "extern volatile libnux::vx::PPUOnDLS ppu;\n";
		source << "volatile uint32_t runtime;\n";
		source << "volatile uint32_t scheduler_event_drop_count;\n";
		for (size_t i = 0; i < m_plasticity_rules.size(); ++i) {
			source << "extern Timer timer_" << i << ";\n";
			source << "volatile uint32_t timer_" << i << "_event_drop_count;\n";
		}
		for (size_t i = 0; i < m_plasticity_rules.size(); ++i) {
			source << "extern void plasticity_rule_" << i << "();\n";
			source << "extern Service_Function<" << i << ", &plasticity_rule_" << i << "> service_"
			       << i << ";\n";
		}
		source << "namespace {\n";
		source << "Scheduler<32> scheduler;\n";
		source << "auto timers = std::tie(";
		{
			std::vector<std::string> s;
			for (size_t i = 0; i < m_plasticity_rules.size(); ++i) {
				s.push_back("timer_" + std::to_string(i));
			}
			source << hate::join_string(s.begin(), s.end(), ",\n") << ");\n";
		}
		source << "auto services = std::tie(";
		{
			std::vector<std::string> s;
			for (size_t i = 0; i < m_plasticity_rules.size(); ++i) {
				s.push_back("service_" + std::to_string(i));
			}
			source << hate::join_string(s.begin(), s.end(), ",\n") << ");\n";
		}
		source << "} // namespace\n";
		source << "void scheduling() {\n";
		source << "static_cast<void>(ppu);\n";
		source << "static_cast<void>(runtime);\n";
		if (!m_plasticity_rules.empty()) {
			source << "auto current = get_time();\n";
			source << "SchedulerSignallerTimer timer(current, current + runtime);\n";
			for (size_t i = 0; i < m_plasticity_rules.size(); ++i) {
				source << "timer_" << i << ".set_first_deadline(current + timer_" << i
				       << ".get_first_deadline());\n";
			}
			source << "libnux::vx::mailbox_write_string(\"time:\");\n";
			source << "libnux::vx::mailbox_write_int(current);\n";
			source << "libnux::vx::mailbox_write_string(\"\\n\");\n";
			source << "scheduler.execute(timer, services, timers);\n";
			source << "scheduler_event_drop_count = scheduler.get_dropped_events_count();\n";
			for (size_t i = 0; i < m_plasticity_rules.size(); ++i) {
				source << "timer_" << i << "_event_drop_count = timer_" << i
				       << ".get_missed_count();\n";
			}
		}
		source << "}";
		sources.push_back(source.str());
	}
	// periodic CADC readout
	{
		size_t const num_samples = has_periodic_cadc_readout ? num_cadc_samples_in_extmem : 0;

		std::stringstream source;
		source << "#include \"grenade/vx/ppu/status.h\"\n";
		source << "#include \"libnux/vx/dls.h\"\n";
		source << "#include \"libnux/vx/vector.h\"\n";
		source << "#include \"libnux/vx/globaladdress.h\"\n";
		source << "#include \"libnux/vx/time.h\"\n";
		source << "#include <array>\n";
		source << "#include <tuple>\n";
		source << "\n";
		source << "extern volatile libnux::vx::PPUOnDLS ppu;\n";
		source << "extern volatile grenade::vx::ppu::Status status;\n";
		source << "std::tuple<std::array<std::pair<uint64_t, libnux::vx::vector_row_t>, "
		       << num_samples
		       << ">, uint32_t> periodic_cadc_samples_top "
		          "__attribute__((section(\"ext.data\")));\n";
		source << "std::tuple<std::array<std::pair<uint64_t, libnux::vx::vector_row_t>, "
		       << num_samples
		       << ">, uint32_t> periodic_cadc_samples_bot "
		          "__attribute__((section(\"ext.data\")));\n";
		source << "\n";
		source << "auto& local_periodic_cadc_samples = "
		          "std::get<0>(ppu == libnux::vx::PPUOnDLS::bottom ? periodic_cadc_samples_bot : "
		          "periodic_cadc_samples_top);\n";
		source << "void perform_periodic_read_recording(size_t const offset) {\n";
		source << "using namespace libnux::vx;\n";
		source << "if (offset >= local_periodic_cadc_samples.size()) {\n";
		source << "  return;\n";
		source << "}\n";
		source << "// synchronize time-stamp and CADC readout\n";
		source << "asm volatile(\n";
		source << "\"sync\\n\"\n";
		source << ":::\n";
		source << ");\n";
		source << "uint64_t const time = libnux::vx::now();\n";
		source << "// clang-format off\n";
		source << "asm volatile(\n";
		source << "\"fxvinx 0, %[ca_base], %[i]\\n\"\n";
		source << "\"fxvinx 1, %[cab_base], %[j]\\n\"\n";
		source << "\"fxvoutx 0, %[b0], %[i]\\n\"\n";
		source << "\"fxvoutx 1, %[b1], %[i]\\n\"\n";
		source << "::\n";
		source << "  [b0] \"b\" (GlobalAddress::from_global(0, "
		          "reinterpret_cast<uint32_t>(&(local_periodic_cadc_samples[offset].second.even"
		          ")) & 0x3fff'ffff).to_extmem().to_fxviox_addr()),\n";
		source << "  [b1] \"b\" (GlobalAddress::from_global(0, "
		          "reinterpret_cast<uint32_t>(&(local_periodic_cadc_samples[offset].second.odd"
		          ")) & 0x3fff'ffff).to_extmem().to_fxviox_addr()),\n";
		source << "  [ca_base] \"r\" (dls_causal_base),\n";
		source << "  [cab_base] \"r\" (dls_causal_base|dls_buffer_enable_mask),\n";
		source << "  [i] \"r\" (uint32_t(0)),\n";
		source << "  [j] \"r\" (uint32_t(1))\n";
		source << ": \"qv0\", \"qv1\"\n";
		source << ");\n";
		source << "local_periodic_cadc_samples[offset].first = time;\n";
		source << "// clang-format on\n";
		source << "}\n";
		source << "void perform_periodic_read()\n";
		source << "{\n";
		source << "using namespace libnux::vx;\n";
		source << "uint32_t offset = 0;\n";
		source << "uint32_t size = 0;\n";
		source << "// instruction cache warming\n";
		source << "perform_periodic_read_recording(offset);\n";
		source << "status = grenade::vx::ppu::Status::inside_periodic_read;\n";
		source << "while (status != grenade::vx::ppu::Status::stop_periodic_read) {\n";
		source << "perform_periodic_read_recording(offset);\n";
		source << "offset++;\n";
		source << "size++;\n";
		source << "}\n";
		source << "std::get<1>(ppu == libnux::vx::PPUOnDLS::bottom ? periodic_cadc_samples_bot : "
		          "periodic_cadc_samples_top) = size;\n";
		source << "asm volatile(\"fxvinx 0, %[b0], %[i]\\n\"\n";
		source << "             \"sync\\n\"\n";
		source << "             :: [b0] "
		          "\"r\"(dls_extmem_base), [i] \"r\"(uint32_t(0))\n";
		source << "             : \"qv0\", \"memory\");\n";
		source << "}";
		sources.push_back(source.str());
	}

	LOG4CXX_TRACE(
	    logger, "Generated PPU program sources:\n"
	                << hate::join_string(sources.begin(), sources.end(), "\n\n") << ".");
	LOG4CXX_TRACE(logger, "Generated PPU program sources in " << timer.print() << ".");
	return sources;
}

} // namespace grenade::vx
