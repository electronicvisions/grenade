#include <gtest/gtest.h>

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/input_data.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "lola/vx/v3/chip.h"

TEST(Reconfiguration, CADC)
{
	using namespace grenade::vx::signal_flow;
	using namespace grenade::common;
	using namespace grenade::vx::common;
	using namespace grenade::vx::network::abstract;
	grenade::common::ExecutionInstanceOnExecutor instance;
	auto topology_using_cadc = std::make_shared<Topology>();
	vertex::NeuronView neuron(
	    vertex::NeuronView::Columns(1), vertex::NeuronView::Row(), ChipOnConnection(),
	    TimeDomainOnTopology(), instance);
	auto const neuron_descriptor = topology_using_cadc->add_vertex(neuron);
	vertex::CADCMembraneReadoutView cadc(
	    vertex::CADCMembraneReadoutView::Columns{
	        {halco::hicann_dls::vx::v3::SynapseOnSynapseRow{}}},
	    vertex::CADCMembraneReadoutView::Synram(), vertex::CADCMembraneReadoutView::Mode::periodic,
	    ChipOnConnection(), TimeDomainOnTopology(), instance);
	auto const cadc_descriptor = topology_using_cadc->add_vertex(cadc);

	auto topology_not_using_cadc = std::make_shared<Topology>();
	topology_not_using_cadc->add_vertex(neuron);

	grenade::vx::execution::JITGraphExecutor executor;
	InputData input_data;
	input_data.time_domain_runtimes.set(
	    TimeDomainOnTopology(),
	    ClockCycleTimeDomainRuntimes({grenade::vx::common::Time(10000)}, Time()));

	input_data.ports.set(
	    {neuron_descriptor, 1},
	    vertex::NeuronView::Parameterization({vertex::NeuronView::Parameterization::Config{}}));

	auto outputs = grenade::vx::execution::run(
	    executor,
	    {topology_not_using_cadc, topology_using_cadc, topology_not_using_cadc, topology_using_cadc,
	     topology_using_cadc},
	    {input_data, input_data, input_data, input_data, input_data});

	EXPECT_TRUE(outputs.at(0).ports.empty());
	EXPECT_GE(
	    dynamic_cast<vertex::CADCMembraneReadoutView::Results const&>(
	        outputs.at(1).ports.get({cadc_descriptor, 0}))
	        .samples.at(0)
	        .size(),
	    0);
	EXPECT_TRUE(outputs.at(2).ports.empty());
	EXPECT_GE(
	    dynamic_cast<vertex::CADCMembraneReadoutView::Results const&>(
	        outputs.at(3).ports.get({cadc_descriptor, 0}))
	        .samples.at(0)
	        .size(),
	    0);
	EXPECT_GE(
	    dynamic_cast<vertex::CADCMembraneReadoutView::Results const&>(
	        outputs.at(4).ports.get({cadc_descriptor, 0}))
	        .samples.at(0)
	        .size(),
	    0);
}
