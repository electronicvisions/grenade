#!/usr/bin/env python
import unittest
from dlens_vx_v2 import halco, hxcomm
import pygrenade_vx as grenade


class HwTestPygrenadeVx(unittest.TestCase):
    def run_network_graph(self, enable_spikes=True, enable_v=False):
        network_builder = grenade.network.NetworkBuilder()

        ext_pop = grenade.network.ExternalPopulation(256)

        neurons = [halco.AtomicNeuronOnDLS(coord, halco.NeuronRowOnDLS.top)
                   for coord in halco.iter_all(halco.NeuronColumnOnDLS)]
        int_pop = grenade.network.Population(neurons, enable_spikes, enable_v)

        ext_pop_descr = network_builder.add(ext_pop)
        int_pop_descr = network_builder.add(int_pop)

        connections = []
        for i in range(256):
            connections.append(grenade.network.Projection.Connection(i, i, 63))
        proj = grenade.network.Projection(
            grenade.network.Projection.ReceptorType.excitatory,
            connections,
            ext_pop_descr,
            int_pop_descr
        )

        network_builder.add(proj)

        network = network_builder.done()

        routing_result = grenade.network.build_routing(network)

        network_graph = grenade.network.build_network_graph(network, routing_result)

        config = grenade.ChipConfig()

        inputs = grenade.IODataMap()
        inputs.spike_events[network_graph.event_input_vertex] = [[]]

        with hxcomm.ManagedConnection() as connection:
            outputs = grenade.network.run(connection, config, network_graph, inputs)

        if enable_spikes:
            self.assertEqual(len(outputs.spike_event_output[network_graph.event_output_vertex]), 1)

        if enable_v:
            self.assertEqual(len(outputs.madc_samples[network_graph.madc_sample_output_vertex]), 1)

    def test_run_network_graph(self):
        for enable_spikes in [True, False]:
            for enable_v in [False, True]:
                self.run_network_graph(enable_spikes, enable_v)


if __name__ == "__main__":
    unittest.main()
