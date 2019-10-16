#include "grenade/vx/compute_single_mac.h"

#include "grenade/vx/config.h"
#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/jit_graph_executor.h"

#include <algorithm>

namespace grenade::vx {

class SingleChipExecutionInstanceManager
{
public:
	SingleChipExecutionInstanceManager() : m_current_hemisphere(), m_current_index() {}

	coordinate::ExecutionInstance next()
	{
		using namespace halco::hicann_dls::vx;
		if (m_current_hemisphere) {
			m_current_index = coordinate::ExecutionIndex(m_current_index + 1);
		}
		m_current_hemisphere = HemisphereOnDLS((m_current_hemisphere + 1) % HemisphereOnDLS::size);
		return coordinate::ExecutionInstance(
		    m_current_index, HemisphereGlobal(m_current_hemisphere, DLSGlobal()));
	}

	coordinate::ExecutionInstance next_index()
	{
		if (m_current_hemisphere == 0) {
			next();
		}
		return next();
	}

private:
	halco::hicann_dls::vx::HemisphereOnDLS m_current_hemisphere;
	coordinate::ExecutionIndex m_current_index;
};


ComputeSingleMAC::ComputeSingleMAC(
    Weights const& weights, RowModes const& row_modes, ChipConfig const& config, size_t num_sends) :
    m_graph(),
    m_input_vertices(),
    m_output_vertices(),
    m_input_sizes(),
    m_input_offsets(),
    m_weights(),
    m_row_modes(),
    m_config_map()
{
	using namespace halco::hicann_dls::vx;

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
	auto instance = execution_instance_manager.next();

	while (o_offset < output_size()) {
		size_t const local_o_size = std::min(NeuronColumnOnDLS::size, output_size() - o_offset);

		i_offset = 0;
		std::vector<Graph::vertex_descriptor> local_output_vertices;
		while (i_offset < input_size()) {
			size_t const local_i_size = std::min(input_size() - i_offset, SynapseRowOnSynram::size);

			vertex::ExternalInput external_input(ConnectionType::DataOutputUInt5, local_i_size);
			vertex::DataInput data_input(ConnectionType::SynapseInputLabel, local_i_size);

			// construct SynapseArrayView on one vertical half
			vertex::SynapseArrayView::synapse_rows_type synapse_rows;
			// fill synapse array view
			for (size_t i = 0; i < local_i_size; ++i) {
				auto const synapse_row = SynapseRowOnSynram(i);
				// all synapse rows
				synapse_rows.push_back(vertex::SynapseArrayView::SynapseRow());
				synapse_rows.back().coordinate = synapse_row;
				std::vector<vertex::SynapseArrayView::SynapseRow::Weight> weights(local_o_size);
				std::copy(
				    m_weights.at(i + i_offset).begin() + o_offset,
				    m_weights.at(i + i_offset).begin() + o_offset + local_o_size, weights.begin());
				synapse_rows.back().weights = weights;
				synapse_rows.back().mode = m_row_modes.at(i + i_offset);
			}
			// construct vertex::SynapseArrayView
			vertex::SynapseArrayView synapses(synapse_rows);
			synapses.num_sends = num_sends;

			vertex::NeuronView::neurons_type nrns;
			for (size_t o = 0; o < local_o_size; ++o) {
				auto const nrn = NeuronColumnOnDLS(o);
				nrns.push_back(nrn);
			}
			vertex::NeuronView neurons(nrns);

			vertex::CADCMembraneReadoutView readout(local_o_size);

			vertex::DataOutput data_output(ConnectionType::Int8, local_o_size);

			auto const v1 = m_graph.add(external_input, instance, {});
			auto const v2 = m_graph.add(data_input, instance, {v1});
			auto const v3 = m_graph.add(synapses, instance, {v2});
			auto const v4 = m_graph.add(neurons, instance, {v3});
			auto const v5 = m_graph.add(readout, instance, {v4});
			auto const v6 = m_graph.add(data_output, instance, {v5});
			m_input_vertices.push_back(v1);
			m_input_sizes.push_back(local_i_size);
			m_input_offsets.push_back(i_offset);
			local_output_vertices.push_back(v6);

			instance = execution_instance_manager.next();

			i_offset += local_i_size;
		}

		if (local_output_vertices.size() > 1) {
			instance =
			    execution_instance_manager.next_index(); // FIXME we only want to get next index
			// add additions
			// load all data
			vertex::DataInput data_input(ConnectionType::Int8, local_o_size);
			std::vector<Graph::vertex_descriptor> local_inputs;
			for (auto const vertex : local_output_vertices) {
				local_inputs.push_back(m_graph.add(data_input, instance, {vertex}));
			}
			// add all data
			vertex::Addition addition(local_o_size, local_inputs.size());
			auto const v_add = m_graph.add(addition, instance, local_inputs);
			vertex::DataOutput data_output(ConnectionType::Int8, local_o_size);
			auto const v_out = m_graph.add(data_output, instance, {v_add});
			m_output_vertices.push_back(v_out);
		} else {
			std::copy(
			    local_output_vertices.begin(), local_output_vertices.end(),
			    std::back_inserter(m_output_vertices));
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

std::vector<Int8> ComputeSingleMAC::run(
    Activations const& inputs, hxcomm::vx::ConnectionVariant& connection)
{
	using namespace halco::hicann_dls::vx;

	// Construct map of one executor and connect to HW
	JITGraphExecutor::ExecutorMap executors;
	executors.insert(std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection));

	// fill graph inputs (with UInt5(0))
	if (inputs.size() != input_size()) {
		throw std::runtime_error("Provided inputs size does not match MAC input size.");
	}

	DataMap input_list;
	size_t i = 0;
	for (auto const vertex : m_input_vertices) {
		std::vector<UInt5> input(m_input_sizes.at(i));
		size_t const i_offset = m_input_offsets.at(i);
		std::copy(
		    inputs.begin() + i_offset, inputs.begin() + i_offset + m_input_sizes.at(i),
		    input.begin());
		input_list.uint5[vertex] = input;
		i++;
	}

	// run Graph with given inputs and return results
	auto const output_activation_map =
	    JITGraphExecutor::run(m_graph, input_list, executors, m_config_map);

	std::vector<Int8> output(output_size());
	size_t o_offset = 0;
	for (auto const vertex : m_output_vertices) {
		auto const& o = output_activation_map.int8.at(vertex);
		std::copy(o.begin(), o.end(), output.begin() + o_offset);
		o_offset += o.size();
	}
	return output;
}

} // namespace grenade::vx
