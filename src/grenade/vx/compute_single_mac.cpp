#include "grenade/vx/compute_single_mac.h"

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/event.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/single_chip_execution_instance_manager.h"
#include "hate/math.h"

#include <algorithm>

namespace grenade::vx {

Graph::vertex_descriptor ComputeSingleMAC::insert_synram(
    Graph& graph,
    Weights const& weights,
    RowModes const& row_modes,
    coordinate::ExecutionInstance const& instance,
    halco::hicann_dls::vx::v2::HemisphereOnDLS const& hemisphere,
    Graph::vertex_descriptor const crossbar_input_vertex)
{
	using namespace haldls::vx::v2;
	using namespace halco::hicann_dls::vx::v2;

	auto const x_size = weights.at(0).size();
	auto const y_size = weights.size();

	vertex::SynapseArrayView::Columns columns;
	for (size_t o = 0; o < x_size; ++o) {
		auto const column = SynapseOnSynapseRow(o);
		columns.push_back(column);
	}

	// configure crossbar and padi busses
	auto const num_padi_bus = std::min(
	    hate::math::round_up_integer_division(y_size, 2),
	    static_cast<size_t>(PADIBusOnPADIBusBlock::size));
	std::vector<Input> padi_bus_vertices;
	for (size_t i = 0; i < num_padi_bus; ++i) {
		CrossbarNodeOnDLS coordinate(
		    CrossbarInputOnDLS(i + 8), CrossbarOutputOnDLS(i + (hemisphere.toEnum() * 4)));
		CrossbarNode config;
		config.set_mask(CrossbarNode::neuron_label_type(0b1 << 13));
		config.set_target(CrossbarNode::neuron_label_type((hemisphere.toEnum() << 13)));
		vertex::CrossbarNode crossbar_node(coordinate, config);
		auto const v2 = graph.add(crossbar_node, instance, {crossbar_input_vertex});
		padi_bus_vertices.push_back(graph.add(
		    vertex::PADIBus(vertex::PADIBus::Coordinate(
		        PADIBusOnPADIBusBlock(i), hemisphere.toPADIBusBlockOnDLS())),
		    instance, {v2}));
	}

	vertex::SynapseArrayView::Labels labels(y_size);
	vertex::SynapseArrayView::Rows rows;
	size_t i = 0;
	for (auto& l : labels) {
		SynapseRowOnSynram const sr(i);
		rows.push_back(sr);
		l.insert(l.begin(), x_size, lola::vx::v2::SynapseMatrix::Label((i % 2) << 5));
		i++;
	}

	// add synapse driver
	auto const num_syndrv = y_size / 2;
	auto const rest_syndrv = y_size % 2;
	std::vector<Input> synapse_driver_vertices;
	for (size_t i = 0; i < num_syndrv; ++i) {
		auto const padi_bus_vertex = padi_bus_vertices.at(i % PADIBusOnPADIBusBlock::size);
		vertex::SynapseDriver synapse_driver(
		    SynapseDriverOnDLS(
		        SynapseDriverOnSynapseDriverBlock(i), hemisphere.toSynapseDriverBlockOnDLS()),
		    vertex::SynapseDriver::Config(0b11111), {row_modes.at(i * 2 + 1), row_modes.at(i * 2)});
		synapse_driver_vertices.push_back(graph.add(synapse_driver, instance, {padi_bus_vertex}));
	}
	if (rest_syndrv) {
		auto const padi_bus_vertex = padi_bus_vertices.at(num_syndrv % PADIBusOnPADIBusBlock::size);
		vertex::SynapseDriver synapse_driver(
		    SynapseDriverOnDLS(
		        SynapseDriverOnSynapseDriverBlock(num_syndrv),
		        hemisphere.toSynapseDriverBlockOnDLS()),
		    vertex::SynapseDriver::Config(0b11111),
		    {SynapseDriverConfig::RowMode::disabled, row_modes.at(num_syndrv * 2)});
		synapse_driver_vertices.push_back(graph.add(synapse_driver, instance, {padi_bus_vertex}));
	}
	// add synapse array
	vertex::SynapseArrayView synapse_array(
	    hemisphere.toSynramOnDLS(), rows, columns, weights, labels);
	auto const synapse_array_vertex = graph.add(synapse_array, instance, synapse_driver_vertices);
	// add neurons
	vertex::NeuronView::Columns nrns;
	for (size_t o = 0; o < x_size; ++o) {
		auto const nrn = NeuronColumnOnDLS(o);
		nrns.push_back(nrn);
	}
	vertex::NeuronView neurons(nrns, hemisphere.toNeuronRowOnDLS());
	auto const v1 = graph.add(neurons, instance, {synapse_array_vertex});
	// add readout
	vertex::CADCMembraneReadoutView readout(columns, hemisphere.toSynramOnDLS());
	auto const v2 = graph.add(readout, instance, {v1});
	// add store
	vertex::DataOutput data_output(ConnectionType::Int8, x_size);
	return graph.add(data_output, instance, {v2});
}

ComputeSingleMAC::ComputeSingleMAC(
    Weights const& weights,
    RowModes const& row_modes,
    ChipConfig const& config,
    size_t num_sends,
    haldls::vx::v2::Timer::Value wait_between_events) :
    m_graph(),
    m_synram_handles(),
    m_output_vertices(),
    m_weights(),
    m_row_modes(),
    m_config_map(),
    m_num_sends(num_sends),
    m_wait_between_events(wait_between_events)
{
	using namespace halco::hicann_dls::vx::v2;

	m_config_map[DLSGlobal()] = config;

	if (std::adjacent_find(weights.begin(), weights.end(), [](auto const& a, auto const& b) {
		    return a.size() != b.size();
	    }) != weights.end()) {
		throw std::runtime_error("Synapse weight matrix is not rectangular.");
	}

	if (weights.size() != row_modes.size()) {
		throw std::runtime_error("Synapse weight matrix row size and row modes size don't match.");
	}

	m_weights = weights;
	m_row_modes = row_modes;

	size_t o_offset = 0;
	size_t i_offset = 0;
	SingleChipExecutionInstanceManager execution_instance_manager;
	auto instance = coordinate::ExecutionInstance();

	vertex::ExternalInput external_input(ConnectionType::DataInputUInt16, 1);
	vertex::DataInput data_input(ConnectionType::CrossbarInputLabel, 1);
	auto last_instance = instance;

	auto input_vertex = m_graph.add(external_input, instance, {});
	Graph::vertex_descriptor crossbar_input_vertex =
	    m_graph.add(data_input, instance, {input_vertex});

	while (o_offset < output_size()) {
		size_t const local_o_size = std::min(NeuronColumnOnDLS::size, output_size() - o_offset);

		i_offset = 0;
		std::vector<Input> local_output_vertices;
		while (i_offset < input_size()) {
			if (instance != last_instance) {
				input_vertex = m_graph.add(external_input, instance, {});
				crossbar_input_vertex = m_graph.add(data_input, instance, {input_vertex});
			}

			size_t const local_i_size = std::min(input_size() - i_offset, SynapseRowOnSynram::size);

			Weights local_weights(local_i_size);
			for (size_t i = 0; i < local_weights.size(); ++i) {
				local_weights.at(i).insert(
				    local_weights.at(i).begin(), m_weights.at(i + i_offset).begin() + o_offset,
				    m_weights.at(i + i_offset).begin() + o_offset + local_o_size);
			}

			RowModes local_row_modes(local_i_size);
			for (size_t i = 0; i < local_weights.size(); ++i) {
				local_row_modes.at(i) = m_row_modes.at(i + i_offset);
			}

			local_output_vertices.push_back(insert_synram(
			    m_graph, local_weights, local_row_modes, instance,
			    execution_instance_manager.get_current_hemisphere(), crossbar_input_vertex));
			m_synram_handles.push_back({input_vertex, local_i_size, i_offset,
			                            execution_instance_manager.get_current_hemisphere()});

			last_instance = instance;
			instance = execution_instance_manager.next();

			i_offset += local_i_size;
		}

		if (local_output_vertices.size() > 1) {
			instance =
			    execution_instance_manager.next_index(); // FIXME we only want to get next index
			// add additions
			// load all data
			vertex::DataInput data_input(ConnectionType::Int8, local_o_size);
			std::vector<Input> local_inputs;
			for (auto const vertex : local_output_vertices) {
				local_inputs.push_back(m_graph.add(data_input, instance, {vertex}));
			}
			// add all data
			vertex::Addition addition(local_o_size);
			auto const v_add = m_graph.add(addition, instance, local_inputs);
			vertex::DataOutput data_output(ConnectionType::Int8, local_o_size);
			auto const v_out = m_graph.add(data_output, instance, {v_add});
			m_output_vertices.push_back(v_out);
		} else {
			std::transform(
			    local_output_vertices.begin(), local_output_vertices.end(),
			    std::back_inserter(m_output_vertices), [](auto const& i) { return i.descriptor; });
		}

		o_offset += local_o_size;
	}
}

size_t ComputeSingleMAC::input_size() const
{
	return m_row_modes.size();
}

size_t ComputeSingleMAC::output_size() const
{
	if (m_row_modes.size()) {
		return m_weights.at(0).size();
	}
	return 0;
}

std::optional<haldls::vx::v2::SpikeLabel> ComputeSingleMAC::get_spike_label(
    halco::hicann_dls::vx::v2::SynapseRowOnDLS const& row, UInt5 const value)
{
	if (value == 0) {
		return std::nullopt;
	}
	haldls::vx::v2::SpikeLabel label;
	label.set_spl1_address(
	    halco::hicann_dls::vx::v2::SPL1Address(row.toSynapseRowOnSynram()
	                                               .toSynapseDriverOnSynapseDriverBlock()
	                                               .toPADIBusOnPADIBusBlock()));
	label.set_synapse_label(lola::vx::v2::SynapseMatrix::Label(
	    ((row.toSynapseRowOnSynram() % 2) << 5) | UInt5((UInt5::max - value) + 1)));
	label.set_row_select_address(
	    haldls::vx::v2::PADIEvent::RowSelectAddress(row.toSynapseRowOnSynram()
	                                                    .toSynapseDriverOnSynapseDriverBlock()
	                                                    .toSynapseDriverOnPADIBus()));
	label.set_neuron_label(halco::hicann_dls::vx::v2::NeuronLabel(
	    label.get_neuron_label() |
	    (static_cast<uint16_t>(row.toSynramOnDLS().toHemisphereOnDLS()) << 13)));
	return label;
}

DataMap ComputeSingleMAC::generate_input_events(
    Activations const& inputs,
    std::vector<SynramHandle> const& synram_handles,
    size_t const num_sends,
    haldls::vx::v2::Timer::Value const wait_between_events)
{
	DataMap data_map;

	size_t const batch_size = inputs.size();
	for (size_t i = 0; i < synram_handles.size(); ++i) {
		auto const& local_synram_handle = synram_handles.at(i);
		auto const input_vertex = local_synram_handle.input_vertex;
		auto const input_size = local_synram_handle.input_size;
		auto const input_offset = local_synram_handle.input_offset;
		auto const hemisphere = local_synram_handle.hemisphere;

		data_map.spike_events[input_vertex].resize(batch_size);
		for (size_t batch = 0; batch < batch_size; ++batch) {
			auto& events = data_map.spike_events[input_vertex].at(batch);
			TimedSpike::Time time(0);
			// pack all events from one hemisphere after one another
			for (size_t i = 0; i < num_sends; ++i) {
				for (size_t j = 0; j < input_size; ++j) {
					auto const label = get_spike_label(
					    halco::hicann_dls::vx::v2::SynapseRowOnDLS(
					        halco::hicann_dls::vx::v2::SynapseRowOnSynram(j),
					        hemisphere.toSynramOnDLS()),
					    inputs.at(batch).at(input_offset + j));
					if (label) {
						haldls::vx::v2::SpikePack1ToChip const payload(
						    haldls::vx::v2::SpikePack1ToChip::labels_type{*label});
						events.push_back(TimedSpike{time, payload});
						time = TimedSpike::Time(time + wait_between_events);
					}
				}
			}
		}
	}

	// sort all events from both hemishperes together so that they are sent in pairs for top and
	// bottom hemisphere
	for (auto& events_batch : data_map.spike_events) {
		for (auto& events : events_batch.second) {
			std::sort(events.begin(), events.end(), [](auto const& a, auto const& b) {
				return a.time < b.time;
			});
		}
	}

	return data_map;
}

std::vector<std::vector<Int8>> ComputeSingleMAC::run(
    Activations const& inputs, hxcomm::vx::ConnectionVariant& connection)
{
	using namespace halco::hicann_dls::vx::v2;


	// Construct map of one executor and connect to HW
	JITGraphExecutor::ExecutorMap executors;
	executors.insert(std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection));

	if (inputs.size() == 0) {
		throw std::runtime_error("Provided inputs are empty.");
	}

	// FIXME: inputs.at(i).size() equality check missing
	size_t const batch_entry_size = inputs.front().size();
	for (auto const& batch_entry : inputs) {
		if (batch_entry.size() != batch_entry_size) {
			throw std::runtime_error("Provided batch entries don't share a common size.");
		}
	}

	// fill graph inputs (with UInt5(0))
	if (batch_entry_size != input_size()) {
		throw std::runtime_error("Provided inputs size does not match MAC input size.");
	}
	auto const input_list =
	    generate_input_events(inputs, m_synram_handles, m_num_sends, m_wait_between_events);

	// run Graph with given inputs and return results
	auto const output_activation_map =
	    JITGraphExecutor::run(m_graph, input_list, executors, m_config_map);

	std::vector<std::vector<Int8>> output(output_activation_map.batch_size());
	for (auto& entry : output) {
		entry.resize(output_size());
	}
	for (size_t i = 0; i < output.size(); ++i) {
		size_t o_offset = 0;
		for (auto const vertex : m_output_vertices) {
			auto const& o = output_activation_map.int8.at(vertex).at(i);
			std::copy(o.begin(), o.end(), output.at(i).begin() + o_offset);
			o_offset += o.size();
		}
	}
	return output;
}

} // namespace grenade::vx
