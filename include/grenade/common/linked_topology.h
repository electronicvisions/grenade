#pragma once
#include "grenade/common/genpybind.h"
#include "grenade/common/input_data.h"
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/common/inter_topology_hyper_edge_on_linked_topology.h"
#include "grenade/common/linked_graph.h"
#include "grenade/common/output_data.h"
#include "grenade/common/topology.h"
#include <memory>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

extern template struct SYMBOL_VISIBLE
    LinkedGraph<InterTopologyHyperEdgeOnLinkedTopology, InterTopologyHyperEdge, Topology, Topology>;

/**
 * Topology which allows linking of elements to a reference topology.
 * This enables e.g. keeping track of rewritten subgraphs and their corresponding
 * elements in the reference topology.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    inline_base("*LinkedGraph*"),
    holder_type("std::shared_ptr<grenade::common::LinkedTopology>")) LinkedTopology
    : public LinkedGraph<
          InterTopologyHyperEdgeOnLinkedTopology,
          InterTopologyHyperEdge,
          Topology,
          Topology>
{
	/**
	 * Construct linked topology with reference topology.
	 * @param reference Reference topology
	 */
	LinkedTopology(std::shared_ptr<Topology const> reference);

	GENPYBIND(return_value_policy(reference_internal))
	virtual Topology const& get_reference() const override;

	/**
	 * Get root topology at the end of a possible chain of multiple linked topologies.
	 */
	Topology const& get_root() const GENPYBIND(hidden);

	GENPYBIND(expose_as(get_root))
	std::shared_ptr<Topology const> _get_root() const;

	/**
	 * Map reference input data to link input data.
	 * The resulting link input data is not verified, but it is ensured, that the same
	 * link input data entries are not added by multiple mapping entries.
	 * @param reference_input_data InputData for reference topology
	 * @throws std::invalid_argument On input data not valid for reference topology
	 */
	InputData map_reference_input_data(InputData const& reference_input_data) const;

	/**
	 * Map link output data to reference output data.
	 * The resulting reference output data is not verified, but it is ensured, that the same
	 * reference output data entries are not added by multiple mapping entries.
	 * @param linked_output_data OutputData for linked topology
	 * @throws std::invalid_argument On output data not valid for linked topology
	 */
	OutputData map_reference_output_data(OutputData const& linked_output_data) const;

	/**
	 * Iteratively map root input data to linked input data.
	 * The resulting linked input data is not verified, but it is ensured, that the same
	 * linked input data entries are not added by multiple mapping entries.
	 * @param root_input_data InputData for root topology
	 * @throws std::invalid_argument On input data not valid for root topology
	 */
	InputData map_root_input_data(InputData const& root_input_data) const;

	/**
	 * Iteratively map linked output data to root output data.
	 * The resulting root output data is not verified, but it is ensured, that the same
	 * root output data entries are not added by multiple mapping entries.
	 * @param linked_output_data OutputData for linked topology
	 * @throws std::invalid_argument On output data not valid for linked topology
	 */
	OutputData map_root_output_data(OutputData const& linked_output_data) const;

	/**
	 * Create copy of linked topology with (deep-)copied reference topology.
	 */
	LinkedTopology deepcopy() const;

protected:
	virtual std::ostream& print(std::ostream& os) const override;

private:
	std::shared_ptr<Topology const> m_reference;
};

} // namespace common
} // namespace grenade
