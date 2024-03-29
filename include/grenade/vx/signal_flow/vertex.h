#pragma once
#include "grenade/vx/signal_flow/detail/vertex_concept.h"
#include "grenade/vx/signal_flow/vertex/background_spike_source.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/data_input.h"
#include "grenade/vx/signal_flow/vertex/data_output.h"
#include "grenade/vx/signal_flow/vertex/external_input.h"
#include "grenade/vx/signal_flow/vertex/madc_readout.h"
#include "grenade/vx/signal_flow/vertex/neuron_event_output_view.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"
#include "grenade/vx/signal_flow/vertex/padi_bus.h"
#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"
#include "grenade/vx/signal_flow/vertex/synapse_driver.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <variant>

namespace grenade::vx::signal_flow {

/** Vertex configuration as variant over possible types. */
typedef std::variant<
    vertex::PlasticityRule,
    vertex::BackgroundSpikeSource,
    vertex::CrossbarL2Input,
    vertex::CrossbarL2Output,
    vertex::CrossbarNode,
    vertex::PADIBus,
    vertex::SynapseDriver,
    vertex::SynapseArrayViewSparse,
    vertex::SynapseArrayView,
    vertex::ExternalInput,
    vertex::DataInput,
    vertex::DataOutput,
    vertex::Transformation,
    vertex::NeuronView,
    vertex::NeuronEventOutputView,
    vertex::MADCReadoutView,
    vertex::PadReadoutView,
    vertex::CADCMembraneReadoutView>
    Vertex;

static_assert(
    sizeof(detail::CheckVertexConcept<Vertex>), "Vertices don't adhere to VertexConcept.");

} // namespace grenade::vx::signal_flow
