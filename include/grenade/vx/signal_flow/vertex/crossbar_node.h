#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/routing_crossbar.h"
#include "haldls/vx/v3/routing_crossbar.h"
#include "hate/visibility.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex {

/**
 * Crossbar node.
 */
struct SYMBOL_VISIBLE CrossbarNode : public EntityOnChip
{
	typedef haldls::vx::v3::CrossbarNode Config;
	typedef haldls::vx::v3::CrossbarNode::coordinate_type Coordinate;

	struct Parameterization : public grenade::common::PortData
	{
		haldls::vx::v3::CrossbarNode config;

		Parameterization(haldls::vx::v3::CrossbarNode config);

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual bool is_equal_to(PortData const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;
	};

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, grenade::common::PortData const& data) const override;

	/**
	 * Construct node at specified coordinate.
	 * @param coordinate Coordinate to use
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance to use
	 */
	CrossbarNode(
	    Coordinate const& coordinate,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor)
	    SYMBOL_VISIBLE;

	virtual std::vector<Port> get_input_ports() const override;

	virtual std::vector<Port> get_output_ports() const override;

	Coordinate coordinate{};

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Vertex const& other) const override;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // grenade::vx::signal_flow::vertex
