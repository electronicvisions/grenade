# !/usr/bin/env python
# pylint: disable=too-many-locals, too-many-statements, too-many-lines
from typing import Set, Tuple
import unittest

import matplotlib.pyplot as plt
from matplotlib import lines

import pygrenade_vx.network as grenade
import pygrenade_vx
from _pygrenade_vx_network import CoordinateSystem, CompartmentOnNeuron


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
              coordinate_system, dictionary, directory="", markings=None,
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
                    axis.text(j + 0.5, i + 0.5,
                              dictionary[coordinate_system
                                         .coordinate_system[1 - i][j]
                                         .compartment],
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
    plt.savefig(directory + title + "_CoordinatePlot_"
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
        coordinate_system: CoordinateSystem) -> Set[CompartmentOnNeuron]:
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

    def test_single_compartment_one(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_cap_compartment()

        compartment_a_on_neuron = neuron.add_compartment(compartment)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()

        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: parameter_space}

        resources.add_config(neuron, neuron_parameter_space, environment)

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)

        limits = [120, 160]
        title = "Neuron single compartment with single neuron circuit"
        dictionary = {compartment_a_on_neuron: "A"}

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))

    def test_multiple_neuron_circuits(self):
        """
        Test a single comaprtment which is made up of several neuron
            circuits.
        """
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_cap_compartment(0, 11)

        compartment_a_on_neuron = neuron.add_compartment(compartment)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: parameter_space}

        resources.add_config(neuron, neuron_parameter_space, environment)

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)

        limits = [120, 160]
        title = "Neuron single compartment with multiple neuron circuits"
        dictionary = {compartment_a_on_neuron: "A"}

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))

    def test_two_connected(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        comp_small, param_space_small = get_cap_compartment()
        comp_big, param_space_big = get_cap_compartment(0, 11)

        compartment_a_on_neuron = neuron.add_compartment(comp_small)
        compartment_b_on_neuron = neuron.add_compartment(comp_big)

        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_b_on_neuron,
            grenade.CompartmentConnectionConductance())

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: param_space_small,
            compartment_b_on_neuron: param_space_big}

        resources.add_config(neuron, neuron_parameter_space, environment)

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)

        limits = [120, 160]
        title = "Neuron with two connected compartments"
        dictionary = {compartment_a_on_neuron: "A",
                      compartment_b_on_neuron: "B"}

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))

    def test_compartment_chain(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        comp_small, param_space_small = get_cap_compartment()
        comp_big, param_space_big = get_cap_compartment(0, 11)

        compartment_a_on_neuron = neuron.add_compartment(comp_small)
        compartment_b_on_neuron = neuron.add_compartment(comp_big)
        compartment_c_on_neuron = neuron.add_compartment(comp_big)
        compartment_d_on_neuron = neuron.add_compartment(comp_small)

        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_b_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_b_on_neuron,
            compartment_c_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_c_on_neuron,
            compartment_d_on_neuron,
            grenade.CompartmentConnectionConductance())

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: param_space_small,
            compartment_b_on_neuron: param_space_big,
            compartment_c_on_neuron: param_space_big,
            compartment_d_on_neuron: param_space_small}

        resources.add_config(neuron, neuron_parameter_space, environment)

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)

        title = "Neuron with linearly chained compartments"
        limits = [120, 160]
        dictionary = {compartment_a_on_neuron: "A",
                      compartment_b_on_neuron: "B",
                      compartment_c_on_neuron: "C",
                      compartment_d_on_neuron: "D"}

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))

    def test_synaptic_input(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_syn_compartment()

        compartment_a_on_neuron = neuron.add_compartment(compartment)

        synaptic_input_a = pygrenade_vx.network\
            .SynapticInputEnvironmentCurrent(
                True, grenade.NumberTopBottom(1200, 0, 257))
        environment.add(compartment_a_on_neuron, synaptic_input_a)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: parameter_space}

        resources.add_config(neuron, neuron_parameter_space, environment)

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)

        title = "Neuron with synatpic input form both lanes"
        limits = [120, 160]
        dictionary = {compartment_a_on_neuron: "A"}

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))

    def test_change_shape(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_cap_compartment()

        compartment_a_on_neuron = neuron.add_compartment(compartment)
        compartment_b_on_neuron = neuron.add_compartment(compartment)
        compartment_c_on_neuron = neuron.add_compartment(compartment)
        compartment_d_on_neuron = neuron.add_compartment(compartment)
        compartment_e_on_neuron = neuron.add_compartment(compartment)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: parameter_space,
            compartment_b_on_neuron: parameter_space,
            compartment_c_on_neuron: parameter_space,
            compartment_d_on_neuron: parameter_space,
            compartment_e_on_neuron: parameter_space}

        resources.add_config(neuron, neuron_parameter_space, environment)

        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_b_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_c_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_d_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_e_on_neuron,
            grenade.CompartmentConnectionConductance())

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)

        title = "Neuron with central compartment that changes shape"
        limits = [120, 160]
        dictionary = {compartment_a_on_neuron: "A",
                      compartment_b_on_neuron: "B",
                      compartment_c_on_neuron: "C",
                      compartment_d_on_neuron: "D",
                      compartment_e_on_neuron: "E"}

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        synaptic_input_a = pygrenade_vx\
            .network\
            .SynapticInputEnvironmentCurrent(
                True, grenade.NumberTopBottom(1200, 0, 257))
        environment.add(compartment_a_on_neuron, synaptic_input_a)

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))

    def test_seven_connections(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_cap_compartment()

        compartment_a_on_neuron = neuron.add_compartment(compartment)
        compartment_b_on_neuron = neuron.add_compartment(compartment)
        compartment_c_on_neuron = neuron.add_compartment(compartment)
        compartment_d_on_neuron = neuron.add_compartment(compartment)
        compartment_e_on_neuron = neuron.add_compartment(compartment)
        compartment_f_on_neuron = neuron.add_compartment(compartment)
        compartment_g_on_neuron = neuron.add_compartment(compartment)
        compartment_h_on_neuron = neuron.add_compartment(compartment)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: parameter_space,
            compartment_b_on_neuron: parameter_space,
            compartment_c_on_neuron: parameter_space,
            compartment_d_on_neuron: parameter_space,
            compartment_e_on_neuron: parameter_space,
            compartment_f_on_neuron: parameter_space,
            compartment_g_on_neuron: parameter_space,
            compartment_h_on_neuron: parameter_space}

        resources.add_config(neuron, neuron_parameter_space, environment)

        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_b_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_c_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_d_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_e_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_f_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_g_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_h_on_neuron,
            grenade.CompartmentConnectionConductance())

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)

        title = "Neuron with central compartment that changes shape"
        limits = [120, 160]
        dictionary = {compartment_a_on_neuron: "A",
                      compartment_b_on_neuron: "B",
                      compartment_c_on_neuron: "C",
                      compartment_d_on_neuron: "D",
                      compartment_e_on_neuron: "E",
                      compartment_f_on_neuron: "F",
                      compartment_g_on_neuron: "G",
                      compartment_h_on_neuron: "H"}

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))

    def test_combination(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_cap_compartment()

        compartment_a_on_neuron = neuron.add_compartment(compartment)
        compartment_b_on_neuron = neuron.add_compartment(compartment)
        compartment_c_on_neuron = neuron.add_compartment(compartment)
        compartment_d_on_neuron = neuron.add_compartment(compartment)
        compartment_e_on_neuron = neuron.add_compartment(compartment)
        compartment_f_on_neuron = neuron.add_compartment(compartment)
        compartment_g_on_neuron = neuron.add_compartment(compartment)
        compartment_h_on_neuron = neuron.add_compartment(compartment)
        compartment_i_on_neuron = neuron.add_compartment(compartment)
        compartment_j_on_neuron = neuron.add_compartment(compartment)
        compartment_k_on_neuron = neuron.add_compartment(compartment)
        compartment_l_on_neuron = neuron.add_compartment(compartment)
        compartment_m_on_neuron = neuron.add_compartment(compartment)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: parameter_space,
            compartment_b_on_neuron: parameter_space,
            compartment_c_on_neuron: parameter_space,
            compartment_d_on_neuron: parameter_space,
            compartment_e_on_neuron: parameter_space,
            compartment_f_on_neuron: parameter_space,
            compartment_g_on_neuron: parameter_space,
            compartment_h_on_neuron: parameter_space,
            compartment_i_on_neuron: parameter_space,
            compartment_j_on_neuron: parameter_space,
            compartment_k_on_neuron: parameter_space,
            compartment_l_on_neuron: parameter_space,
            compartment_m_on_neuron: parameter_space}

        resources.add_config(neuron, neuron_parameter_space, environment)

        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_b_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_b_on_neuron,
            compartment_c_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_b_on_neuron,
            compartment_d_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_d_on_neuron,
            compartment_e_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_e_on_neuron,
            compartment_f_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_b_on_neuron,
            compartment_g_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_b_on_neuron,
            compartment_h_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_f_on_neuron,
            compartment_i_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_f_on_neuron,
            compartment_j_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_f_on_neuron,
            compartment_k_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_f_on_neuron,
            compartment_l_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_f_on_neuron,
            compartment_m_on_neuron,
            grenade.CompartmentConnectionConductance())

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)

        title = "Neuron with multiple connected branches"
        limits = [120, 160]
        dictionary = {compartment_a_on_neuron: "A",
                      compartment_b_on_neuron: "B",
                      compartment_c_on_neuron: "C",
                      compartment_d_on_neuron: "D",
                      compartment_e_on_neuron: "E",
                      compartment_f_on_neuron: "F",
                      compartment_g_on_neuron: "G",
                      compartment_h_on_neuron: "H",
                      compartment_i_on_neuron: "I",
                      compartment_j_on_neuron: "J",
                      compartment_k_on_neuron: "K",
                      compartment_l_on_neuron: "L",
                      compartment_m_on_neuron: "M"}

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))

    def test_pyramid(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_cap_compartment()

        compartment_a_on_neuron = neuron.add_compartment(compartment)
        compartment_b_on_neuron = neuron.add_compartment(compartment)
        compartment_c_on_neuron = neuron.add_compartment(compartment)
        compartment_d_on_neuron = neuron.add_compartment(compartment)
        compartment_e_on_neuron = neuron.add_compartment(compartment)
        compartment_f_on_neuron = neuron.add_compartment(compartment)
        compartment_g_on_neuron = neuron.add_compartment(compartment)
        compartment_h_on_neuron = neuron.add_compartment(compartment)
        compartment_i_on_neuron = neuron.add_compartment(compartment)
        compartment_j_on_neuron = neuron.add_compartment(compartment)
        compartment_k_on_neuron = neuron.add_compartment(compartment)
        compartment_l_on_neuron = neuron.add_compartment(compartment)
        compartment_m_on_neuron = neuron.add_compartment(compartment)
        compartment_n_on_neuron = neuron.add_compartment(compartment)
        compartment_o_on_neuron = neuron.add_compartment(compartment)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: parameter_space,
            compartment_b_on_neuron: parameter_space,
            compartment_c_on_neuron: parameter_space,
            compartment_d_on_neuron: parameter_space,
            compartment_e_on_neuron: parameter_space,
            compartment_f_on_neuron: parameter_space,
            compartment_g_on_neuron: parameter_space,
            compartment_h_on_neuron: parameter_space,
            compartment_i_on_neuron: parameter_space,
            compartment_j_on_neuron: parameter_space,
            compartment_k_on_neuron: parameter_space,
            compartment_l_on_neuron: parameter_space,
            compartment_m_on_neuron: parameter_space,
            compartment_n_on_neuron: parameter_space,
            compartment_o_on_neuron: parameter_space}

        resources.add_config(neuron, neuron_parameter_space, environment)

        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_b_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_b_on_neuron,
            compartment_c_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_d_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_e_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_f_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_g_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_g_on_neuron,
            compartment_h_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_h_on_neuron,
            compartment_i_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_h_on_neuron,
            compartment_j_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_j_on_neuron,
            compartment_k_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_j_on_neuron,
            compartment_l_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_g_on_neuron,
            compartment_m_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_m_on_neuron,
            compartment_n_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_m_on_neuron,
            compartment_o_on_neuron,
            grenade.CompartmentConnectionConductance())

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        placement_result = placement_algorithm\
            .run(grenade.CoordinateSystem(), neuron, resources)

        title = "Neuron pyramidal shape"
        limits = [120, 160]
        dictionary = {compartment_a_on_neuron: "A",
                      compartment_b_on_neuron: "B",
                      compartment_c_on_neuron: "C",
                      compartment_d_on_neuron: "D",
                      compartment_e_on_neuron: "E",
                      compartment_f_on_neuron: "F",
                      compartment_g_on_neuron: "G",
                      compartment_h_on_neuron: "H",
                      compartment_i_on_neuron: "I",
                      compartment_j_on_neuron: "J",
                      compartment_k_on_neuron: "K",
                      compartment_l_on_neuron: "L",
                      compartment_m_on_neuron: "M",
                      compartment_n_on_neuron: "N",
                      compartment_o_on_neuron: "O"}

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))

    def test_purkinje(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_cap_compartment()

        compartment_a_on_neuron = neuron.add_compartment(compartment)
        compartment_b_on_neuron = neuron.add_compartment(compartment)
        compartment_c_on_neuron = neuron.add_compartment(compartment)
        compartment_d_on_neuron = neuron.add_compartment(compartment)
        compartment_e_on_neuron = neuron.add_compartment(compartment)
        compartment_f_on_neuron = neuron.add_compartment(compartment)
        compartment_g_on_neuron = neuron.add_compartment(compartment)
        compartment_h_on_neuron = neuron.add_compartment(compartment)
        compartment_i_on_neuron = neuron.add_compartment(compartment)
        compartment_j_on_neuron = neuron.add_compartment(compartment)
        compartment_k_on_neuron = neuron.add_compartment(compartment)
        compartment_l_on_neuron = neuron.add_compartment(compartment)
        compartment_m_on_neuron = neuron.add_compartment(compartment)
        compartment_n_on_neuron = neuron.add_compartment(compartment)
        compartment_o_on_neuron = neuron.add_compartment(compartment)
        compartment_p_on_neuron = neuron.add_compartment(compartment)
        compartment_q_on_neuron = neuron.add_compartment(compartment)
        compartment_r_on_neuron = neuron.add_compartment(compartment)
        compartment_s_on_neuron = neuron.add_compartment(compartment)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_a_on_neuron: parameter_space,
            compartment_b_on_neuron: parameter_space,
            compartment_c_on_neuron: parameter_space,
            compartment_d_on_neuron: parameter_space,
            compartment_e_on_neuron: parameter_space,
            compartment_f_on_neuron: parameter_space,
            compartment_g_on_neuron: parameter_space,
            compartment_h_on_neuron: parameter_space,
            compartment_i_on_neuron: parameter_space,
            compartment_j_on_neuron: parameter_space,
            compartment_k_on_neuron: parameter_space,
            compartment_l_on_neuron: parameter_space,
            compartment_m_on_neuron: parameter_space,
            compartment_n_on_neuron: parameter_space,
            compartment_o_on_neuron: parameter_space,
            compartment_p_on_neuron: parameter_space,
            compartment_q_on_neuron: parameter_space,
            compartment_r_on_neuron: parameter_space,
            compartment_s_on_neuron: parameter_space}

        resources.add_config(neuron, neuron_parameter_space, environment)

        neuron.add_compartment_connection(
            compartment_a_on_neuron,
            compartment_b_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_b_on_neuron,
            compartment_c_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_b_on_neuron,
            compartment_d_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_d_on_neuron,
            compartment_e_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_e_on_neuron,
            compartment_f_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_e_on_neuron,
            compartment_g_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_d_on_neuron,
            compartment_h_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_h_on_neuron,
            compartment_i_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_i_on_neuron,
            compartment_j_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_j_on_neuron,
            compartment_k_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_j_on_neuron,
            compartment_l_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_h_on_neuron,
            compartment_m_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_m_on_neuron,
            compartment_n_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_m_on_neuron,
            compartment_o_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_o_on_neuron,
            compartment_p_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_o_on_neuron,
            compartment_q_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_q_on_neuron,
            compartment_r_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_q_on_neuron,
            compartment_s_on_neuron,
            grenade.CompartmentConnectionConductance())

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        with self.assertRaises(RuntimeError):
            placement_algorithm.run(
                grenade.CoordinateSystem(), neuron, resources)

    def test_many_branches(self):
        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()

        compartment, parameter_space = get_cap_compartment(0, 11)

        compartment_aa_on_neuron = neuron.add_compartment(compartment)
        compartment_ab_on_neuron = neuron.add_compartment(compartment)
        compartment_ac_on_neuron = neuron.add_compartment(compartment)
        compartment_ad_on_neuron = neuron.add_compartment(compartment)
        compartment_ae_on_neuron = neuron.add_compartment(compartment)
        compartment_af_on_neuron = neuron.add_compartment(compartment)
        compartment_ag_on_neuron = neuron.add_compartment(compartment)
        compartment_ah_on_neuron = neuron.add_compartment(compartment)
        compartment_ai_on_neuron = neuron.add_compartment(compartment)
        compartment_aj_on_neuron = neuron.add_compartment(compartment)
        compartment_ak_on_neuron = neuron.add_compartment(compartment)
        compartment_al_on_neuron = neuron.add_compartment(compartment)
        compartment_am_on_neuron = neuron.add_compartment(compartment)
        compartment_an_on_neuron = neuron.add_compartment(compartment)
        compartment_ao_on_neuron = neuron.add_compartment(compartment)
        compartment_ap_on_neuron = neuron.add_compartment(compartment)
        compartment_aq_on_neuron = neuron.add_compartment(compartment)
        compartment_ar_on_neuron = neuron.add_compartment(compartment)
        compartment_as_on_neuron = neuron.add_compartment(compartment)
        compartment_at_on_neuron = neuron.add_compartment(compartment)
        compartment_au_on_neuron = neuron.add_compartment(compartment)
        compartment_av_on_neuron = neuron.add_compartment(compartment)
        compartment_aw_on_neuron = neuron.add_compartment(compartment)
        compartment_ax_on_neuron = neuron.add_compartment(compartment)
        compartment_ay_on_neuron = neuron.add_compartment(compartment)
        compartment_az_on_neuron = neuron.add_compartment(compartment)
        compartment_ba_on_neuron = neuron.add_compartment(compartment)
        compartment_bb_on_neuron = neuron.add_compartment(compartment)
        compartment_bc_on_neuron = neuron.add_compartment(compartment)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = {
            compartment_aa_on_neuron: parameter_space,
            compartment_ab_on_neuron: parameter_space,
            compartment_ac_on_neuron: parameter_space,
            compartment_ad_on_neuron: parameter_space,
            compartment_ae_on_neuron: parameter_space,
            compartment_af_on_neuron: parameter_space,
            compartment_ag_on_neuron: parameter_space,
            compartment_ah_on_neuron: parameter_space,
            compartment_ai_on_neuron: parameter_space,
            compartment_aj_on_neuron: parameter_space,
            compartment_ak_on_neuron: parameter_space,
            compartment_al_on_neuron: parameter_space,
            compartment_am_on_neuron: parameter_space,
            compartment_an_on_neuron: parameter_space,
            compartment_ao_on_neuron: parameter_space,
            compartment_ap_on_neuron: parameter_space,
            compartment_aq_on_neuron: parameter_space,
            compartment_ar_on_neuron: parameter_space,
            compartment_as_on_neuron: parameter_space,
            compartment_at_on_neuron: parameter_space,
            compartment_au_on_neuron: parameter_space,
            compartment_av_on_neuron: parameter_space,
            compartment_aw_on_neuron: parameter_space,
            compartment_ax_on_neuron: parameter_space,
            compartment_ay_on_neuron: parameter_space,
            compartment_az_on_neuron: parameter_space,
            compartment_ba_on_neuron: parameter_space,
            compartment_bb_on_neuron: parameter_space,
            compartment_bc_on_neuron: parameter_space}

        resources.add_config(neuron, neuron_parameter_space, environment)

        neuron.add_compartment_connection(
            compartment_aa_on_neuron,
            compartment_ab_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_aa_on_neuron,
            compartment_ac_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_ac_on_neuron,
            compartment_ad_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_ad_on_neuron,
            compartment_ae_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_ac_on_neuron,
            compartment_af_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_af_on_neuron,
            compartment_ag_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_aa_on_neuron,
            compartment_ah_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_ah_on_neuron,
            compartment_ai_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_ah_on_neuron,
            compartment_aj_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_aj_on_neuron,
            compartment_ak_on_neuron,
            grenade.CompartmentConnectionConductance())
        neuron.add_compartment_connection(
            compartment_aj_on_neuron,
            compartment_al_on_neuron,
            grenade.CompartmentConnectionConductance())

        placement_algorithm = pygrenade_vx.network\
            .PlacementAlgorithmRuleset()
        with self.assertRaises(RuntimeError):
            placement_algorithm.run(
                grenade.CoordinateSystem(), neuron, resources)

    def test_spiny_dendrite(self):
        comp_branch, param_space_branch = get_cap_compartment(0, 1)
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

        neuron = grenade.Neuron()
        resources = grenade.ResourceManager()
        environment = grenade.Environment()
        syn_input = pygrenade_vx.network\
            .SynapticInputEnvironmentCurrent(
                True, grenade.NumberTopBottom(5, 0, 5))

        chain_length = 5
        spine_positions = [1, 3]

        compartments = {}
        dictionary = {}
        branch = []
        for pos in range(chain_length):
            comp = neuron.add_compartment(comp_branch)
            compartments[comp] = param_space_branch
            dictionary[comp] = f"C{pos}"
            if pos > 0:
                neuron.add_compartment_connection(
                    branch[-1], comp,
                    grenade.CompartmentConnectionConductance())
            branch.append(comp)

        for pos in spine_positions:
            comp = neuron.add_compartment(comp_spine)
            compartments[comp] = param_space_spine
            dictionary[comp] = f"S{pos}"
            neuron.add_compartment_connection(
                branch[pos], comp, grenade.CompartmentConnectionConductance())
            environment.add(comp, syn_input)

        neuron_parameter_space = grenade.Neuron.ParameterSpace()
        neuron_parameter_space.compartments = compartments

        resources.add_config(neuron, neuron_parameter_space, environment)

        placement_algorithm = pygrenade_vx.network.PlacementAlgorithmRuleset()
        placement_result = placement_algorithm.run(
            grenade.CoordinateSystem(), neuron, resources)

        title = "spiny_dendrite"
        limits = [120, 135]

        if placement_result.finished and self.save_plot:
            plot_grid(limits=limits, title=title,
                      coordinate_system=placement_result
                      .coordinate_system, dictionary=dictionary)
        if not placement_result.finished:
            print("Placement failed.")

        self.assertEqual(
            get_compartment_ids(placement_result.coordinate_system),
            set(dictionary.keys()))


if __name__ == "__main__":
    pygrenade_vx.logger.default_config(
        level=pygrenade_vx.logger.LogLevel.TRACE)
    log = pygrenade_vx.logger.get("MC.Placement.Ruleset")

    unittest.main()
