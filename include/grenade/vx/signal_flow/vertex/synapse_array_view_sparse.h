#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "halco/hicann-dls/vx/v3/synram.h"
#include "hate/visibility.h"
#include "lola/vx/v3/synapse.h"
#include <array>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <vector>
#include <boost/range/iterator_range.hpp>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::ppu {
struct SynapseArrayViewHandle;
} // namespace grenade::vx::ppu

namespace grenade::vx::signal_flow {
namespace vertex GENPYBIND_TAG_GRENADE_VX_SIGNAL_FLOW {

/**
 * A sparse view of synapses connected to a set of synapse drivers.
 */
struct GENPYBIND(
    visible,
    inline_base("*EntityOnChip*"),
    holder_type("std::shared_ptr<grenade::vx::signal_flow::vertex::SynapseArrayViewSparse>"))
    SYMBOL_VISIBLE SynapseArrayViewSparse : public EntityOnChip
{
	typedef std::vector<halco::hicann_dls::vx::v3::SynapseRowOnSynram> Rows;
	typedef halco::hicann_dls::vx::v3::SynramOnDLS Synram;
	typedef std::vector<halco::hicann_dls::vx::v3::SynapseOnSynapseRow> Columns;

	struct Parameterization : public grenade::common::PortData
	{
		typedef std::vector<lola::vx::v3::SynapseMatrix::Label> Labels;
		typedef std::vector<lola::vx::v3::SynapseMatrix::Weight> Weights;
		Labels labels;
		Weights weights;

		Parameterization(Labels labels, Weights weights);

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

	struct Synapse
	{
		size_t index_row;
		size_t index_column;

		bool operator==(Synapse const& other) const SYMBOL_VISIBLE;
		bool operator!=(Synapse const& other) const SYMBOL_VISIBLE;

	private:
		friend struct cereal::access;
		template <typename Archive>
		void serialize(Archive& ar, std::uint32_t);
	};

	typedef std::vector<Synapse> Synapses;

	SynapseArrayViewSparse() = default;

	/**
	 * Construct synapse array view.
	 * @param synram Synram location of synapses
	 * @param rows Coordinates of rows
	 * @param columns Coordinates of columns
	 * @param synapses Synapse values
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance to use
	 */
	explicit SynapseArrayViewSparse(
	    Synram synram,
	    Rows rows,
	    Columns columns,
	    Synapses synapses,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor);

	/**
	 * Accessor to synapse row coordinates via a range.
	 * @return Range of synapse row coordinates
	 */
	boost::iterator_range<Rows::const_iterator> get_rows() const SYMBOL_VISIBLE GENPYBIND(hidden);

	/**
	 * Accessor to synapse column coordinates via a range.
	 * @return Range of synapse column coordinates
	 */
	boost::iterator_range<Columns::const_iterator> get_columns() const SYMBOL_VISIBLE
	    GENPYBIND(hidden);

	/**
	 * Accessor to synapse configuration via a range.
	 * @return Range of synapse configuration
	 */
	boost::iterator_range<Synapses::const_iterator> get_synapses() const SYMBOL_VISIBLE
	    GENPYBIND(hidden);

	Synram const& get_synram() const SYMBOL_VISIBLE GENPYBIND(hidden);

	/**
	 * Convert to synapse array view handle for PPU programs.
	 */
	ppu::SynapseArrayViewHandle toSynapseArrayViewHandle() const SYMBOL_VISIBLE GENPYBIND(hidden);

	virtual bool valid_edge_from(
	    Vertex const& source, grenade::common::Edge const& edge) const override;

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

	Synram synram{};
	Rows rows{};
	Columns columns{};
	Synapses synapses{};

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Vertex const& other) const override;

	void check(Rows const& rows, Columns const& columns, Synapses const& synapses) SYMBOL_VISIBLE;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex
} // grenade::vx::signal_flow
