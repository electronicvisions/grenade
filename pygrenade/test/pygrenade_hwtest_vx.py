#!/usr/bin/env python
import unittest
from dlens_vx_v2 import halco, hxcomm, sta, hal, lola, logger
import pygrenade_vx as grenade

logger_grenade = logger.get("grenade")
logger.set_loglevel(logger_grenade, logger.LogLevel.INFO)


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

        config = lola.Chip()

        inputs = grenade.IODataMap()
        inputs.data = \
            {network_graph.event_input_vertex: [[grenade.TimedSpike()]]}

        with hxcomm.ManagedConnection() as connection:
            init, _ = sta.generate(sta.DigitalInit())
            sta.run(connection, init.done())
            outputs = grenade.run(connection, config, network_graph, inputs)

        if enable_spikes:
            self.assertEqual(len(
                outputs.data[network_graph.event_output_vertex]
            ), 1)

        if enable_v:
            self.assertEqual(len(
                outputs.data[network_graph.madc_sample_output_vertex]
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
                lola.Chip(),
                network_graph,
                inputs)

    @staticmethod
    def test_cadc_readout(enable_spikes: bool = True):
        batch_size = 2
        int_pop_size = 3
        ext_pop_size = 3
        network_builder = grenade.NetworkBuilder()

        ext_pop = grenade.ExternalPopulation(ext_pop_size)

        neurons = [halco.AtomicNeuronOnDLS(coord, halco.NeuronRowOnDLS.top)
                   for coord
                   in halco.iter_all(halco.NeuronColumnOnDLS)][:int_pop_size]
        record_spikes = [enable_spikes] * len(neurons)
        int_pop = grenade.Population(neurons, record_spikes)

        int_pop_descr = network_builder.add(int_pop)
        ext_pop_descr = network_builder.add(ext_pop)

        cadc_recording = grenade.CADCRecording()
        recorded_neurons = list()
        for nrn_id in range(int_pop_size):
            recorded_neurons.append(
                grenade.CADCRecording.Neuron(
                    int_pop_descr, nrn_id,
                    lola.AtomicNeuron.Readout.Source.membrane))
        cadc_recording.neurons = recorded_neurons
        network_builder.add(cadc_recording)

        connections = []
        for i in range(ext_pop_size):
            for j in range(int_pop_size):
                connections.append(grenade.Projection.Connection(i, j, 63))
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

        input_generator = grenade.InputGenerator(network_graph, batch_size)
        times = [None for _ in range(batch_size)]
        for i in range(batch_size):
            times[i] = [[] for _ in range(ext_pop_size)]
        input_generator.add(times, ext_pop_descr)
        inputs = input_generator.done()
        inputs.runtime = batch_size * [
            int(hal.Timer.Value.fpga_clock_cycles_per_us) * 100]

        with hxcomm.ManagedConnection() as connection:
            init, _ = sta.generate(sta.ExperimentInit())
            sta.run(connection, init.done())
            result_map = grenade.run(connection, config, network_graph, inputs)

        hw_spike_times = grenade.extract_neuron_spikes(
            result_map, network_graph)
        print(hw_spike_times)
        hw_cadc_samples = grenade.extract_cadc_samples(
            result_map, network_graph)
        print(hw_cadc_samples)


if __name__ == "__main__":
    unittest.main()
