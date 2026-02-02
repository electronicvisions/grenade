#pragma once
#include "dapr/empty_property.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/routing_table_entry.h"
#include "haldls/vx/v3/routing_table_entry.h"
#include "hate/visibility.h"
#include <map>

namespace grenade::vx::signal_flow {
namespace vertex {

/**
 * Output routing table of a chip.
 * Needs to be connected to a crossbar l2 output and the inter chip router.
 */
struct SYMBOL_VISIBLE OutputRoutingTable : public EntityOnChip
{
	struct SYMBOL_VISIBLE Parameterization : public grenade::common::PortData
	{
		Parameterization(std::map<
		                 halco::hicann_dls::vx::v3::OutputRoutingTableEntryOnFPGA,
		                 haldls::vx::v3::OutputRoutingTableEntry::Label> table_entries);

		virtual std::unique_ptr<grenade::common::PortData> copy() const override;
		virtual std::unique_ptr<grenade::common::PortData> move() override;

		std::map<
		    halco::hicann_dls::vx::v3::OutputRoutingTableEntryOnFPGA, // is SpikeLabel
		    haldls::vx::v3::OutputRoutingTableEntry::Label>
		    entries;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(grenade::common::PortData const& other) const override;
	};

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	OutputRoutingTable(
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor)
	    SYMBOL_VISIBLE;

	virtual bool valid_edge_from(
	    Vertex const& source, grenade::common::Edge const& edge) const override;

	virtual bool valid_edge_to(
	    Vertex const& target, grenade::common::Edge const& edge) const override;

	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;
	virtual std::vector<grenade::common::Vertex::Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Vertex const& other) const override;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};


} // namespace vertex
} // namespace grenade::vx::signal_flow