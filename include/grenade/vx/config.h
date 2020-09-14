#pragma once
#include <boost/hana/adapt_struct.hpp>
#include "grenade/vx/genpybind.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v1/chip.h"
#include "halco/hicann-dls/vx/v1/routing_crossbar.h"
#include "halco/hicann-dls/vx/v1/synapse_driver.h"
#include "haldls/vx/v1/padi.h"
#include "haldls/vx/v1/routing_crossbar.h"
#include "haldls/vx/v1/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/v1/synapse.h"
#include "stadls/vx/v1/dumper.h"

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
	lola::vx::v1::SynapseMatrix synapse_matrix;

	typedef halco::common::typed_array<
	    haldls::vx::v1::SynapseDriverConfig,
	    halco::hicann_dls::vx::v1::SynapseDriverOnSynapseDriverBlock>
	    _synapse_driver_block_type GENPYBIND(opaque);
	/** Synapse driver block. TODO: should be lola container. */
	_synapse_driver_block_type synapse_driver_block;

	haldls::vx::v1::CommonPADIBusConfig common_padi_bus_config;

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
	    typed_array<grenade::vx::HemisphereConfig, halco::hicann_dls::vx::v1::HemisphereOnDLS>
	        _hemispheres_type GENPYBIND(opaque);

	/** HemisphereConfig configuration. */
	_hemispheres_type hemispheres;

	typedef halco::common::
	    typed_array<haldls::vx::v1::CrossbarNode, haldls::vx::v1::CrossbarNode::coordinate_type>
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
    convert_to_chip(stadls::vx::v1::Dumper::done_type const& cocos) SYMBOL_VISIBLE;

} // namespace grenade::vx

BOOST_HANA_ADAPT_STRUCT(grenade::vx::HemisphereConfig, synapse_matrix, synapse_driver_block);
BOOST_HANA_ADAPT_STRUCT(grenade::vx::ChipConfig, hemispheres, crossbar_nodes);
