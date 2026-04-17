#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/background.h"
#include "haldls/vx/background.h"
#include "hate/visibility.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {

namespace vertex {

/**
 * Background event source to the Crossbar.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) BackgroundSpikeSource : public EntityOnChip
{
	struct Parameterization : public grenade::common::PortData
	{
		haldls::vx::BackgroundSpikeSource config;

		Parameterization(haldls::vx::BackgroundSpikeSource config);

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

	typedef halco::hicann_dls::vx::BackgroundSpikeSourceOnDLS Coordinate;
	Coordinate coordinate;

	/**
	 * Construct BackgroundSpikeSource.
	 * @param coordinate Coordinate to use
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance to use
	 */
	BackgroundSpikeSource(
	    Coordinate const& coordinate,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor);

	virtual bool valid_edge_to(
	    Vertex const& target, grenade::common::Edge const& edge) const override;

	virtual std::vector<PartitionedVertex::Port> get_input_ports() const override;
	virtual std::vector<PartitionedVertex::Port> get_output_ports() const override;

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

} // vertex

} // grenade::vx::signal_flow
