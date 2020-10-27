#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v2/chip.h"
#include "halco/hicann-dls/vx/v2/routing_crossbar.h"
#include "halco/hicann-dls/vx/v2/synapse_driver.h"
#include "haldls/vx/v2/madc.h"
#include "haldls/vx/v2/padi.h"
#include "haldls/vx/v2/readout.h"
#include "haldls/vx/v2/routing_crossbar.h"
#include "haldls/vx/v2/synapse_driver.h"
#include "hate/visibility.h"
#include "lola/vx/v2/neuron.h"
#include "lola/vx/v2/synapse.h"
#include "stadls/vx/v2/dumper.h"
#include <boost/hana/adapt_struct.hpp>

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
	lola::vx::v2::SynapseMatrix synapse_matrix;

	typedef halco::common::typed_array<
	    haldls::vx::v2::SynapseDriverConfig,
	    halco::hicann_dls::vx::v2::SynapseDriverOnSynapseDriverBlock>
	    _synapse_driver_block_type GENPYBIND(opaque);
	/** Synapse driver block. TODO: should be lola container. */
	_synapse_driver_block_type synapse_driver_block;

	haldls::vx::v2::CommonPADIBusConfig common_padi_bus_config;

	typedef halco::common::
	    typed_array<lola::vx::v2::AtomicNeuron, halco::hicann_dls::vx::v2::NeuronColumnOnDLS>
	        _neuron_block_type GENPYBIND(opaque);
	/** Neuron block. TODO: should be lola container. */
	_neuron_block_type neuron_block;

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
	    typed_array<grenade::vx::HemisphereConfig, halco::hicann_dls::vx::v2::HemisphereOnDLS>
	        _hemispheres_type GENPYBIND(opaque);

	/** HemisphereConfig configuration. */
	_hemispheres_type hemispheres;

	typedef halco::common::
	    typed_array<haldls::vx::v2::CrossbarNode, haldls::vx::v2::CrossbarNode::coordinate_type>
	        _crossbar_nodes_type GENPYBIND(opaque);

	/** Crossbar node configuration. */
	_crossbar_nodes_type crossbar_nodes;

	/** Readout source selection configuration */
	haldls::vx::ReadoutSourceSelection readout_source_selection;

	/** MADC configuration */
	haldls::vx::MADCConfig madc_config;

	bool operator==(ChipConfig const& other) const SYMBOL_VISIBLE;
	bool operator!=(ChipConfig const& other) const SYMBOL_VISIBLE;
};

/**
 * Convert the PlaybackProgramBuilderDumper result to a ChipConfig.
 * This conversion is not bijective.
 * @param cocos Coordinate container pair sequence
 */
ChipConfig GENPYBIND(visible)
    convert_to_chip(stadls::vx::v2::Dumper::done_type const& cocos) SYMBOL_VISIBLE;

} // namespace grenade::vx

BOOST_HANA_ADAPT_STRUCT(
    grenade::vx::HemisphereConfig, synapse_matrix, synapse_driver_block, neuron_block);
BOOST_HANA_ADAPT_STRUCT(
    grenade::vx::ChipConfig, hemispheres, crossbar_nodes, readout_source_selection, madc_config);
