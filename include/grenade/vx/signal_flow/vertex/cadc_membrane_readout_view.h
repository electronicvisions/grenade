#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/port.h"
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

struct PortRestriction;

namespace vertex {

struct NeuronView;

/**
 * Readout of membrane voltages via the CADC.
 */
struct CADCMembraneReadoutView : public EntityOnChip
{
	constexpr static bool can_connect_different_execution_instances = false;

	/**
	 * Columns to record as collection of collections of columns.
	 * Each inner collection corresponds to one input port of the vertex and thus allows recording
	 * neurons from different non-contiguous input vertices.
	 */
	typedef std::vector<std::vector<halco::hicann_dls::vx::v3::SynapseOnSynapseRow>> Columns;
	typedef halco::hicann_dls::vx::v3::SynramOnDLS Synram;

	enum class Mode
	{
		hagen,
		periodic,
		periodic_on_dram
	};

	typedef std::vector<std::vector<lola::vx::v3::AtomicNeuron::Readout::Source>> Sources;

	CADCMembraneReadoutView() = default;

	/**
	 * Construct CADCMembraneReadoutView with specified size.
	 * @param columns Columns to read out
	 */
	template <typename ColumnsT, typename SynramT, typename SourcesT>
	explicit CADCMembraneReadoutView(
	    ColumnsT&& columns,
	    SynramT&& synram,
	    Mode const& mode,
	    SourcesT&& sources,
	    ChipCoordinate const& chip_coordinate = ChipCoordinate());

	Columns const& get_columns() const SYMBOL_VISIBLE;
	Synram const& get_synram() const SYMBOL_VISIBLE;
	Mode const& get_mode() const SYMBOL_VISIBLE;
	Sources const& get_sources() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::vector<Port> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, CADCMembraneReadoutView const& config)
	    SYMBOL_VISIBLE;

	bool supports_input_from(
	    NeuronView const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

	bool operator==(CADCMembraneReadoutView const& other) const SYMBOL_VISIBLE;
	bool operator!=(CADCMembraneReadoutView const& other) const SYMBOL_VISIBLE;

private:
	Columns m_columns{};
	Synram m_synram{};
	Mode m_mode{};
	Sources m_sources{};

	void check(Columns const& columns, Sources const& sources) SYMBOL_VISIBLE;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex

} // grenade::vx::signal_flow

#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.tcc"
