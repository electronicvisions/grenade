#!/usr/bin/env python
import unittest
from dlens_vx_v2 import halco, hxcomm, sta
import pygrenade_vx as grenade


class HwTestPygrenadeVx(unittest.TestCase):
    # pylint: disable=too-many-locals
    def run_network_graph(self, enable_spikes=True, enable_v=False):
        network_builder = grenade.NetworkBuilder()

        ext_pop = grenade.ExternalPopulation(256)

        neurons = [halco.AtomicNeuronOnDLS(coord, halco.NeuronRowOnDLS.top)
                   for coord in halco.iter_all(halco.NeuronColumnOnDLS)]
        record_spikes = [False] * len(neurons)
        record_spikes[0] = enable_spikes
        int_pop = grenade.Population(neurons, record_spikes)

        ext_pop_descr = network_builder.add(ext_pop)
        int_pop_descr = network_builder.add(int_pop)

        if enable_v:
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

        config = grenade.ChipConfig()

        inputs = grenade.IODataMap()
        inputs.spike_events = {network_graph.event_input_vertex: [[]]}

        with hxcomm.ManagedConnection() as connection:
            init, _ = sta.generate(sta.DigitalInit())
            sta.run(connection, init.done())
            outputs = grenade.run(connection, config, network_graph, inputs)

        if enable_spikes:
            self.assertEqual(len(
                outputs.spike_event_output[network_graph.event_output_vertex]
            ), 1)

        if enable_v:
            self.assertEqual(len(
                outputs.madc_samples[network_graph.madc_sample_output_vertex]
            ), 1)

    def test_run_network_graph(self):
        for enable_spikes in [True, False]:
            for enable_v in [False, True]:
                self.run_network_graph(enable_spikes, enable_v)

    @staticmethod
    def test_run_empty_graph():
        network_builder = grenade.NetworkBuilder()
        network = network_builder.done()

        network_graph = grenade.build_network_graph(
            network, grenade.build_routing(network))
        inputs = grenade.IODataMap()
        with hxcomm.ManagedConnection() as connection:
            init, _ = sta.generate(sta.DigitalInit())
            sta.run(connection, init.done())
            grenade.run(
                connection,
                grenade.ChipConfig(),
                network_graph,
                inputs)


if __name__ == "__main__":
    unittest.main()
