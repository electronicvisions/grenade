#include "grenade/vx/signal_flow/execution_health_info.h"

#include "halco/common/iter_all.h"
#include "hate/indent.h"
#include <ostream>

namespace grenade::vx::signal_flow {

ExecutionHealthInfo::ExecutionInstance& ExecutionHealthInfo::ExecutionInstance::operator-=(
    ExecutionInstance const& rhs)
{
	using namespace halco::hicann_dls::vx::v3;
	hicann_arq_status.set_read_count(
	    hicann_arq_status.get_read_count() - rhs.hicann_arq_status.get_read_count());
	hicann_arq_status.set_write_count(
	    hicann_arq_status.get_write_count() - rhs.hicann_arq_status.get_write_count());
	hicann_arq_status.set_rx_count(
	    hicann_arq_status.get_rx_count() - rhs.hicann_arq_status.get_rx_count());
	hicann_arq_status.set_tx_count(
	    hicann_arq_status.get_tx_count() - rhs.hicann_arq_status.get_tx_count());

	for (auto const coord : halco::common::iter_all<PhyStatusOnFPGA>()) {
		phy_status[coord].set_crc_error_count(
		    phy_status[coord].get_crc_error_count() - rhs.phy_status[coord].get_crc_error_count());
		phy_status[coord].set_online_time(
		    phy_status[coord].get_online_time() - rhs.phy_status[coord].get_online_time());
		phy_status[coord].set_rx_dropped_count(
		    phy_status[coord].get_rx_dropped_count() -
		    rhs.phy_status[coord].get_rx_dropped_count());
		phy_status[coord].set_rx_count(
		    phy_status[coord].get_rx_count() - rhs.phy_status[coord].get_rx_count());
		phy_status[coord].set_tx_count(
		    phy_status[coord].get_tx_count() - rhs.phy_status[coord].get_tx_count());
	}
	for (auto const coord : halco::common::iter_all<CrossbarInputOnDLS>()) {
		crossbar_input_drop_counter[coord].set_value(
		    haldls::vx::v3::CrossbarInputDropCounter::Value(
		        ((crossbar_input_drop_counter[coord].get_value() +
		          haldls::vx::v3::CrossbarInputDropCounter::Value::size) -
		         rhs.crossbar_input_drop_counter[coord].get_value()) %
		        haldls::vx::v3::CrossbarInputDropCounter::Value::size));
	}
	for (auto const coord : halco::common::iter_all<CrossbarOutputOnDLS>()) {
		crossbar_output_event_counter[coord].set_value(
		    crossbar_output_event_counter[coord].get_value() -
		    rhs.crossbar_output_event_counter[coord].get_value());
	}

	return *this;
}

ExecutionHealthInfo::ExecutionInstance& ExecutionHealthInfo::ExecutionInstance::operator+=(
    ExecutionInstance const& rhs)
{
	using namespace halco::hicann_dls::vx::v3;
	hicann_arq_status.set_read_count(
	    hicann_arq_status.get_read_count() + rhs.hicann_arq_status.get_read_count());
	hicann_arq_status.set_write_count(
	    hicann_arq_status.get_write_count() + rhs.hicann_arq_status.get_write_count());
	hicann_arq_status.set_rx_count(
	    hicann_arq_status.get_rx_count() + rhs.hicann_arq_status.get_rx_count());
	hicann_arq_status.set_tx_count(
	    hicann_arq_status.get_tx_count() + rhs.hicann_arq_status.get_tx_count());

	for (auto const coord : halco::common::iter_all<PhyStatusOnFPGA>()) {
		phy_status[coord].set_crc_error_count(
		    phy_status[coord].get_crc_error_count() + rhs.phy_status[coord].get_crc_error_count());
		phy_status[coord].set_online_time(
		    phy_status[coord].get_online_time() + rhs.phy_status[coord].get_online_time());
		phy_status[coord].set_rx_dropped_count(
		    phy_status[coord].get_rx_dropped_count() +
		    rhs.phy_status[coord].get_rx_dropped_count());
		phy_status[coord].set_rx_count(
		    phy_status[coord].get_rx_count() + rhs.phy_status[coord].get_rx_count());
		phy_status[coord].set_tx_count(
		    phy_status[coord].get_tx_count() + rhs.phy_status[coord].get_tx_count());
	}
	for (auto const coord : halco::common::iter_all<CrossbarInputOnDLS>()) {
		crossbar_input_drop_counter[coord].set_value(
		    crossbar_input_drop_counter[coord].get_value() +
		    rhs.crossbar_input_drop_counter[coord].get_value());
	}
	for (auto const coord : halco::common::iter_all<CrossbarOutputOnDLS>()) {
		crossbar_output_event_counter[coord].set_value(
		    crossbar_output_event_counter[coord].get_value() +
		    rhs.crossbar_output_event_counter[coord].get_value());
	}

	return *this;
}

std::ostream& operator<<(std::ostream& os, ExecutionHealthInfo::ExecutionInstance const& info)
{
	using namespace halco::hicann_dls::vx::v3;
	hate::IndentingOstream ios(os);
	ios << "ExecutionInstance(\n";
	ios << hate::Indentation("\t");
	ios << info.hicann_arq_status << "\n";
	for (auto const coord : halco::common::iter_all<PhyStatusOnFPGA>()) {
		ios << coord << ": " << info.phy_status[coord] << "\n";
	}
	for (auto const coord : halco::common::iter_all<CrossbarInputOnDLS>()) {
		ios << coord << ": " << info.crossbar_input_drop_counter[coord] << "\n";
	}
	for (auto const coord : halco::common::iter_all<CrossbarOutputOnDLS>()) {
		ios << coord << ": " << info.crossbar_output_event_counter[coord] << "\n";
	}
	ios << hate::Indentation();
	ios << "\n)";
	return os;
}

void ExecutionHealthInfo::merge(ExecutionHealthInfo& other)
{
	merge(std::forward<ExecutionHealthInfo>(other));
}

void ExecutionHealthInfo::merge(ExecutionHealthInfo&& other)
{
	execution_instances.merge(other.execution_instances);
}

std::ostream& operator<<(std::ostream& os, ExecutionHealthInfo const& info)
{
	hate::IndentingOstream ios(os);
	ios << "ExecutionHealthInfo(\n";
	ios << hate::Indentation("\t");
	ios << "execution_instances:\n";
	ios << hate::Indentation("\t\t");
	for (auto const& [id, execution_instance] : info.execution_instances) {
		ios << id << ": " << execution_instance << "\n";
	}
	ios << hate::Indentation();
	os << ")";
	return os;
}

} // namespace grenade::vx::signal_flow
