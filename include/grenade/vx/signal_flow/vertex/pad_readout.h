#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/readout.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <tuple>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {

namespace vertex {

/**
 * Readout of neuron voltages via a pad.
 */
struct SYMBOL_VISIBLE PadReadoutView : public EntityOnChip
{
	struct Parameterization : public grenade::common::PortData
	{
		bool enable_buffered{false};

		Parameterization();

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(PortData const& other) const override;
	};

	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, grenade::common::PortData const& data) const override;

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	typedef halco::hicann_dls::vx::v3::AtomicNeuronOnDLS Source;
	typedef halco::hicann_dls::vx::v3::PadOnDLS Coordinate;

	/**
	 * Construct PadReadoutView.
	 * @param source Neuron source and location to read out
	 * @param coordinate Pad to use
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance to use
	 */
	PadReadoutView(
	    Source const& source,
	    Coordinate const& coordinate,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor);

	Source source;
	Coordinate coordinate;

	virtual std::vector<Port> get_input_ports() const override;

	virtual std::vector<Port> get_output_ports() const override;

	virtual bool valid_edge_from(
	    Vertex const& source, grenade::common::Edge const& edge) const override;

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
