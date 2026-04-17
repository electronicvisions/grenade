#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"

namespace grenade::vx::signal_flow {
namespace vertex GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * Catch-all chip base configuration.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) Chip : public EntityOnChip
{
	struct Parameterization : public grenade::common::PortData
	{
		lola::vx::v3::Chip base;

		Parameterization(lola::vx::v3::Chip base);

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

	Chip(
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor)
	    SYMBOL_VISIBLE;

	virtual std::vector<Vertex::Port> get_input_ports() const override;
	virtual std::vector<Vertex::Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual bool is_equal_to(grenade::common::Vertex const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace vertex
} // namespace grenade::vx::signal_flow
