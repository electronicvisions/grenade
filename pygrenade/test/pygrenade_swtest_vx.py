#!/usr/bin/env python
import unittest
from dlens_vx_v3 import lola, hal, halco
import pygrenade_vx as grenade


class SwTestPygrenadeVx(unittest.TestCase):
    # pylint: disable=no-member,unsubscriptable-object
    def test_hemisphere_config(self):
        config = grenade.HemisphereConfig()
        self.assertIsInstance(config.synapse_matrix, lola.SynapseMatrix)
        self.assertIsInstance(
            config.synapse_driver_block[
                halco.SynapseDriverOnSynapseDriverBlock()],
            hal.SynapseDriverConfig)

        other = grenade.HemisphereConfig()
        self.assertEqual(config, other)

        other.synapse_matrix.weights[0][0] = 15
        self.assertNotEqual(config, other)

    def test_chip_config(self):
        config = grenade.ChipConfig()
        self.assertIsInstance(config.hemispheres[0], grenade.HemisphereConfig)

        other = grenade.ChipConfig()
        self.assertEqual(config, other)

        other.hemispheres[0].synapse_matrix.weights[0][0] = 15
        self.assertNotEqual(config, other)

    def test_build_network_graph(self):
        network_builder = grenade.NetworkBuilder()

        ext_pop = grenade.ExternalPopulation(256)

        neurons = [halco.AtomicNeuronOnDLS(coord, halco.NeuronRowOnDLS.top)
                   for coord in halco.iter_all(halco.NeuronColumnOnDLS)]
        record_spikes = [False] * len(neurons)
        record_spikes[0] = True
        int_pop = grenade.Population(neurons, record_spikes)

        ext_pop_descr = network_builder.add(ext_pop)
        int_pop_descr = network_builder.add(int_pop)

        madc_recording = grenade.MADCRecording()
        madc_recording.population = int_pop_descr
        madc_recording.index = 0
        network_builder.add(madc_recording)

        connections = []
        for i in range(256):
            connections.append(grenade.Projection.Connection(i, i, 63))
        proj = grenade.Projection(
            grenade.Projection.ReceptorType.excitatory,
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
