#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/port_data.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/cadc.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synram.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {
namespace vertex {

/**
 * Readout of membrane voltages via the CADC.
 */
struct SYMBOL_VISIBLE CADCMembraneReadoutView : public EntityOnChip
{
	struct Results : public grenade::common::BatchedPortData
	{
		typedef std::vector<common::TimedDataSequence<std::vector<Int8>>> Samples;
		Samples samples;

		Results(Samples samples);

		virtual size_t batch_size() const override;

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(PortData const& other) const override;
	};

	virtual bool valid_output_port_data(
	    size_t output_port_on_vertex, grenade::common::PortData const& data) const override;

	typedef std::vector<halco::hicann_dls::vx::v3::SynapseOnSynapseRow> Columns;
	typedef halco::hicann_dls::vx::v3::SynramOnDLS Synram;

	enum class Mode
	{
		hagen,
		periodic,
		periodic_on_dram
	};

	/**
	 * Construct CADCMembraneReadoutView with specified size.
	 * @param columns Columns to read out
	 */
	CADCMembraneReadoutView(
	    Columns columns,
	    Synram const& synram,
	    Mode const& mode,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor);

	Columns const& get_columns() const SYMBOL_VISIBLE;
	Synram const& get_synram() const SYMBOL_VISIBLE;
	Mode const& get_mode() const SYMBOL_VISIBLE;

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
	Columns m_columns{};
	Synram m_synram{};
	Mode m_mode{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex
} // grenade::vx::signal_flow
