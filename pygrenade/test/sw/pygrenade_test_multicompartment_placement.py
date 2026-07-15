# !/usr/bin/env python
# pylint: disable=too-many-locals
from typing import List, Mapping, Set, Tuple, Optional
import unittest

import matplotlib.pyplot as plt
from matplotlib import lines
import numpy as np

import pygrenade_vx.network.abstract as grenade
import pygrenade_vx


def neuron_from_edgelist(
        connections: List[Tuple[int, int]],
        compartments: Optional[Mapping[int, grenade.Compartment]] = None,
        param_spaces: Optional[
            Mapping[int, grenade.Compartment.ParameterSpace]] = None,
) -> Tuple[grenade.Neuron, grenade.Neuron.ParameterSpace]:
    """
    Create a grenade neuron and its parameter space from an edegelist.

    :param connections: List defining the connections of different
        compartments.
    :param compartments: Map (integer id as key) of compartments
        to add to the morphology builder. If no compartment is provided
        a compartment with a small capacitance is used per default.
    :param param_spaces: Map (integer id as key) of parameters spaces
        belonging to the provided compartments.
    :return: Constructed neuron and resource manager.
    """

    neuron = grenade.Neuron()

    comp_ids = {}
    p_spaces_neuron = {}
    neuron_indices = range(np.max(connections) + 1) if len(connections) > 0 \
        else [0]
    for idx in neuron_indices:
        compartment, parameter_space = get_cap_compartment()
        if compartments is not None and idx in compartments:
            compartment = compartments[idx]
            if param_spaces is None or idx not in param_spaces:
                raise ValueError(
                    "You need to provide a parameter space for "
                    "each index for which a compartment is provided")
            parameter_space = param_spaces[idx]
        comp_ids[idx] = neuron.add_compartment(compartment)
        p_spaces_neuron[comp_ids[idx]] = parameter_space

    neuron_parameter_space = grenade.Neuron.ParameterSpace()
    neuron_parameter_space.compartments = p_spaces_neuron

    for (parent, child) in connections:
        neuron.add_compartment_connection(
            comp_ids[parent],
            comp_ids[child],
            grenade.CompartmentConnectionConductance())
    return neuron, neuron_parameter_space


def neuron_and_res_from_edgelist(
        connections: List[Tuple[int, int]],
        compartments: Optional[Mapping[int, grenade.Compartment]] = None,
        param_spaces: Optional[
            Mapping[int, grenade.Compartment.ParameterSpace]] = None,
) -> Tuple[grenade.Neuron, grenade.ResourceManager]:
    """
    Create a grenade neuron and its resources from an edgelist.

    Note, the environment of the resources is default constructed,
        i.e. it this function can not be used if synaptic inputs
        are present on the neuron.

    :param connections: List defining the connections of different
        compartments.
    :param compartments: Map (integer id as key) of compartments
        to add to the morphology builder. If no compartment is provided
        a compartment with a small capacitance is used per default.
    :param param_spaces: Map (integer id as key) of parameters spaces
        belonging to the provided compartments.
    :return: Constructed neuron and resource manager.
    """

    resources = grenade.ResourceManager()
    environment = grenade.Environment()

    neuron, neuron_parameter_space = neuron_from_edgelist(
        connections, compartments, param_spaces)

    resources.add_config(neuron, neuron_parameter_space, environment)

    return neuron, resources


def plot_switch_shared_conductance(axis, i, j):
    '''
    Helper to plot a conductance between the neuron circuit at the given
    coordinates and the shared line.

    :param axis: Figure to add conductance to.
    :param i: X-Coordinate of the neuron circuit.
    :param j: Y-Coordiante of the neuron circuit.
    '''
    if 1 - i == 0:
        axis.add_line(lines.Line2D([j + 0.5, j + 0.5], [2, 2.1]))
        axis.add_line(lines.Line2D([j + 0.5, j + 0.6], [2.1, 2.1]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.15, 2.1]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.15, 2.2]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.25, 2.2]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.25, 2.3]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.35, 2.3]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.35, 2.4]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.45, 2.4]))

        axis.add_line(lines.Line2D([j + 0.5, j + 0.4], [2.45, 2.45]))
        axis.add_line(lines.Line2D([j + 0.5, j + 0.5], [2.45, 2.55]))
    else:
        axis.add_line(lines.Line2D([j + 0.5, j + 0.5], [0, -0.1]))
        axis.add_line(lines.Line2D([j + 0.5, j + 0.6], [-0.1, -0.1]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.15, -0.1]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.15, -0.2]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.25, -0.2]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.25, -0.3]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.35, -0.3]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.35, -0.4]))
        axis.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.45, -0.4]))

        axis.add_line(lines.Line2D([j + 0.5, j + 0.4], [-0.45, -0.45]))
        axis.add_line(lines.Line2D([j + 0.5, j + 0.5], [-0.45, -0.55]))


def plot_switch_top_bottom(axis, i, j):
    '''
    Helper to plot a conductance between the neuron circuit at the given
    coordinates and the neuron circuit in the opposite row.

    :param axis: Figure to add conductance to.
    :param i: X-Coordinate of the neuron circuit.
    :param j: Y-Coordiante of the neuron circuit.
    '''
    if 1 - i == 0:
        axis.add_line(lines.Line2D([j + 0.5, j + 0.5], [1, 1.2]))
    else:
        axis.add_line(lines.Line2D([j + 0.5, j + 0.5], [1, 0.8]))


def plot_switch_right(axis, i, j):
    '''
    Helper to plot a switch between a neuron circuit and its right neighour
    circuit.

    :param axis: Figure to add conductance to.
    :param i: X-Coordinate of the neuron circuit.
    :param j: Y-Coordiante of the neuron circuit.
    '''
    axis.add_line(lines.Line2D([j + 0.8, j + 1.2], [i + 0.5, i + 0.5]))


def plot_switch_shared(axis, i, j):
    '''
    Helper to plot a switch between a neuron circuit and the shared line.

    :param axis: Figure to add conductance to.
    :param i: X-Coordinate of the neuron circuit.
    :param j: Y-Coordiante of the neuron circuit.
    '''
    if 1 - i == 0:
        axis.add_line(lines.Line2D([j + 0.5, j + 0.5], [2, 2.55]))
    else:
        axis.add_line(lines.Line2D([j + 0.5, j + 0.5], [0, -0.55]))


def plot_switch_shared_right(axis, i, j):
    '''
    Helper to plot a switch closing the shared line to the right.

    :param axis: Figure to add conductance to.
    :param i: X-Coordinate of the neuron circuit.
    :param j: Y-Coordiante of the neuron circuit.
    '''
    if 1 - i == 0:
        axis.add_line(lines.Line2D([j + 0.5, j + 1.5], [2.55, 2.55]))
    else:
        axis.add_line(lines.Line2D([j + 0.5, j + 1.5], [-0.55, -0.55]))


plt.rcParams['hatch.linewidth'] = 5
plt.rcParams['hatch.color'] = 'blue'


def plot_grid(*, limits, title="", caption="", step="",
              coordinate_system, dictionary=None, directory="", markings=None,
              plot_ids=True):
    '''
    Plots a coordinate system that visualizes the placement of
    multicompartment neurons on a 2d-grid.

    :param limits: Limits of the part of the grid that is plotted.
    :param title: Title of the plot.
    :param caption: Caption of the plot.
    :param step: Step of the placement. Used for naming files but not plotted.
    :param coordinate_system: Grid that is plotted.
    :param dictionary: Mapping of compartment-ids to plotted names of
     compartments.
    :param directory: Where the plot should be stored.
    :param markings: Mapping of compartment-ids to coloring of the cells.
    :param plot_ids: Whether compartment names should be plotted. If this is
     disable only connections are plotted.
    '''
    cols_lower = limits[0]
    cols_upper = limits[1]

    _, axis = plt.subplots(figsize=((cols_upper - cols_lower), 10))

    # Create grid of boxes
    for i in range(2):
        for j in range(cols_lower, cols_upper):
            color = 'none'
            # Change color for marked compartments
            if markings is not None and coordinate_system.coordinate_system[
                    1 - i][j].compartment in markings:
                color = markings[coordinate_system
                                 .coordinate_system[1 - i][j].compartment]
            rect = plt.Rectangle((j, i), 1, 1, edgecolor='black',
                                 facecolor=color, alpha=0.3)
            axis.add_patch(rect)
            if plot_ids:
                if coordinate_system.coordinate_system[1 - i][j].compartment\
                        is not None:
                    comp_id = coordinate_system.coordinate_system[1 - i][j]\
                        .compartment
                    label = dictionary[comp_id] if dictionary is not None else\
                        comp_id.value()
                    axis.text(j + 0.5, i + 0.5,
                              label,
                              va='center', ha='center')
            if (coordinate_system.coordinate_system[1 - i][j]
                    .switch_right):
                plot_switch_right(axis, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .switch_top_bottom):
                plot_switch_top_bottom(axis, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .switch_circuit_shared):
                plot_switch_shared(axis, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .switch_circuit_shared_conductance):
                plot_switch_shared_conductance(axis, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .switch_shared_right):
                plot_switch_shared_right(axis, i, j)

    # Print caption
    axis.text((cols_upper - cols_lower) / 2 + cols_lower,
              -1, caption, va='center', ha='center')

    plt.axis('off')
    axis.set_xlim(cols_lower - 1, cols_upper + 1)
    axis.set_ylim(-1, 2 + 1)
    axis.set_aspect(1)
    axis.set_title(title)
    axis.grid(True)
    plt.savefig(directory + title.replace(" ", "_") + "_CoordinatePlot_"
                + str(step) + ".png", dpi=300)
    plt.close()


def get_cap_compartment(
        lower: int = 0, upper: int = 0
) -> Tuple[grenade.Compartment, grenade.Compartment.ParameterSpace]:
    '''
    Generate a compartment with a capacitance.

    :param lower: Lower limit of parameter space.
    :param upper: Upper limit of parameter space.
    :return: Compartment and its parameter space.
    '''
    comp = grenade.Compartment()

    param_interval = grenade.ParameterIntervalDouble(lower, upper)
    param_space_mech = grenade.MechanismCapacitance\
        .ParameterSpace(param_interval)

    mechanism = grenade.MechanismCapacitance()
    mechanism_on_comp = comp.add(mechanism)

    param_space_comp = grenade.Compartment.ParameterSpace()
    param_space_comp.mechanisms.set(mechanism_on_comp, param_space_mech)  # pylint: disable=no-member

    return comp, param_space_comp


def get_syn_compartment(
) -> Tuple[grenade.Compartment, grenade.Compartment.ParameterSpace]:
    '''
    Generate a compartment with a synaptic input (and a capacitance)

    :return: Compartment and its parameter space.
    '''
    comp = grenade.Compartment()

    # Capacitance
    param_interval_cap = grenade.ParameterIntervalDouble(0, 11)
    param_space_cap = grenade.MechanismCapacitance.ParameterSpace(
        param_interval_cap)

    mech_cap = grenade.MechanismCapacitance()
    cap_on_comp = comp.add(mech_cap)

    # Synaptic input
    param_interval_syn = grenade.ParameterIntervalDouble(0, 7)

    param_space_syn = grenade.MechanismSynapticInputCurrent.ParameterSpace(
        param_interval_syn, param_interval_syn)
    mech_syn = grenade.MechanismSynapticInputCurrent()
    syn_on_comp = comp.add(mech_syn)

    # Parameter space
    param_space_comp = grenade.Compartment.ParameterSpace()
    param_space_comp.mechanisms.set(cap_on_comp, param_space_cap)  # pylint: disable=no-member
    param_space_comp.mechanisms.set(syn_on_comp, param_space_syn)  # pylint: disable=no-member

    return comp, param_space_comp


def get_compartment_ids(
        coordinate_system: grenade.CoordinateSystem
) -> Set[grenade.CompartmentOnNeuron]:
    """
    Extract all compartment IDs from the given coordinate_system.
    :param coordinate_system: Coordinate system.
    :return: Set of compartment IDs.
    """
    comp_list = []
    for row in coordinate_system.coordinate_system:
        for circuit in row:
            comp_list.append(circuit.compartment)
    comp_set = set(comp_list)
    comp_set.remove(None)
    return comp_set


class SwTestPygrenadeVxMulticompartmentPlacement(unittest.TestCase):
    save_plot = False

    @classmethod
    def assert_valid(cls, coordinate_system: grenade.CoordinateSystem):
        '''
        Check if a valid neuron can be constructed from the given
            coordinate system.

        We check that all compartments are connected.

        :param coordinate_system: coordinate system from which a neuron is
            constructed.
        '''
        neuron_placed, _ = coordinate_system.construct_neuron()
        cls.assertTrue(neuron_placed.compartments_connected(),
                       "Placment result yields a neuron which is "
                       "not fully connected.")

    def test_single_compartment_one(self):
        neuron, resources = neuron_and_res_from_edgelist([])

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)
        # Validation not possible since only single circuit is allocated.

        limits = [120, 160]
        title = "Neuron single compartment with single neuron circuit"

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")

    def test_multiple_neuron_circuits(self):
        """
        Test a single comaprtment which is made up of several neuron
            circuits.
        """

        compartment, parameter_space = get_cap_compartment(0, 11)
        neuron, resources = neuron_and_res_from_edgelist(
            [],
            {0: compartment},
            {0: parameter_space})

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

        limits = [120, 160]
        title = "Neuron single compartment with multiple neuron circuits"

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")

    def test_two_connected(self):
        comp_small, param_space_small = get_cap_compartment()
        comp_big, param_space_big = get_cap_compartment(0, 11)

        neuron, resources = neuron_and_res_from_edgelist(
            [(0, 1)],
            {0: comp_small,
             1: comp_big},
            {0: param_space_small,
             1: param_space_big})

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

        limits = [120, 160]
        title = "Neuron with two connected compartments"

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")

    def test_compartment_chain(self):
        neuron = grenade.Neuron()

        comp_small, param_space_small = get_cap_compartment()
        comp_big, param_space_big = get_cap_compartment(0, 11)

        neuron, resources = neuron_and_res_from_edgelist(
            [(0, 1),
             (1, 2),
             (2, 3)],
            {0: comp_small,
             1: comp_big,
             2: comp_big,
             3: comp_small},
            {0: param_space_small,
             1: param_space_big,
             2: param_space_big,
             3: param_space_small})

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

        title = "Neuron with linearly chained compartments"
        limits = [120, 160]

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")

    def test_synaptic_input(self):
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_syn_compartment()
        neuron, neuron_parameter_space = neuron_from_edgelist(
            [],
            {0: compartment},
            {0: parameter_space})

        synaptic_input_a = grenade.SynapticInputEnvironmentCurrent(
            True, grenade.NumberTopBottom(1200, 0, 257))
        environment.add(grenade.CompartmentOnNeuron(0), synaptic_input_a)

        resources.add_config(neuron, neuron_parameter_space, environment)

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

        title = "Neuron with synatpic input form both lanes"
        limits = [120, 160]

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")

    def test_many_neighbors(self):
        '''
        Test that the algorithm can handle a compartment with many neighbors.
        '''
        connections = [(0, 1),
                       (0, 2),
                       (0, 3),
                       (0, 4)]
        neuron, resources = neuron_and_res_from_edgelist(connections)

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

        title = "Compartment with many neighbors"
        limits = [120, 160]

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")

    def test_seven_connections(self):
        connections = [(0, 1),
                       (0, 2),
                       (0, 3),
                       (0, 4),
                       (0, 5),
                       (0, 6),
                       (0, 7)]
        neuron, resources = neuron_and_res_from_edgelist(connections)

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

        title = "Neuron with central compartment that changes shape"
        limits = [120, 160]

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")

    def test_combination(self):
        connections = [(0, 1),
                       (1, 2),
                       (1, 3),
                       (3, 4),
                       (4, 5),
                       (1, 6),
                       (1, 7),
                       (5, 8),
                       (5, 9),
                       (5, 10),
                       (5, 11),
                       (5, 12)]
        neuron, resources = neuron_and_res_from_edgelist(connections)

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

        title = "Neuron with multiple connected branches"
        limits = [120, 160]

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")

    def test_pyramid(self):
        connections = [(0, 1),
                       (1, 2),
                       (0, 3),
                       (0, 4),
                       (0, 5),
                       (0, 6),
                       (6, 7),
                       (7, 8),
                       (8, 9),
                       (9, 10),
                       (9, 11),
                       (6, 12),
                       (12, 13),
                       (12, 14)]
        neuron, resources = neuron_and_res_from_edgelist(connections)

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

        title = "Neuron pyramidal shape"
        limits = [120, 160]

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")

    def test_purkinje(self):
        # compartments are numbered from left to right in a depth first
        # fashion
        connections = [(0, 1),
                       (1, 2),
                       (2, 3),
                       (3, 4),
                       (4, 5),
                       (5, 6),
                       (5, 7),
                       (3, 8),
                       (8, 9),
                       (9, 10),
                       (10, 11),
                       (10, 12),
                       (9, 13),
                       (8, 14),
                       (2, 15),
                       (15, 16),
                       (15, 17),
                       (1, 18)]
        neuron, resources = neuron_and_res_from_edgelist(connections)

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm.run(
            grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

    def test_many_branches(self):
        """
        Test pyramidal neuron from literature.

        This test implements the representation of a pyramidal neuron
        in figure 5c (middle panel) from Wybo, 2021:
        https://doi.org/10.7554/eLife.60936
        """
        # compartments are numbered from left to right in a depth first
        # fashion
        connections = [(0, 1),
                       (1, 2),
                       (2, 3),
                       (1, 4),
                       (4, 5),
                       (0, 6),
                       (0, 7),
                       (7, 8),
                       (8, 9),
                       (8, 10),
                       (7, 11),
                       (0, 12),
                       (12, 13),
                       (13, 14),
                       (14, 15),
                       (14, 16),
                       (13, 17),
                       (12, 18),
                       (18, 19),
                       (19, 20),
                       (20, 21),
                       (20, 22),
                       (18, 23),
                       (0, 24),
                       (24, 25),
                       (25, 26),
                       (25, 27),
                       (24, 28)]
        neuron, resources = neuron_and_res_from_edgelist(connections)

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm.run(
            grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

    def test_spiny_dendrite(self):
        comp_spine = grenade.Compartment()

        # Parameter
        param_interval_syn = grenade.ParameterIntervalDouble(0, 7)
        param_interval_cap = grenade.ParameterIntervalDouble(0, 1)

        parameter_space_a = grenade.MechanismSynapticInputCurrent\
            .ParameterSpace(param_interval_syn, param_interval_syn)

        parameter_space_b = grenade.MechanismCapacitance\
            .ParameterSpace(param_interval_cap)

        # Mechansims
        mechanism_capacitance = grenade.MechanismCapacitance()
        mechanism_synaptic_input = grenade.MechanismSynapticInputCurrent()

        # Add Mechanisms to compartments
        syn_on_spine = comp_spine.add(mechanism_synaptic_input)
        cap_on_spine = comp_spine.add(mechanism_capacitance)

        param_space_spine = grenade.Compartment.ParameterSpace()

        # pylint: disable=locally-disabled, no-member
        param_space_spine.mechanisms.set(syn_on_spine, parameter_space_a)
        param_space_spine.mechanisms.set(cap_on_spine, parameter_space_b)

        resources = grenade.ResourceManager()
        environment = grenade.Environment()
        syn_input = grenade.SynapticInputEnvironmentCurrent(
            True, grenade.NumberTopBottom(5, 0, 5))

        chain_length = 5
        spine_positions = [1, 3]

        compartments = {}
        parameter_spaces = {}

        connections = [(n, n + 1) for n in range(chain_length - 1)]
        for n_spine, spine_pos in enumerate(spine_positions):
            comp_idx = chain_length + n_spine
            compartments[comp_idx] = comp_spine
            parameter_spaces[comp_idx] = param_space_spine
            connections.append((spine_pos, comp_idx))
            environment.add(grenade.CompartmentOnNeuron(comp_idx), syn_input)

        neuron, neuron_parameter_space = neuron_from_edgelist(
            connections, compartments, parameter_spaces)

        resources.add_config(neuron, neuron_parameter_space, environment)

        placement_algorithm = grenade.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm.run(
            grenade.CoordinateSystem(), neuron, resources)
        self.assert_valid(placement_result.coordinate_system)

        title = "spiny_dendrite"
        limits = [120, 135]

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result.coordinate_system)
        if not placement_result.finished:
            print("Placement failed.")


if __name__ == "__main__":
    pygrenade_vx.logger.default_config(
        level=pygrenade_vx.logger.LogLevel.TRACE)
    log = pygrenade_vx.logger.get("MC.Placement.Ruleset")

    unittest.main()
