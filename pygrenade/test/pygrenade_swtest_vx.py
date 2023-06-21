#!/usr/bin/env python
import unittest
from dlens_vx_v3 import halco
import pygrenade_vx.network as grenade


class SwTestPygrenadeVx(unittest.TestCase):
    def test_build_network_graph(self):
        network_builder = grenade.NetworkBuilder()

        ext_pop = grenade.ExternalSourcePopulation(256)

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

        routing_result = grenade.build_routing(network)

        network_graph = grenade.build_network_graph(network, routing_result)

        self.assertEqual(network_graph.event_input_vertex, 0)


if __name__ == "__main__":
    unittest.main()
