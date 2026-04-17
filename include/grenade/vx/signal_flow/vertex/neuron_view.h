#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/ppu/neuron_view_handle.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <array>
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {
namespace vertex {

/**
 * A view of neuron circuits.
 */
struct SYMBOL_VISIBLE NeuronView : public EntityOnChip
{
	struct Parameterization : public grenade::common::PortData
	{
		struct Config
		{
			lola::vx::v3::AtomicNeuron atomic_neuron_config;
			bool enable_reset;

			bool operator==(Config const& other) const SYMBOL_VISIBLE;
			bool operator!=(Config const& other) const SYMBOL_VISIBLE;

		private:
			friend struct cereal::access;
			template <typename Archive>
			void serialize(Archive& ar, std::uint32_t version);
		};

		std::vector<Config> configs;

		Parameterization(std::vector<Config> configs);

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(PortData const& other) const override;
	};

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	typedef std::vector<halco::hicann_dls::vx::v3::NeuronColumnOnDLS> Columns;
	typedef halco::hicann_dls::vx::v3::NeuronRowOnDLS Row;

	/**
	 * Construct NeuronView with specified neurons.
	 * @param columns Neuron columns
	 * @param enable_resets Enable values for initial reset of the neurons
	 * @param row Neuron row
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance to use
	 */
	NeuronView(
	    Columns columns,
	    Row const& row,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor);

	Row row;

	Columns const& get_columns() const;

	/**
	 * Convert to neuron view handle for PPU programs.
	 */
	ppu::NeuronViewHandle toNeuronViewHandle() const SYMBOL_VISIBLE;

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual bool valid_edge_from(
	    Vertex const& source, grenade::common::Edge const& edge) const override;

	virtual bool valid_edge_to(
	    Vertex const& target, grenade::common::Edge const& edge) const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual bool is_equal_to(Vertex const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	Columns m_columns;
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex
} // grenade::vx::signal_flow
