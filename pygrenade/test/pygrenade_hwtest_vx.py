#!/usr/bin/env python
import unittest
from dlens_vx_v3 import halco, hxcomm, sta, hal, lola, logger
import pygrenade_vx as grenade

logger_grenade = logger.get("grenade")
logger.set_loglevel(logger_grenade, logger.LogLevel.INFO)


class HwTestPygrenadeVx(unittest.TestCase):
    # pylint: disable=too-many-locals
    def run_network_graph(self, enable_spikes=True, enable_v=False):
        network_builder = grenade.network.placed_atomic.NetworkBuilder()

        ext_pop = grenade.network.placed_atomic.ExternalPopulation(256)

        neurons = [halco.AtomicNeuronOnDLS(coord, halco.NeuronRowOnDLS.top)
                   for coord in halco.iter_all(halco.NeuronColumnOnDLS)]
        record_spikes = [False] * len(neurons)
        record_spikes[0] = enable_spikes
        int_pop = grenade.network.placed_atomic.Population(
            neurons, record_spikes)

        ext_pop_descr = network_builder.add(ext_pop)
        int_pop_descr = network_builder.add(int_pop)

        if enable_v:
            madc_recording = grenade.network.placed_atomic.MADCRecording()
            madc_recording.population = int_pop_descr
            madc_recording.index = 0
            network_builder.add(madc_recording)

        connections = []
        for i in range(256):
            connections.append(grenade.network.placed_atomic.Projection
                               .Connection(i, i, 63))
        proj = grenade.network.placed_atomic.Projection(
            grenade.network.placed_atomic.Projection.ReceptorType.excitatory,
            connections,
            ext_pop_descr,
            int_pop_descr
        )

        network_builder.add(proj)

        network = network_builder.done()

        routing_result = grenade.network.placed_atomic.build_routing(network)

        network_graph = grenade.network.placed_atomic.build_network_graph(
            network, routing_result)

        config = lola.Chip()

        inputs = grenade.network.placed_atomic.InputGenerator(
            network_graph).done()

        with hxcomm.ManagedConnection() as connection:
            init, _ = sta.generate(sta.DigitalInit())
            sta.run(connection, init.done())
            outputs = grenade.network.placed_atomic.run(
                connection, config, network_graph, inputs)

        if enable_spikes or enable_v:
            self.assertEqual(outputs.batch_size(), 1)

    def test_run_network_graph(self):
        for enable_spikes in [True, False]:
            for enable_v in [False, True]:
                self.run_network_graph(enable_spikes, enable_v)

    @staticmethod
    def test_run_empty_graph():
        network_builder = grenade.network.placed_atomic.NetworkBuilder()
        network = network_builder.done()

        network_graph = grenade.network.placed_atomic.build_network_graph(
            network, grenade.network.placed_atomic.build_routing(network))
        inputs = grenade.signal_flow.IODataMap()
        with hxcomm.ManagedConnection() as connection:
            init, _ = sta.generate(sta.DigitalInit())
            sta.run(connection, init.done())
            grenade.network.placed_atomic.run(
                connection,
                lola.Chip(),
                network_graph,
                inputs)

    @staticmethod
    def test_cadc_readout(enable_spikes: bool = True):
        batch_size = 2
        int_pop_size = 3
        ext_pop_size = 3
        network_builder = grenade.network.placed_atomic.NetworkBuilder()

        ext_pop = grenade.network.placed_atomic.ExternalPopulation(
            ext_pop_size)

        neurons = [halco.AtomicNeuronOnDLS(coord, halco.NeuronRowOnDLS.top)
                   for coord
                   in halco.iter_all(halco.NeuronColumnOnDLS)][:int_pop_size]
        record_spikes = [enable_spikes] * len(neurons)
        int_pop = grenade.network.placed_atomic.Population(
            neurons, record_spikes)

        int_pop_descr = network_builder.add(int_pop)
        ext_pop_descr = network_builder.add(ext_pop)

        cadc_recording = grenade.network.placed_atomic.CADCRecording()
        recorded_neurons = []
        for nrn_id in range(int_pop_size):
            recorded_neurons.append(
                grenade.network.placed_atomic.CADCRecording.Neuron(
                    int_pop_descr, nrn_id,
                    lola.AtomicNeuron.Readout.Source.membrane))
        cadc_recording.neurons = recorded_neurons
        network_builder.add(cadc_recording)

        connections = []
        for i in range(ext_pop_size):
            for j in range(int_pop_size):
                connections.append(grenade.network.placed_atomic.Projection
                                   .Connection(i, j, 63))
        proj = grenade.network.placed_atomic.Projection(
            grenade.network.placed_atomic.Projection.ReceptorType.excitatory,
            connections,
            ext_pop_descr,
            int_pop_descr
        )
        network_builder.add(proj)

        network = network_builder.done()

        routing_result = grenade.network.placed_atomic.build_routing(network)

        network_graph = grenade.network.placed_atomic.build_network_graph(
            network, routing_result)

        config = lola.Chip()

        input_generator = grenade.network.placed_atomic.InputGenerator(
            network_graph, batch_size)
        times = [None for _ in range(batch_size)]
        for i in range(batch_size):
            times[i] = [[] for _ in range(ext_pop_size)]
        input_generator.add(times, ext_pop_descr)
        inputs = input_generator.done()
        inputs.runtime = [{
            grenade.signal_flow.ExecutionInstance():
            int(hal.Timer.Value.fpga_clock_cycles_per_us) * 100}] * batch_size

        with hxcomm.ManagedConnection() as connection:
            init, _ = sta.generate(sta.ExperimentInit())
            sta.run(connection, init.done())
            result_map = grenade.network.placed_atomic.run(
                connection, config, network_graph, inputs)

        hw_spike_times = grenade.network.placed_atomic.extract_neuron_spikes(
            result_map, network_graph)
        print(hw_spike_times)
        hw_cadc_samples = grenade.network.placed_atomic.extract_cadc_samples(
            result_map, network_graph)
        print(hw_cadc_samples)


if __name__ == "__main__":
    unittest.main()
