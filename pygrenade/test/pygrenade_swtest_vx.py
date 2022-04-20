#!/usr/bin/env python
import unittest
from dlens_vx_v3 import halco
import pygrenade_vx as grenade


class SwTestPygrenadeVx(unittest.TestCase):
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
