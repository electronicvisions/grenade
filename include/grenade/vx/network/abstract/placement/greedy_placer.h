#pragma once
#include "grenade/common/linked_topology.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/placement/placer.h"
#include "grenade/vx/network/abstract/placement_result.h"
#include "hate/visibility.h"

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Greedy placement algorithm.
 * Neurons on populations are placed in order of iteration in the topology.
 * The placer uses the permutation sequences to decide where to place a neuron sequentially.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) GreedyPlacer : public Placer
{
	GreedyPlacer();

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

	/**
	 * Place given topology.
	 * @param topology Topology to place for
	 * @return Placement result
	 */
	virtual PlacementResult operator()(
	    grenade::common::LinkedTopology const& topology) const override;

private:
	NeuronPermutation m_neuron_permutation;
	BackgroundSourcePermutation m_background_source_permutation;
	log4cxx::LoggerPtr m_logger;
};

} // namespace abstract
} // namespace grenade::vx::network
