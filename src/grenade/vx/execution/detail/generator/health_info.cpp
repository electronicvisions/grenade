#include "grenade/vx/execution/detail/generator/health_info.h"
#include "grenade/vx/signal_flow/execution_health_info.h"
#include "haldls/vx/v3/arq.h"
#include "haldls/vx/v3/phy.h"
#include "haldls/vx/v3/routing_crossbar.h"

namespace grenade::vx::execution::detail::generator {

using namespace haldls::vx::v3;
using namespace stadls::vx::v3;
using namespace halco::hicann_dls::vx::v3;

signal_flow::ExecutionHealthInfo::ExecutionInstance HealthInfo::Result::get_execution_health_info()
    const
{
	signal_flow::ExecutionHealthInfo::ExecutionInstance ret;

	ret.hicann_arq_status = dynamic_cast<HicannARQStatus const&>(m_hicann_arq_status.get());

	for (auto const coord : halco::common::iter_all<PhyStatusOnFPGA>()) {
		ret.phy_status[coord] = dynamic_cast<PhyStatus const&>(m_phy_status[coord].get());
	}
	for (auto const coord : halco::common::iter_all<CrossbarInputOnDLS>()) {
		ret.crossbar_input_drop_counter[coord] = dynamic_cast<CrossbarInputDropCounter const&>(
		    m_crossbar_input_drop_counter[coord].get());
	}
	for (auto const coord : halco::common::iter_all<CrossbarOutputOnDLS>()) {
		ret.crossbar_output_event_counter[coord] = dynamic_cast<CrossbarOutputEventCounter const&>(
		    m_crossbar_output_event_counter[coord].get());
	}

	return ret;
}

stadls::vx::PlaybackGeneratorReturn<HealthInfo::Builder, HealthInfo::Result> HealthInfo::generate()
    const
{
	Result result;
	Builder builder;
	result.execution_duration = Timer::Value(0);

	result.m_hicann_arq_status = builder.read(result.execution_duration, HicannARQStatusOnFPGA());
	result.execution_duration += Timer::Value(2 * HicannARQStatus::read_config_size_in_words);

	for (auto const coord : halco::common::iter_all<PhyStatusOnFPGA>()) {
		result.m_phy_status[coord] = builder.read(result.execution_duration, coord);
		result.execution_duration += Timer::Value(2 * PhyStatus::read_config_size_in_words);
	}
	for (auto const coord : halco::common::iter_all<CrossbarInputOnDLS>()) {
		result.m_crossbar_input_drop_counter[coord] =
		    builder.read(result.execution_duration, coord);
		result.execution_duration +=
		    Timer::Value(2 * CrossbarInputDropCounter::read_config_size_in_words);
	}
	for (auto const coord : halco::common::iter_all<CrossbarOutputOnDLS>()) {
		result.m_crossbar_output_event_counter[coord] =
		    builder.read(result.execution_duration, coord);
		result.execution_duration +=
		    Timer::Value(2 * CrossbarOutputEventCounter::read_config_size_in_words);
	}

	return {std::move(builder), std::move(result)};
}

} // namespace grenade::vx::execution::detail::generator
