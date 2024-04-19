# !/usr/bin/env python
# pylint: disable=too-many-locals, too-many-statements
import unittest
from dlens_vx_v3 import halco
import pygrenade_vx.network as grenade
import pygrenade_vx
import matplotlib.pyplot as plt
from matplotlib import lines


def plot_switch_shared_conductance(ax, i, j):
    if 1 - i == 0:
        ax.add_line(lines.Line2D([j + 0.5, j + 0.5], [2, 2.1]))
        ax.add_line(lines.Line2D([j + 0.5, j + 0.6], [2.1, 2.1]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.15, 2.1]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.15, 2.2]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.25, 2.2]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.25, 2.3]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.35, 2.3]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.35, 2.4]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [2.45, 2.4]))

        ax.add_line(lines.Line2D([j + 0.5, j + 0.4], [2.45, 2.45]))
        ax.add_line(lines.Line2D([j + 0.5, j + 0.5], [2.45, 2.55]))
    else:
        ax.add_line(lines.Line2D([j + 0.5, j + 0.5], [0, -0.1]))
        ax.add_line(lines.Line2D([j + 0.5, j + 0.6], [-0.1, -0.1]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.15, -0.1]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.15, -0.2]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.25, -0.2]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.25, -0.3]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.35, -0.3]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.35, -0.4]))
        ax.add_line(lines.Line2D([j + 0.4, j + 0.6], [-0.45, -0.4]))

        ax.add_line(lines.Line2D([j + 0.5, j + 0.4], [-0.45, -0.45]))
        ax.add_line(lines.Line2D([j + 0.5, j + 0.5], [-0.45, -0.55]))


def plot_switch_top_bottom(ax, i, j):
    if 1 - i == 0:
        ax.add_line(lines.Line2D([j + 0.5, j + 0.5], [1, 1.2]))
    else:
        ax.add_line(lines.Line2D([j + 0.5, j + 0.5], [1, 0.8]))


def plot_switch_right(ax, i, j):
    ax.add_line(lines.Line2D([j + 0.8, j + 1.2], [i + 0.5, i + 0.5]))


def plot_switch_shared(ax, i, j):
    if 1 - i == 0:
        ax.add_line(lines.Line2D([j + 0.5, j + 0.5], [2, 2.55]))
    else:
        ax.add_line(lines.Line2D([j + 0.5, j + 0.5], [0, -0.55]))


def plot_switch_shared_right(ax, i, j):
    if 1 - i == 0:
        ax.add_line(lines.Line2D([j + 0.5, j + 1.5], [2.55, 2.55]))
    else:
        ax.add_line(lines.Line2D([j + 0.5, j + 1.5], [-0.55, -0.55]))


def plot_grid(limits, text, coordinate_system, step, dictionary):
    cols_lower = limits[0]
    cols_upper = limits[1]
    title = text[0]
    input_string = text[1]

    _, ax = plt.subplots(figsize=((cols_upper - cols_lower), 10))

    # Create grid of boxes
    for i in range(2):
        for j in range(cols_lower, cols_upper):
            rect = plt.Rectangle((j, i), 1, 1,
                                 edgecolor='black', facecolor='none')
            ax.add_patch(rect)
            ax.text(j + 0.5, i + 0.5,
                    dictionary[coordinate_system
                               .coordinate_system[1 - i][j].compartment],
                    va='center', ha='center')
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_right):
                plot_switch_right(ax, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_top_bottom):
                plot_switch_top_bottom(ax, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_circuit_shared):
                plot_switch_shared(ax, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_circuit_shared_conductance):
                plot_switch_shared_conductance(ax, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_shared_right):
                plot_switch_shared_right(ax, i, j)

    # Print efficiency or comment
    ax.text((cols_upper - cols_lower) / 2 + cols_lower,
            -1, input_string, va='center', ha='center')

    plt.axis('off')
    ax.set_xlim(cols_lower - 1, cols_upper + 1)
    ax.set_ylim(-1, 2 + 1)
    ax.set_aspect(1)
    # Print title
    ax.set_title(title)
    ax.grid(True)
    plt.savefig("Multicompartment_Placement_Plots/" + title
                + "_Plot" + str(step) + ".png",
                dpi=500)
    plt.close()


def plot_grid_no_ids(limits, text, coordinate_system, step):
    cols_lower = limits[0]
    cols_upper = limits[1]
    title = text[0]
    input_string = text[1]

    _, ax = plt.subplots(figsize=((cols_upper - cols_lower), 10))

    # Create grid of boxes
    for i in range(2):
        for j in range(cols_lower, cols_upper):
            rect = plt.Rectangle((j, i), 1, 1,
                                 edgecolor='black',
                                 facecolor='none')
            ax.add_patch(rect)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .compartment != grenade.CompartmentOnNeuron()):
                ax.text(j + 0.5, i + 0.5, "X", va='center', ha='center')
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_right):
                plot_switch_right(ax, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_top_bottom):
                plot_switch_top_bottom(ax, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_circuit_shared):
                plot_switch_shared(ax, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_circuit_shared_conductance):
                plot_switch_shared_conductance(ax, i, j)
            if (coordinate_system.coordinate_system[1 - i][j]
                    .neuron_circuit_config.switch_shared_right):
                plot_switch_shared_right(ax, i, j)

    # Print efficiency or comment
    ax.text((cols_upper - cols_lower) / 2 + cols_lower, -1,
            input_string, va='center', ha='center')

    # Print algorithm step
    ax.text((cols_upper - cols_lower) / 2 + cols_lower, 2.5,
            'Algorithm Step: ' + str(step), va='center', ha='center')

    plt.axis('off')
    ax.set_xlim(cols_lower - 1, cols_upper + 1)
    ax.set_ylim(-1, 2 + 1)
    ax.set_aspect(1)
    # Print title
    ax.set_title(title)
    ax.grid(True)
    plt.savefig("Multicompartment_Placement_Plots/" + title
                + "_Plot" + str(step) + ".png")
    plt.close()


class SwTestPygrenadeVx(unittest.TestCase):
    def build_network_graph(self):
        network_builder = grenade.NetworkBuilder()

        ext_pop = grenade.ExternalSourcePopulation(
            [grenade.ExternalSourcePopulation.Neuron()] * 256)

        neurons = [grenade.Population.Neuron(halco.LogicalNeuronOnDLS(
            halco.LogicalNeuronCompartments({
                halco.CompartmentOnLogicalNeuron():
                [halco.AtomicNeuronOnLogicalNeuron()]}),
            halco.AtomicNeuronOnDLS(coord, halco.NeuronRowOnDLS.top)),
            {halco.CompartmentOnLogicalNeuron(): grenade.Population.Neuron
             .Compartment(grenade.Population.Neuron.Compartment.SpikeMaster(
                 0, True),
             [{grenade.Receptor(grenade.Receptor.ID(),
               grenade.Receptor.Type.excitatory)}])})
            for coord in halco.iter_all(halco.NeuronColumnOnDLS)]
        int_pop = grenade.Population(neurons)

        ext_pop_descr = network_builder.add(ext_pop)
        int_pop_descr = network_builder.add(int_pop)

        madc_recording = grenade.MADCRecording()
        madc_recording_neurons = [grenade.MADCRecording.Neuron()]
        madc_recording_neurons[0].coordinate.population = int_pop_descr
        madc_recording.neurons = madc_recording_neurons
        network_builder.add(madc_recording)

        connections = []
        for i in range(256):
            connections.append(grenade.Projection.Connection(
                (i, halco.CompartmentOnLogicalNeuron()),
                (i, halco.CompartmentOnLogicalNeuron()),
                grenade.Projection.Connection.Weight(63)))
        proj = grenade.Projection(
            grenade.Receptor(
                grenade.Receptor.ID(), grenade.Receptor.Type.excitatory),
            connections,
            ext_pop_descr,
            int_pop_descr
        )

        network_builder.add(proj)

        network = network_builder.done()

        routing_result = grenade.routing.PortfolioRouter()(network)

        network_graph = grenade.build_network_graph(network, routing_result)

        self.assertEqual(network_graph.graph_translation.execution_instances[
            pygrenade_vx.common.ExecutionInstanceID()].event_input_vertex, 0)

    def test_multicompartment_neuron(self):
        save_plot = False

        def single_compartment_one(compartments: list):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a = compartments[2]

            compartment_a_on_neuron = neuron.add_compartment(compartment_a)

            resources.add_config(neuron, environment)

            placement_algorithm = pygrenade_vx.network\
                .PlacementAlgorithmRuleset()
            placement_result = placement_algorithm\
                .run(grenade.CoordinateSystem(), neuron, resources)

            plot_limits = [120, 160]
            plot_text = ["Neuron single compartment"
                         + "with single neuron circuit", ""]
            dictionary = {compartment_a_on_neuron: "A",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(plot_limits, plot_text,
                          placement_result.coordinate_system, 0, dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def single_compartment_multiple(compartments: list):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a = compartments[0]

            compartment_a_on_neuron = neuron.add_compartment(compartment_a)

            resources.add_config(neuron, environment)

            placement_algorithm = pygrenade_vx.network\
                .PlacementAlgorithmRuleset()
            placement_result = placement_algorithm\
                .run(grenade.CoordinateSystem(), neuron, resources)

            plot_limits = [120, 160]
            plot_text = ["Neuron single compartment"
                         + "with multiple neuron circuit", ""]
            dictionary = {compartment_a_on_neuron: "A",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(plot_limits, plot_text,
                          placement_result.coordinate_system, 0, dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def two_connected(compartments: list):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a = compartments[0]
            compartment_b = compartments[2]

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

            plot_limits = [120, 160]
            plot_text = ["Neuron with two connected compartments", ""]
            dictionary = {compartment_a_on_neuron: "A",
                          compartment_b_on_neuron: "B",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(plot_limits, plot_text,
                          placement_result.coordinate_system, 0, dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def compartment_chain(compartments: list):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a = compartments[0]
            compartment_b = compartments[2]

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

            plot_limits = [120, 160]
            plot_text = ["Neuron with chained compartments", ""]
            dictionary = {compartment_a_on_neuron: "A",
                          compartment_b_on_neuron: "B",
                          compartment_c_on_neuron: "C",
                          compartment_d_on_neuron: "D",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(plot_limits, plot_text,
                          placement_result.coordinate_system, 0, dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def synaptic_input(compartments: list):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a = compartments[1]

            compartment_a_on_neuron = neuron.add_compartment(compartment_a)

            synaptic_input_a = pygrenade_vx.network\
                .SynapticInputEnvironmentCurrent(
                    True, grenade.NumberTopBottom(1200, 0, 257))
            environment.add(compartment_a_on_neuron, synaptic_input_a)

            resources.add_config(neuron, environment)

            placement_algorithm = pygrenade_vx.network\
                .PlacementAlgorithmRuleset()
            placement_result = placement_algorithm\
                .run(grenade.CoordinateSystem(), neuron, resources)

            plot_limits = [120, 160]
            plot_text = ["Neuron with synaptic input from both lanes", ""]
            dictionary = {compartment_a_on_neuron: "A",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(plot_limits, plot_text,
                          placement_result.coordinate_system, 0, dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def change_shape(compartments: list):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a = compartments[2]

            compartment_a_on_neuron = neuron.add_compartment(compartment_a)
            compartment_b_on_neuron = neuron.add_compartment(compartment_a)
            compartment_c_on_neuron = neuron.add_compartment(compartment_a)
            compartment_d_on_neuron = neuron.add_compartment(compartment_a)
            compartment_e_on_neuron = neuron.add_compartment(compartment_a)

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

            plot_limits = [120, 160]
            plot_text = ["Neuron with central compartment"
                         + " that changes shape", ""]
            dictionary = {compartment_a_on_neuron: "A",
                          compartment_b_on_neuron: "B",
                          compartment_c_on_neuron: "C",
                          compartment_d_on_neuron: "D",
                          compartment_e_on_neuron: "E",
                          grenade.CompartmentOnNeuron(): " "}

            if placement_result.finished and save_plot:
                plot_grid(plot_limits, plot_text,
                          placement_result.coordinate_system, 0, dictionary)
            if not placement_result.finished:
                print("Placement failed.")

            synaptic_input_a = pygrenade_vx\
                .network\
                .SynapticInputEnvironmentCurrent(
                    True, grenade.NumberTopBottom(1200, 0, 257))
            environment.add(compartment_a_on_neuron, synaptic_input_a)

        def seven_connections(compartments: list):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a = compartments[2]

            compartment_a_on_neuron = neuron.add_compartment(compartment_a)
            compartment_b_on_neuron = neuron.add_compartment(compartment_a)
            compartment_c_on_neuron = neuron.add_compartment(compartment_a)
            compartment_d_on_neuron = neuron.add_compartment(compartment_a)
            compartment_e_on_neuron = neuron.add_compartment(compartment_a)
            compartment_f_on_neuron = neuron.add_compartment(compartment_a)
            compartment_g_on_neuron = neuron.add_compartment(compartment_a)
            compartment_h_on_neuron = neuron.add_compartment(compartment_a)

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

            plot_limits = [120, 160]
            plot_text = ["Neuron with central compartment"
                         + " that changes shape", ""]
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
                plot_grid(plot_limits, plot_text,
                          placement_result.coordinate_system, 0, dictionary)
            if not placement_result.finished:
                print("Placement failed.")

        def combination(compartments: list):
            neuron = grenade.Neuron()
            resources = grenade.ResourceManager()
            environment = grenade.Environment()

            compartment_a = compartments[2]

            compartment_a_on_neuron = neuron.add_compartment(compartment_a)
            compartment_b_on_neuron = neuron.add_compartment(compartment_a)
            compartment_c_on_neuron = neuron.add_compartment(compartment_a)
            compartment_d_on_neuron = neuron.add_compartment(compartment_a)
            compartment_e_on_neuron = neuron.add_compartment(compartment_a)
            compartment_f_on_neuron = neuron.add_compartment(compartment_a)
            compartment_g_on_neuron = neuron.add_compartment(compartment_a)
            compartment_h_on_neuron = neuron.add_compartment(compartment_a)
            compartment_i_on_neuron = neuron.add_compartment(compartment_a)
            compartment_j_on_neuron = neuron.add_compartment(compartment_a)
            compartment_k_on_neuron = neuron.add_compartment(compartment_a)
            compartment_l_on_neuron = neuron.add_compartment(compartment_a)
            compartment_m_on_neuron = neuron.add_compartment(compartment_a)

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

            plot_limits = [120, 160]
            plot_text = ["Neuron with central"
                         + " compartment that changes shape", ""]
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
                plot_grid(plot_limits, plot_text,
                          placement_result.coordinate_system, 0, dictionary)
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

        # Final compartments
        compartments = [compartment_a, compartment_b, compartment_c]

        # Test of different neurons
        single_compartment_one(compartments)
        single_compartment_multiple(compartments)
        two_connected(compartments)
        compartment_chain(compartments)
        synaptic_input(compartments)
        change_shape(compartments)
        seven_connections(compartments)
        combination(compartments)


if __name__ == "__main__":
    unittest.main()
