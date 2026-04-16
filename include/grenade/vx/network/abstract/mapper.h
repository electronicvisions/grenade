#pragma once
#include "grenade/common/linked_topology.h"
#include "grenade/common/topology.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <memory>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "pyhxcomm/vx/connection_handle.h"
#endif

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

struct Calibration;

/**
 * Mapper creating mapping for given topology between given model topology to an executable topology
 * for the back end(s).
 * The mapping is provided as a linked topology (potentially including intermediate layers) between
 * the model topology (root) and the executable topology.
 * Input data provided for the model topology can be mapped down to the executable topology and
 * output data from the executable topology can be mapped up to the model topology.
 * For this, inter topology hyper edges provide the mapping functionality.
 * TODO: Could be moved to common namespace once executor has common parent
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE Mapper
{
	virtual grenade::common::LinkedTopology GENPYBIND(hidden) operator()(
	    std::shared_ptr<grenade::common::Topology const> topology,
	    Calibration const& calibration,
	    grenade::vx::execution::JITGraphExecutor& executor) const = 0;

	GENPYBIND_MANUAL({
		using namespace grenade;
		parent.def(
		    "__call__",
		    [](GENPYBIND_PARENT_TYPE& self,
		       std::shared_ptr<::grenade::common::Topology const> const& topology,
		       vx::network::abstract::Calibration const& calibration,
		       ::pyhxcomm::Handle<vx::execution::JITGraphExecutor>& executor)
		        -> ::grenade::common::LinkedTopology {
			    return self(topology, calibration, executor.get());
		    },
		    pybind11::arg("topology"), pybind11::arg("calibration"), pybind11::arg("executor"));
	})
};

} // namespace abstract
} // namespace grenade::vx::network
