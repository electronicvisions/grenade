#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/fpga.h"
#include "haldls/vx/fpga.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <map>

namespace cereal {
struct access;
}

namespace grenade::vx::signal_flow::vertex {

struct SYMBOL_VISIBLE GENPYBIND(visible) SpikeIOSourcePopulation : public EntityOnChip
{
	struct Parameterization : public grenade::common::PortData
	{
		typedef haldls::vx::SpikeIOConfig Config;
		typedef std::map<halco::hicann_dls::vx::SpikeLabel, halco::hicann_dls::vx::SpikeIOAddress>
		    OutputRouting;
		typedef std::map<halco::hicann_dls::vx::SpikeIOAddress, halco::hicann_dls::vx::SpikeLabel>
		    InputRouting;

		Parameterization(
		    Config const& config,
		    OutputRouting const& output_routing,
		    InputRouting const& input_routing);

		Config config;
		OutputRouting output_routing;
		InputRouting input_routing;

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual bool is_equal_to(PortData const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;
	};

	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, grenade::common::PortData const& data) const override;

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	/**
	 * Construct SpikeIOSourcePopulation.
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance to use
	 */
	SpikeIOSourcePopulation(
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor);

	virtual bool valid_edge_to(
	    Vertex const& target, grenade::common::Edge const& edge) const override;

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual bool is_equal_to(Vertex const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex
