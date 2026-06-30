#pragma once
#include "grenade/common/connection_on_executor.h"
#include "grenade/common/linked_topology.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/placement_result.h"
#include "hate/visibility.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Placement algorithm for topology.
 * It places neurons on neuron circuits and Poisson sources on background generators.
 */
struct SYMBOL_VISIBLE Placer
{
	virtual ~Placer();

	/**
	 * Place given topology onto hardware entities.
	 * @param topology Topology to find placement result for
	 * @return Placement result
	 */
	virtual PlacementResult operator()(
	    grenade::common::LinkedTopology const& topology,
	    std::map<
	        grenade::common::ConnectionOnExecutor,
	        std::vector<common::ChipOnConnection>> const& chips_per_connection) const = 0;
};

GENPYBIND_MANUAL({
	using namespace ::grenade::vx::network::abstract;

	struct PyPlacer : public Placer
	{
		virtual PlacementResult operator()(
		    grenade::common::LinkedTopology const& topology,
		    std::map<
		        grenade::common::ConnectionOnExecutor,
		        std::vector<grenade::vx::common::ChipOnConnection>> const& chips_per_connection)
		    const override
		{
			PYBIND11_OVERLOAD_PURE(
			    PlacementResult, Placer, operator(), topology, chips_per_connection);
		}
	};

	parent->py::template class_<Placer, PyPlacer>(parent, "Placer")
	    .def("__call__", &Placer::operator());
})


} // namespace abstract
} // namespace grenade::vx::network
