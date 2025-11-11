# !/usr/bin/env python
# pylint: disable=too-many-locals, too-many-statements
import unittest

import matplotlib.pyplot as plt
from matplotlib import lines

import pygrenade_vx.network as grenade
import pygrenade_vx


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
                axis.text(j + 0.5, i + 0.5,
                          dictionary[coordinate_system
                                     .coordinate_system[1 - i][j].compartment],
                          va='center', ha='center')
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_right):
                plot_switch_right(axis, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_top_bottom):
                plot_switch_top_bottom(axis, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_circuit_shared):
                plot_switch_shared(axis, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_circuit_shared_conductance):
                plot_switch_shared_conductance(axis, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_shared_right):
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


class SwTestPygrenadeVxMulticompartmentPlacement(unittest.TestCase):
    def test_multicompartment_neuron(self):
        save_plot = False

        def single_compartment_one(compartment: grenade.Compartment):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a_on_neuron = neuron.add_compartment(compartment)

            resources.add_config(neuron, environment)

            placement_algorithm = pygrenade_vx.network\
                .PlacementAlgorithmRuleset()
            placement_result = placement_algorithm\
                .run(grenade.CoordinateSystem(), neuron, resources)

            limits = [120, 160]
            title = "Neuron single compartment with single neuron circuit"
            dictionary = {compartment_a_on_neuron: "A",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(limits=limits, title=title,
                          coordinate_system=placement_result
                          .coordinate_system, dictionary=dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def single_compartment_multiple(compartment: grenade.Compartment):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a_on_neuron = neuron.add_compartment(compartment)

            resources.add_config(neuron, environment)

            placement_algorithm = pygrenade_vx.network\
                .PlacementAlgorithmRuleset()
            placement_result = placement_algorithm\
                .run(grenade.CoordinateSystem(), neuron, resources)

            limits = [120, 160]
            title = "Neuron single compartment with multiple neuron circuits"
            dictionary = {compartment_a_on_neuron: "A",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(limits=limits, title=title,
                          coordinate_system=placement_result
                          .coordinate_system, dictionary=dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def two_connected(compartment_a: grenade.Compartment,
                          compartment_b: grenade.Compartment):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a_on_neuron = neuron.add_compartment(compartment_a)
            compartment_b_on_neuron = neuron.add_compartment(compartment_b)

            neuron.add_compartment_connection(
                compartment_a_on_neuron,
                compartment_b_on_neuron,
                grenade.CompartmentConnectionConductance())

            resources.add_config(neuron, environment)

            placement_algorithm = pygrenade_vx.network\
                .PlacementAlgorithmRuleset()
            placement_result = placement_algorithm\
                .run(grenade.CoordinateSystem(), neuron, resources)

            limits = [120, 160]
            title = "Neuron with two connected compartments"
            dictionary = {compartment_a_on_neuron: "A",
                          compartment_b_on_neuron: "B",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(limits=limits, title=title,
                          coordinate_system=placement_result
                          .coordinate_system, dictionary=dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def compartment_chain(compartment_a: grenade.Compartment,
                              compartment_b: grenade.Compartment):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a_on_neuron = neuron.add_compartment(compartment_a)
            compartment_b_on_neuron = neuron.add_compartment(compartment_b)
            compartment_c_on_neuron = neuron.add_compartment(compartment_b)
            compartment_d_on_neuron = neuron.add_compartment(compartment_a)

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

            resources.add_config(neuron, environment)

            placement_algorithm = pygrenade_vx.network\
                .PlacementAlgorithmRuleset()
            placement_result = placement_algorithm\
                .run(grenade.CoordinateSystem(), neuron, resources)

            title = "Neuron with linearly chained compartments"
            limits = [120, 160]
            dictionary = {compartment_a_on_neuron: "A",
                          compartment_b_on_neuron: "B",
                          compartment_c_on_neuron: "C",
                          compartment_d_on_neuron: "D",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(limits=limits, title=title,
                          coordinate_system=placement_result
                          .coordinate_system, dictionary=dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def synaptic_input(compartment: grenade.Compartment):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a_on_neuron = neuron.add_compartment(compartment)

            synaptic_input_a = pygrenade_vx.network\
                .SynapticInputEnvironmentCurrent(
                    True, grenade.NumberTopBottom(1200, 0, 257))
            environment.add(compartment_a_on_neuron, synaptic_input_a)

            resources.add_config(neuron, environment)

            placement_algorithm = pygrenade_vx.network\
                .PlacementAlgorithmRuleset()
            placement_result = placement_algorithm\
                .run(grenade.CoordinateSystem(), neuron, resources)

            title = "Neuron with synatpic input form both lanes"
            limits = [120, 160]
            dictionary = {compartment_a_on_neuron: "A",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(limits=limits, title=title,
                          coordinate_system=placement_result
                          .coordinate_system, dictionary=dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def change_shape(compartment: grenade.Compartment):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a_on_neuron = neuron.add_compartment(compartment)
            compartment_b_on_neuron = neuron.add_compartment(compartment)
            compartment_c_on_neuron = neuron.add_compartment(compartment)
            compartment_d_on_neuron = neuron.add_compartment(compartment)
            compartment_e_on_neuron = neuron.add_compartment(compartment)

            resources.add_config(neuron, environment)

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
                          compartment_e_on_neuron: "E",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
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

        def seven_connections(compartment: grenade.Compartment):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a_on_neuron = neuron.add_compartment(compartment)
            compartment_b_on_neuron = neuron.add_compartment(compartment)
            compartment_c_on_neuron = neuron.add_compartment(compartment)
            compartment_d_on_neuron = neuron.add_compartment(compartment)
            compartment_e_on_neuron = neuron.add_compartment(compartment)
            compartment_f_on_neuron = neuron.add_compartment(compartment)
            compartment_g_on_neuron = neuron.add_compartment(compartment)
            compartment_h_on_neuron = neuron.add_compartment(compartment)

            resources.add_config(neuron, environment)

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

            title = "Neuron with central comparmtent that changes shape"
            limits = [120, 160]
            dictionary = {compartment_a_on_neuron: "A",
                          compartment_b_on_neuron: "B",
                          compartment_c_on_neuron: "C",
                          compartment_d_on_neuron: "D",
                          compartment_e_on_neuron: "E",
                          compartment_f_on_neuron: "F",
                          compartment_g_on_neuron: "G",
                          compartment_h_on_neuron: "H",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(limits=limits, title=title,
                          coordinate_system=placement_result
                          .coordinate_system, dictionary=dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def combination(compartment: grenade.Compartment):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

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

            resources.add_config(neuron, environment)

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

            title = "Complex neuron with multiple connected branches"
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
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(limits=limits, title=title,
                          coordinate_system=placement_result
                          .coordinate_system, dictionary=dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        # Compartments
        compartment_a = grenade.Compartment()
        compartment_b = grenade.Compartment()
        compartment_c = grenade.Compartment()

        # Parameter
        parameter_interval_a = grenade.ParameterIntervalDouble(0, 7)
        parameter_interval_b = grenade.ParameterIntervalDouble(0, 11)

        parameterization_a = grenade.MechanismSynapticInputCurrent\
            .ParameterSpace.Parameterization(1, 1)
        parameter_space_a = grenade.MechanismSynapticInputCurrent\
            .ParameterSpace(parameter_interval_a,
                            parameter_interval_a,
                            parameterization_a)

        parameterization_b = grenade.MechanismCapacitance\
            .ParameterSpace.Parameterization(7)
        parameter_space_b = grenade.MechanismCapacitance\
            .ParameterSpace(parameter_interval_b, parameterization_b)

        # Mechansims
        mechanism_capacitance_a = grenade\
            .MechanismCapacitance(parameter_space_b)
        mechanism_capacitance_c = grenade.MechanismCapacitance()
        mechanism_synaptic_input_a = grenade\
            .MechanismSynapticInputCurrent(parameter_space_a)

        # Add Mechanisms to compartments
        compartment_a.add(mechanism_capacitance_a)
        compartment_b.add(mechanism_synaptic_input_a)
        compartment_b.add(mechanism_capacitance_a)
        compartment_c.add(mechanism_capacitance_c)

        # Test of different neurons
        single_compartment_one(compartment_a)
        single_compartment_multiple(compartment_a)
        two_connected(compartment_a, compartment_c)
        compartment_chain(compartment_a, compartment_c)
        synaptic_input(compartment_b)
        change_shape(compartment_c)
        seven_connections(compartment_c)
        combination(compartment_c)


if __name__ == "__main__":
    unittest.main()
