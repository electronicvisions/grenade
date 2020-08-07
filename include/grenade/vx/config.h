#pragma once
#include <boost/hana/adapt_struct.hpp>
#include "grenade/vx/genpybind.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/chip.h"
#include "halco/hicann-dls/vx/routing_crossbar.h"
#include "halco/hicann-dls/vx/synapse_driver.h"
#include "haldls/vx/padi.h"
#include "haldls/vx/routing_crossbar.h"
#include "haldls/vx/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/synapse.h"
#include "stadls/vx/dumper.h"

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

/**
 * Container encapsulating configuration on a hemisphere of the chip needed for graph operation.
 */
class GENPYBIND(visible) HemisphereConfig
{
public:
	/** Default constructor. */
	HemisphereConfig() SYMBOL_VISIBLE;

	/** Synapse matrix. */
	lola::vx::SynapseMatrix synapse_matrix;

	typedef halco::common::typed_array<
	    haldls::vx::SynapseDriverConfig,
	    halco::hicann_dls::vx::SynapseDriverOnSynapseDriverBlock>
	    _synapse_driver_block_type GENPYBIND(opaque);
	/** Synapse driver block. TODO: should be lola container. */
	_synapse_driver_block_type synapse_driver_block;

	haldls::vx::CommonPADIBusConfig common_padi_bus_config;

	bool operator==(HemisphereConfig const& other) const SYMBOL_VISIBLE;
	bool operator!=(HemisphereConfig const& other) const SYMBOL_VISIBLE;
};


/**
 * Container encapsulating configuration of the chip needed for graph operation.
 * TODO: Replace by lola chip object as soon as available.
 */
class GENPYBIND(visible) ChipConfig
{
public:
	/** Default constructor. */
	ChipConfig() SYMBOL_VISIBLE;

	typedef halco::common::
	    typed_array<grenade::vx::HemisphereConfig, halco::hicann_dls::vx::HemisphereOnDLS>
	        _hemispheres_type GENPYBIND(opaque);

	/** HemisphereConfig configuration. */
	_hemispheres_type hemispheres;

	typedef halco::common::
	    typed_array<haldls::vx::CrossbarNode, haldls::vx::CrossbarNode::coordinate_type>
	        _crossbar_nodes_type GENPYBIND(opaque);

	/** Crossbar node configuration. */
	_crossbar_nodes_type crossbar_nodes;

	bool operator==(ChipConfig const& other) const SYMBOL_VISIBLE;
	bool operator!=(ChipConfig const& other) const SYMBOL_VISIBLE;
};

/**
 * Convert the PlaybackProgramBuilderDumper result to a ChipConfig.
 * This conversion is not bijective.
 * @param cocos Coordinate container pair sequence
 */
ChipConfig GENPYBIND(visible)
    convert_to_chip(stadls::vx::Dumper::done_type const& cocos) SYMBOL_VISIBLE;

} // namespace grenade::vx

BOOST_HANA_ADAPT_STRUCT(grenade::vx::HemisphereConfig, synapse_matrix, synapse_driver_block);
BOOST_HANA_ADAPT_STRUCT(grenade::vx::ChipConfig, hemispheres, crossbar_nodes);
