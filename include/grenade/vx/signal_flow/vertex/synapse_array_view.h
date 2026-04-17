#pragma once
#include "dapr/empty_property.h"
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
#include <vector>
#include <boost/range/iterator_range.hpp>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex {

/**
 * A rectangular view of synapses connected to a set of synapse drivers.
 */
struct SYMBOL_VISIBLE SynapseArrayView : public EntityOnChip
{
	struct Parameterization : public grenade::common::PortData
	{
		typedef std::vector<std::vector<lola::vx::v3::SynapseMatrix::Label>> Labels;
		typedef std::vector<std::vector<lola::vx::v3::SynapseMatrix::Weight>> Weights;

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

	typedef std::vector<halco::hicann_dls::vx::v3::SynapseRowOnSynram> Rows;
	typedef halco::hicann_dls::vx::v3::SynramOnDLS Synram;
	typedef std::vector<halco::hicann_dls::vx::v3::SynapseOnSynapseRow> Columns;

	/**
	 * Construct synapse array view.
	 * @param synram Synram location of synapses
	 * @param rows Coordinates of rows
	 * @param columns Coordinates of columns
	 * @param weights Weight values
	 * @param labels Label values
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance to use
	 */
	SynapseArrayView(
	    Synram const& synram,
	    Rows rows,
	    Columns columns,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor);

	/**
	 * Accessor to synapse row coordinates via a range.
	 * @return Range of synapse row coordinates
	 */
	boost::iterator_range<Rows::const_iterator> get_rows() const SYMBOL_VISIBLE;

	/**
	 * Accessor to synapse column coordinates via a range.
	 * @return Range of synapse column coordinates
	 */
	boost::iterator_range<Columns::const_iterator> get_columns() const SYMBOL_VISIBLE;

	Synram const& get_synram() const SYMBOL_VISIBLE;

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
	Synram m_synram{};
	Rows m_rows{};
	Columns m_columns{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // grenade::vx::signal_flow::vertex
