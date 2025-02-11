#include "grenade/vx/network/abstract/execution_instance_global.h"

#include "hate/indent.h"
#include "hate/timer.h"
#include "hate/variant.h"

namespace grenade::vx::network::abstract {

std::unique_ptr<grenade::common::OutputData::ExecutionInstance> ExecutionInstanceGlobal::copy()
    const
{
	return std::make_unique<ExecutionInstanceGlobal>(*this);
}

std::unique_ptr<grenade::common::OutputData::ExecutionInstance> ExecutionInstanceGlobal::move()
{
	return std::make_unique<ExecutionInstanceGlobal>(std::move(*this));
}

bool ExecutionInstanceGlobal::is_equal_to(
    grenade::common::OutputData::ExecutionInstance const& other) const
{
	auto const& other_global = static_cast<ExecutionInstanceGlobal const&>(other);
	return device_usage_duration == other_global.device_usage_duration &&
	       realtime_duration == other_global.realtime_duration &&
	       execution_health_info == other_global.execution_health_info &&
	       read_ppu_symbols == other_global.read_ppu_symbols &&
	       pre_execution_chips == other_global.pre_execution_chips;
}

std::ostream& ExecutionInstanceGlobal::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Global(\n";

	ios << hate::Indentation("\t");
	ios << "device_usage_duration:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [chip_on_connection, duration] : device_usage_duration) {
		os << chip_on_connection << ": " << hate::to_string(duration) << "\n";
	}

	ios << hate::Indentation("\t");
	os << "\trealtime_duration:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [chip_on_connection, duration] : realtime_duration) {
		os << chip_on_connection << ": " << hate::to_string(duration) << "\n";
	}

	ios << hate::Indentation("\t");
	if (execution_health_info) {
		ios << *execution_health_info << "\n";
	}

	ios << "read_ppu_symbols:\n";
	ios << hate::Indentation("\t\t");
	for (size_t b = 0; auto const& batch_entry : read_ppu_symbols) {
		ios << "batch entry " << b << ":\n";
		ios << hate::Indentation("\t\t\t");
		for (auto const& [chip_on_connection, symbols] : batch_entry) {
			ios << chip_on_connection << ":\n";
			ios << hate::Indentation("\t\t\t\t");
			for (auto const& [name, value] : symbols) {
				ios << name << ":";
				std::visit(
				    hate::overloaded{
				        [&ios](std::map<
				               halco::hicann_dls::vx::v3::HemisphereOnDLS,
				               haldls::vx::v3::PPUMemoryBlock> const& v) {
					        ios << "\n";
					        ios << hate::Indentation("\t\t\t\t\t");
					        for (auto const& [hemisphere, vv] : v) {
						        ios << hemisphere << ": " << vv << "\n";
					        }
					        ios << hate::Indentation("\t\t\t\t");
				        },
				        [&ios](auto const& v) { ios << " " << v << "\n"; }},
				    value);
			}
		}
		b++;
	}

	ios << hate::Indentation("\t");
	ios << "pre_execution_chips:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [chip_on_connection, chip] : pre_execution_chips) {
		ios << chip_on_connection << ": " << chip << "\n";
	}

	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
