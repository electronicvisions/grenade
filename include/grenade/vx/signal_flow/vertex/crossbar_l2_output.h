#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "hate/visibility.h"
#include <iosfwd>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {
namespace vertex {

/**
 * Output from the Crossbar to the FPGA.
 * Since the data from the individual channels are merged, they are also presented merged here.
 */
struct SYMBOL_VISIBLE CrossbarL2Output : public EntityOnChip
{
	struct Results : public grenade::common::BatchedPortData
	{
		typedef std::vector<TimedSpikeFromChipSequence> Spikes;
		Spikes spikes;

		Results(Spikes spikes);

		virtual size_t batch_size() const override;

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(PortData const& other) const override;
	};

	virtual bool valid_output_port_data(
	    size_t output_port_on_vertex, grenade::common::PortData const& data) const override;

	/**
	 * Construct CrossbarL2Output.
	 * @param generates_output_port_data Whether to generate data on output port
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance to use
	 */
	CrossbarL2Output(
	    bool generates_output_port_data,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor)
	    SYMBOL_VISIBLE;

	virtual bool valid_edge_from(
	    Vertex const& source, grenade::common::Edge const& edge) const override;

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Vertex const& other) const override;

private:
	bool m_generates_output_port_data;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex
} // grenade::vx::signal_flow
