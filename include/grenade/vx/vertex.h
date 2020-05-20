#pragma once
#include <variant>

#include "grenade/vx/vertex/addition.h"
#include "grenade/vx/vertex/cadc_readout.h"
#include "grenade/vx/vertex/crossbar_l2_output.h"
#include "grenade/vx/vertex/crossbar_node.h"
#include "grenade/vx/vertex/data_input.h"
#include "grenade/vx/vertex/data_output.h"
#include "grenade/vx/vertex/external_input.h"
#include "grenade/vx/vertex/neuron_event_output.h"
#include "grenade/vx/vertex/neuron_view.h"
#include "grenade/vx/vertex/padi_bus.h"
#include "grenade/vx/vertex/synapse_array_view.h"
#include "grenade/vx/vertex/synapse_driver.h"

namespace grenade::vx {

/**
 * A vertex is an entity which has a defined number of inputs and outputs with defined type.
 * If they are the same, one size() / connection_type() function is provided, else input_size() and
 * output_size() / input_connection_type() and output_connection_type() are used if the number /
 * type of inputs and outputs differs.
 */

/** Vertex as variant over possible types. */
typedef std::variant<
    vertex::CrossbarL2Output,
    vertex::CrossbarNode,
    vertex::PADIBus,
    vertex::SynapseDriver,
    vertex::SynapseArrayView,
    vertex::Addition,
    vertex::ExternalInput,
    vertex::DataInput,
    vertex::DataOutput,
    vertex::NeuronView,
    vertex::NeuronEventOutputView,
    vertex::CADCMembraneReadoutView>
    Vertex;

} // namespace grenade::vx
