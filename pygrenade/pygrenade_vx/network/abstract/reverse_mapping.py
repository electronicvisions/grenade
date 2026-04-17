# pylint: disable=invalid-name
from typing import Dict, List
import _pygrenade_vx_network_abstract
import pygrenade_common
from dlens_vx_v3 import halco, lola


def get_locally_placed_neuron_coordinates(
        population_descriptor: pygrenade_common.VertexOnTopology,
        mapped_topology: pygrenade_common.LinkedTopology) \
        -> List[halco.LogicalNeuronOnDLS]:
    """
    Get coordinates of a population with LocallyPlacedNeuron cell type.

    :param population_descriptor: Vertex descriptor of population
    :param mapped_topology: Mapped topology
    :returns: List of LogicalNeuronOnDLS coordinates,
              one per neuron on population
    """
    # pylint: disable=too-many-locals
    population = mapped_topology.get_reference().get_reference()\
        .get(population_descriptor)

    cell = population.get_cell()
    assert isinstance(cell, _pygrenade_vx_network_abstract.LocallyPlacedNeuron)

    population_size = population.size()

    anchors = [None] * population_size

    inter_graph_hyper_edge_descriptors = mapped_topology\
        .get_reference().get_reference()\
        .inter_graph_hyper_edges_by_reference(population_descriptor)

    for inter_graph_hyper_edge_descriptor in \
            inter_graph_hyper_edge_descriptors:
        compatible_vertex_descriptor = mapped_topology\
            .get_reference().get_reference().links(
                inter_graph_hyper_edge_descriptor)[0]
        compatible_inter_graph_hyper_edge_descriptors = \
            mapped_topology.get_reference()\
            .inter_graph_hyper_edges_by_reference(
                compatible_vertex_descriptor)
        for compatible_inter_graph_hyper_edge_descriptor in \
                compatible_inter_graph_hyper_edge_descriptors:
            partitioned_vertex_descriptors = mapped_topology\
                .get_reference().links(
                    compatible_inter_graph_hyper_edge_descriptor)
            assert len(partitioned_vertex_descriptors) == 1
            section = mapped_topology.get_reference().get(
                partitioned_vertex_descriptors[0])
            for section_mapping_descriptor in mapped_topology\
                    .inter_graph_hyper_edges_by_reference(
                        partitioned_vertex_descriptors[0]):
                section_mapping = mapped_topology.get(
                    section_mapping_descriptor)
                if not isinstance(
                        section_mapping,
                        _pygrenade_vx_network_abstract
                        .LocallyPlacedNeuronMapping):
                    continue
                for i, element in enumerate(
                        section.get_shape().get_elements()):
                    anchors[element.value[0]] = \
                        section_mapping.anchors[i][1]

    coords = []
    for anchor in anchors:
        assert anchor is not None
        coords.append(halco.LogicalNeuronOnDLS(cell.shape, anchor))
    return coords


def get_calibrated_neuron_actual_hardware_parameters(
        population_descriptor: pygrenade_common.VertexOnTopology,
        mapped_topology: pygrenade_common.LinkedTopology) \
        -> List[Dict[
            pygrenade_common.CompartmentOnNeuron, List[lola.AtomicNeuron]]]:
    """
    Get hardware configurations (by mutable reference) of a population with
    CalibratedNeuron cell type.

    :param population_descriptor: Vertex descriptor of population
    :param mapped_topology: Mapped topology
    :returns: Dict of atomic neuron containers per compartment per neuron on
              population
    """
    # pylint: disable=too-many-locals
    population = mapped_topology.get_reference().get_reference()\
        .get(population_descriptor)

    cell = population.get_cell()
    assert isinstance(cell, _pygrenade_vx_network_abstract.CalibratedNeuron)

    population_size = population.size()

    configs = [None] * population_size

    inter_graph_hyper_edge_descriptors = mapped_topology\
        .get_reference().get_reference()\
        .inter_graph_hyper_edges_by_reference(population_descriptor)

    # pylint: disable=too-many-nested-blocks
    for inter_graph_hyper_edge_descriptor in \
            inter_graph_hyper_edge_descriptors:
        compatible_vertex_descriptor = mapped_topology\
            .get_reference().get_reference().links(
                inter_graph_hyper_edge_descriptor)[0]
        compatible_inter_graph_hyper_edge_descriptors = \
            mapped_topology.get_reference()\
            .inter_graph_hyper_edges_by_reference(
                compatible_vertex_descriptor)
        for compatible_inter_graph_hyper_edge_descriptor in \
                compatible_inter_graph_hyper_edge_descriptors:
            partitioned_vertex_descriptors = mapped_topology\
                .get_reference().links(
                    compatible_inter_graph_hyper_edge_descriptor)
            assert len(partitioned_vertex_descriptors) == 1
            section = mapped_topology.get_reference().get(
                partitioned_vertex_descriptors[0])
            for section_mapping_descriptor in mapped_topology\
                    .inter_graph_hyper_edges_by_reference(
                        partitioned_vertex_descriptors[0]):
                section_mapping = mapped_topology.get(
                    section_mapping_descriptor)
                if not isinstance(
                        section_mapping,
                        _pygrenade_vx_network_abstract
                        .CalibratedNeuronMapping):
                    continue
                for cell_on_population, element in enumerate(
                        section.get_shape().get_elements()):
                    configs[element.value[0]] = {}
                    for compartment_on_neuron, compartment in \
                            cell.compartments.items():
                        compartment_configs = []
                        for atomic_neuron_on_compartment in range(
                                len(compartment.receptors)):
                            compartment_configs.append(
                                section_mapping.get_config(
                                    cell_on_population,
                                    compartment_on_neuron,
                                    atomic_neuron_on_compartment))
                        configs[element.value[0]].update({
                            compartment_on_neuron: compartment_configs})

    return configs


def get_uncalibrated_synapse_coordinates(
        projection_descriptor: pygrenade_common.VertexOnTopology,
        mapped_topology: pygrenade_common.LinkedTopology) \
        -> List[List[halco.SynapseOnDLS]]:
    """
    Get coordinates of a projection with UncalibratedSynapse synapse type.

    :param projection_descriptor: Vertex descriptor of projection
    :param mapped_topology: Mapped topology
    :returns: List of synapse coordinates per synapse on projection
    """
    # pylint: disable=too-many-locals
    projection = mapped_topology.get_reference().get_reference()\
        .get(projection_descriptor)

    synapse = projection.get_synapse()
    assert isinstance(
        synapse, _pygrenade_vx_network_abstract.UncalibratedSynapse)

    projection_size = projection.size()

    coords = [[] for i in range(projection_size)]

    inter_graph_hyper_edge_descriptors = mapped_topology\
        .get_reference().get_reference()\
        .inter_graph_hyper_edges_by_reference(projection_descriptor)

    connector = projection.get_connector()
    assert isinstance(connector, pygrenade_common.StaticConnector)
    connector_input_output = connector.get_input_sequence().cartesian_product(
        connector.get_output_sequence())
    synapse_connections = connector.get_synapse_connections(
        connector_input_output)

    # pylint: disable=too-many-nested-blocks
    for inter_graph_hyper_edge_descriptor in \
            inter_graph_hyper_edge_descriptors:
        compatible_vertex_descriptor = mapped_topology\
            .get_reference().get_reference().links(
                inter_graph_hyper_edge_descriptor)[0]
        compatible_inter_graph_hyper_edge_descriptors = \
            mapped_topology.get_reference()\
            .inter_graph_hyper_edges_by_reference(
                compatible_vertex_descriptor)
        for compatible_inter_graph_hyper_edge_descriptor in \
                compatible_inter_graph_hyper_edge_descriptors:
            partitioned_vertex_descriptors = mapped_topology\
                .get_reference().links(
                    compatible_inter_graph_hyper_edge_descriptor)
            assert len(partitioned_vertex_descriptors) == 1
            section = mapped_topology.get_reference().get(
                partitioned_vertex_descriptors[0])
            for section_mapping_descriptor in mapped_topology\
                    .inter_graph_hyper_edges_by_reference(
                        partitioned_vertex_descriptors[0]):
                section_mapping = mapped_topology.get(
                    section_mapping_descriptor)
                hardware_vertex_descriptors = mapped_topology.links(
                    section_mapping_descriptor)
                if not isinstance(
                        section_mapping,
                        _pygrenade_vx_network_abstract
                        .UncalibratedSynapseMapping):
                    continue
                section_connector = section.get_connector()
                assert isinstance(
                    section_connector, pygrenade_common.StaticConnector)
                section_synapse_connections = section_connector\
                    .get_synapse_connections(connector_input_output)
                synapses_on_projection = \
                    pygrenade_common.CuboidMultiIndexSequence([
                        synapse_connections.size()])\
                    .related_sequence_subset_restriction(
                        synapse_connections, section_synapse_connections)\
                    .get_elements()
                for i, synapse_on_projection in enumerate(
                        synapses_on_projection):
                    hardware_vertex_translation = \
                        section_mapping.translation[i]
                    for hardware_vertex_index, synapse_indices in \
                            enumerate(hardware_vertex_translation):
                        synapse_vertex = mapped_topology.get(
                            hardware_vertex_descriptors[hardware_vertex_index])
                        for synapse_index in synapse_indices:
                            synapse = synapse_vertex.synapses[synapse_index]
                            coord = halco.SynapseOnDLS(
                                halco.SynapseOnSynram(
                                    synapse_vertex.columns[
                                        synapse.index_column],
                                    synapse_vertex.rows[synapse.index_row]),
                                synapse_vertex.synram)
                            coords[synapse_on_projection.value[0]].append(
                                coord)

    return coords
