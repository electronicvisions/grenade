#pragma once
#include "grenade/common/linked_topology.h"
#include "grenade/common/topology.h"
#include "grenade/vx/execution/detail/connection_acquisor.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/mapper.h"
#include "grenade/vx/network/abstract/placement/greedy_placer.h"
#include "grenade/vx/network/routing/greedy_router.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "hate/visibility.h"
#include <memory>

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include "pyhxcomm/vx/connection_handle.h"
#endif

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

struct Calibration;
struct GreedyMapper;

#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
namespace detail {

template <typename... Ts>
struct GreedyMapperCallUnrollPyBind11Helper
{
	GreedyMapperCallUnrollPyBind11Helper(pybind11::class_<GreedyMapper, Mapper>&){};
};

template <typename T, typename... Ts>
struct GreedyMapperCallUnrollPyBind11Helper<std::variant<T, Ts...>>
    : GreedyMapperCallUnrollPyBind11Helper<std::variant<Ts...>>
{
	using parent_t = GreedyMapperCallUnrollPyBind11Helper<std::variant<Ts...>>;

	GreedyMapperCallUnrollPyBind11Helper(pybind11::class_<GreedyMapper, Mapper>& m);
};

} // namespace detail
#endif


/**
 * Mapper performing sequential steps to map from a model topology to a hardware topology.
 * It performs the following steps:
 * - create partitioning-compatible topology layer and rewrite it
 * - create partitined topology layer and partition using the population rewrite to split
 *   populations, followed by projection, recorder, plasticity rule and execution instance rewrite
 * - perform placement and add placed entities into hardware topology layer
 * - perform calibration and add its results into the linked topology
 * - perform routing and add routing entities into hardware topology layer
 */
struct GENPYBIND(visible) SYMBOL_VISIBLE GreedyMapper : public Mapper
{
	GreedyMapper();

	typedef std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> NeuronPermutation;
	typedef std::vector<halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock>
	    BackgroundSourcePermutation;

	GENPYBIND(getter_for(neuron_permutation))
	NeuronPermutation const& get_neuron_permutation() const;

	GENPYBIND(setter_for(neuron_permutation))
	void set_neuron_permutation(NeuronPermutation value);

	GENPYBIND(getter_for(background_source_permutation))
	BackgroundSourcePermutation const& get_background_source_permutation() const;

	GENPYBIND(setter_for(background_source_permutation))
	void set_background_source_permutation(BackgroundSourcePermutation value);

	void set_router(std::shared_ptr<routing::Router> router);

	virtual grenade::common::LinkedTopology GENPYBIND(hidden) operator()(
	    std::shared_ptr<grenade::common::Topology const> topology,
	    Calibration const& calibration,
	    grenade::vx::execution::JITGraphExecutor& executor) const;

	GENPYBIND_MANUAL({
		[[maybe_unused]] ::grenade::vx::network::abstract::detail::
		    GreedyMapperCallUnrollPyBind11Helper<
		        std::remove_cvref_t<::pyhxcomm::vx::ConnectionHandle>>
		        helper(parent);

		using namespace grenade;
		parent.def(
		    "__call__",
		    [](GENPYBIND_PARENT_TYPE const& self,
		       std::shared_ptr<::grenade::common::Topology const> const& topology,
		       vx::network::abstract::Calibration const& calibration,
		       ::pyhxcomm::Handle<vx::execution::JITGraphExecutor>& executor)
		        -> ::grenade::common::LinkedTopology {
			    return self(topology, calibration, executor.get());
		    },
		    pybind11::arg("topology"), pybind11::arg("calibration"), pybind11::arg("executor"));
	})

private:
	GreedyPlacer m_placer;
	std::shared_ptr<routing::Router> m_router;
	log4cxx::LoggerPtr m_logger;
};


#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
namespace detail {

template <typename T, typename... Ts>
GreedyMapperCallUnrollPyBind11Helper<std::variant<T, Ts...>>::GreedyMapperCallUnrollPyBind11Helper(
    pybind11::class_<GreedyMapper, Mapper>& m) :
    parent_t(m)
{
	m.def(
	    "__call__",
	    [](GreedyMapper const& self,
	       std::shared_ptr<grenade::common::Topology const> const& topology,
	       Calibration const& calibration, T& conn) -> grenade::common::LinkedTopology {
		    execution::detail::ConnectionAcquisor<T> acquisor(conn);
		    return self(topology, calibration, *acquisor.executor);
	    },
	    pybind11::arg("topology"), pybind11::arg("calibration"), pybind11::arg("connection"));
}

} // namespace detail
#endif


} // namespace abstract
} // namespace grenade::vx::network
