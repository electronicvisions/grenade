#include "grenade/vx/compute_single_mac.h"

#include "grenade/vx/config.h"
#include "grenade/vx/event.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/range_split.h"
#include "grenade/vx/single_chip_execution_instance_manager.h"
#include "hate/math.h"
#include "hate/timer.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <log4cxx/logger.h>
#include <tbb/parallel_for_each.h>

#include <algorithm>

namespace grenade::vx {

Graph::vertex_descriptor ComputeSingleMAC::insert_synram(
    Graph& graph,
    Weights&& weights,
    RowModes&& row_modes,
    coordinate::ExecutionInstance const& instance,
    halco::hicann_dls::vx::v2::HemisphereOnDLS const& hemisphere,
    Graph::vertex_descriptor const crossbar_input_vertex)
{
	using namespace haldls::vx::v2;
	using namespace halco::hicann_dls::vx::v2;

	auto const x_size = weights.at(0).size();
	auto const y_size = weights.size();

	vertex::SynapseArrayView::Columns columns;
	columns.reserve(x_size);
	for (size_t o = 0; o < x_size; ++o) {
		auto const column = SynapseOnSynapseRow(o);
		columns.push_back(column);
	}

	// configure crossbar and padi busses
	auto const num_padi_bus = std::min(
	    hate::math::round_up_integer_division(y_size, 2),
	    static_cast<size_t>(PADIBusOnPADIBusBlock::size));
	std::vector<Input> padi_bus_vertices;
	padi_bus_vertices.reserve(num_padi_bus);
	for (size_t i = 0; i < num_padi_bus; ++i) {
		CrossbarNodeOnDLS coordinate(
		    CrossbarInputOnDLS(i + 8), CrossbarOutputOnDLS(i + (hemisphere.toEnum() * 4)));
		CrossbarNode config;
		config.set_mask(CrossbarNode::neuron_label_type(0b1 << 13));
		config.set_target(CrossbarNode::neuron_label_type((hemisphere.toEnum() << 13)));
		vertex::CrossbarNode crossbar_node(coordinate, config);
		auto const v1 = graph.add(crossbar_node, instance, {crossbar_input_vertex});
		padi_bus_vertices.push_back(graph.add(
		    vertex::PADIBus(vertex::PADIBus::Coordinate(
		        PADIBusOnPADIBusBlock(i), hemisphere.toPADIBusBlockOnDLS())),
		    instance, {v1}));
	}

	vertex::SynapseArrayView::Labels labels(y_size);
	vertex::SynapseArrayView::Rows rows;
	rows.reserve(y_size);
	size_t i = 0;
	for (auto& l : labels) {
		SynapseRowOnSynram const sr(i);
		rows.push_back(sr);
		l.insert(l.end(), x_size, lola::vx::v2::SynapseMatrix::Label((i % 2) << 5));
		i++;
	}

	// add synapse driver
	auto const num_syndrv = y_size / 2;
	auto const rest_syndrv = y_size % 2;
	std::vector<Input> synapse_driver_vertices;
	synapse_driver_vertices.reserve(num_syndrv);
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
	    hemisphere.toSynramOnDLS(), std::move(rows), columns, std::move(weights),
	    std::move(labels));
	auto const synapse_array_vertex =
	    graph.add(std::move(synapse_array), instance, synapse_driver_vertices);
	// add neurons
	vertex::NeuronView::Columns nrns;
	nrns.reserve(x_size);
	for (size_t o = 0; o < x_size; ++o) {
		auto const nrn = NeuronColumnOnDLS(o);
		nrns.push_back(nrn);
	}
	vertex::NeuronView::EnableResets enable_resets(x_size, true);
	vertex::NeuronView neurons(
	    std::move(nrns), std::move(enable_resets), hemisphere.toNeuronRowOnDLS());
	auto const v1 = graph.add(std::move(neurons), instance, {synapse_array_vertex});
	// add readout
	vertex::CADCMembraneReadoutView readout(std::move(columns), hemisphere.toSynramOnDLS());
	auto const v2 = graph.add(readout, instance, {v1});
	// add store
	vertex::DataOutput data_output(ConnectionType::Int8, x_size);
	return graph.add(data_output, instance, {v2});
}

void ComputeSingleMAC::build_graph()
{
	using namespace halco::hicann_dls::vx::v2;

	if (std::adjacent_find(m_weights.begin(), m_weights.end(), [](auto const& a, auto const& b) {
		    return a.size() != b.size();
	    }) != m_weights.end()) {
		throw std::runtime_error("Synapse weight matrix is not rectangular.");
	}

	if (m_weights.size() != m_row_modes.size()) {
		throw std::runtime_error("Synapse weight matrix row size and row modes size don't match.");
	}

	RangeSplit x_split(NeuronColumnOnDLS::size);
	RangeSplit y_split(SynapseRowOnSynram::size);
	auto const x_split_ranges = x_split(output_size());
	auto const y_split_ranges = y_split(input_size());

	SingleChipExecutionInstanceManager execution_instance_manager;
	auto instance = coordinate::ExecutionInstance();

	vertex::ExternalInput external_input(ConnectionType::DataInputUInt16, 1);
	vertex::DataInput data_input(ConnectionType::CrossbarInputLabel, 1);
	auto last_instance = instance;

	auto input_vertex = m_graph.add(external_input, instance, {});
	Graph::vertex_descriptor crossbar_input_vertex =
	    m_graph.add(data_input, instance, {input_vertex});

	auto const set_enable_loopback = [&]() {
		if (m_enable_loopback) {
			std::vector<Input> loopback_vertices;
			loopback_vertices.reserve(SPL1Address::size);
			for (size_t i = 0; i < SPL1Address::size; ++i) {
				CrossbarNodeOnDLS coordinate(CrossbarInputOnDLS(i + 8), CrossbarOutputOnDLS(8 + i));
				haldls::vx::CrossbarNode config;
				vertex::CrossbarNode crossbar_node(coordinate, config);
				loopback_vertices.push_back(
				    m_graph.add(crossbar_node, instance, {crossbar_input_vertex}));
			}
			vertex::CrossbarL2Output l2_output;
			auto const vl2 = m_graph.add(l2_output, instance, loopback_vertices);
			vertex::DataOutput data_output_loopback(ConnectionType::DataOutputUInt16, 1);
			m_graph.add(data_output_loopback, instance, {vl2});
		}
	};
	set_enable_loopback();

	for (auto const& x_range : x_split_ranges) {
		std::vector<Input> local_output_vertices;
		for (auto const& y_range : y_split_ranges) {
			if (instance != last_instance) {
				input_vertex = m_graph.add(external_input, instance, {});
				crossbar_input_vertex = m_graph.add(data_input, instance, {input_vertex});
				set_enable_loopback();
			}

			Weights local_weights(y_range.size);
			for (size_t i = 0; i < local_weights.size(); ++i) {
				local_weights.at(i).insert(
				    local_weights.at(i).end(),
				    m_weights.at(i + y_range.offset).begin() + x_range.offset,
				    m_weights.at(i + y_range.offset).begin() + x_range.offset + x_range.size);
			}

			assert(m_row_modes.size() >= y_range.offset + y_range.size);
			RowModes local_row_modes(
			    m_row_modes.begin() + y_range.offset,
			    m_row_modes.begin() + y_range.offset + y_range.size);

			local_output_vertices.push_back(insert_synram(
			    m_graph, std::move(local_weights), std::move(local_row_modes), instance,
			    execution_instance_manager.get_current_hemisphere(), crossbar_input_vertex));
			m_synram_handles.push_back({input_vertex, y_range.size, y_range.offset,
			                            execution_instance_manager.get_current_hemisphere()});

			last_instance = instance;
			instance = execution_instance_manager.next();
		}

		if (local_output_vertices.size() > 1) {
			instance =
			    execution_instance_manager.next_index(); // FIXME we only want to get next index
			// add additions
			// load all data
			vertex::DataInput data_input(ConnectionType::Int8, x_range.size);
			std::vector<Input> local_inputs;
			local_inputs.reserve(local_output_vertices.size());
			for (auto const vertex : local_output_vertices) {
				local_inputs.push_back(m_graph.add(data_input, instance, {vertex}));
			}
			// add all data
			vertex::Addition addition(x_range.size);
			auto const v_add = m_graph.add(addition, instance, local_inputs);
			vertex::DataOutput data_output(ConnectionType::Int8, x_range.size);
			auto const v_out = m_graph.add(data_output, instance, {v_add});
			m_output_vertices.push_back(v_out);
		} else {
			std::transform(
			    local_output_vertices.begin(), local_output_vertices.end(),
			    std::back_inserter(m_output_vertices), [](auto const& i) { return i.descriptor; });
		}
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
	using namespace halco::hicann_dls::vx::v2;
	// conversion impl. from halco inlined because it is faster (Issue #3702)
	auto const local_row = row.toSynapseRowOnSynram();
	auto const synapse_driver = (static_cast<size_t>(local_row) / SynapseRowOnSynapseDriver::size);
	auto const spl1_address = synapse_driver % PADIBusOnPADIBusBlock::size;
	auto const synapse_label =
	    ((static_cast<size_t>(local_row) % SynapseRowOnSynapseDriver::size) << 5) |
	    ((UInt5::max - value.value()) + 1);
	auto const row_select = synapse_driver / PADIBusOnPADIBusBlock::size;
	auto const h = (static_cast<size_t>(row.toSynramOnDLS()) << 13);
	return haldls::vx::SpikeLabel(h | (spl1_address << 14) | (row_select) << 6 | synapse_label);
}

IODataMap ComputeSingleMAC::generate_input_events(
    Activations const& inputs,
    std::vector<SynramHandle> const& synram_handles,
    size_t const num_sends,
    haldls::vx::v2::Timer::Value const wait_between_events)
{
	using namespace haldls::vx::v2;
	using namespace halco::hicann_dls::vx;

	IODataMap data_map;

	size_t const batch_size = inputs.size();
	// get synram_handle indices of the same input_vertex
	std::map<Graph::vertex_descriptor, std::vector<size_t>> indices;
	for (size_t i = 0; i < synram_handles.size(); ++i) {
		auto const input_vertex = synram_handles.at(i).input_vertex;
		indices[input_vertex].push_back(i);
	}

	// resize event map sequentially
	for (auto const& [input_vertex, _] : indices) {
		data_map.spike_events[input_vertex].resize(batch_size);
	}

	auto const generate_events_for_range = [&](tbb::blocked_range<size_t> const& r) {
		// thread-local label storage
		halco::common::typed_array<std::vector<SpikeLabel>, HemisphereOnDLS> labels;

		for (size_t batch = r.begin(); batch < r.end(); ++batch) {
			auto const& local_inputs = inputs.at(batch);
			for (auto const& [input_vertex, index] : indices) {
				// reserve thread-local labels
				for (auto& l : labels) {
					l.reserve(SynapseRowOnSynram::size);
				}

				// generate spike labels
				assert(index.size() <= labels.size());
				for (auto const i : index) {
					auto const& local_synram_handle = synram_handles.at(i);
					auto const input_size = local_synram_handle.input_size;
					auto const input_offset = local_synram_handle.input_offset;
					auto const hemisphere = local_synram_handle.hemisphere;
					auto& local_labels = labels[hemisphere];

					// pack all events from one hemisphere after one another
					assert(local_inputs.size() >= input_offset + input_size);
					for (size_t j = 0; j < input_size; ++j) {
						auto const input = local_inputs[input_offset + j];
						auto const label = get_spike_label(
						    SynapseRowOnDLS(SynapseRowOnSynram(j), hemisphere.toSynramOnDLS()),
						    input);
						if (label) {
							local_labels.push_back(*label);
						}
					}
				}

				auto const [labels_min_it, labels_max_it] = std::minmax_element(
				    labels.begin(), labels.end(),
				    [](auto const& a, auto const& b) { return a.size() < b.size(); });
				auto const& labels_min = *labels_min_it;
				auto const& labels_max = *labels_max_it;

				auto& events = data_map.spike_events.at(input_vertex).at(batch);
				events.reserve(labels_max.size() * num_sends);
				// add events from back to unsure equal time between last event and readout
				// for both hemispheres
				TimedSpike::Time time(labels_max.size() * wait_between_events * num_sends);
				// add 2-packed events (both hemispheres)
				size_t const labels_min_size = labels_min.size();
				size_t const labels_max_size = labels_max.size();
				size_t const both_hemispheres = labels_min_size * num_sends;
				for (size_t n = 0; n < both_hemispheres; ++n) {
					auto const label_min = labels_min[n % labels_min_size];
					auto const label_max = labels_max[n % labels_max_size];
					SpikePack2ToChip const payload(
					    SpikePack2ToChip::labels_type{label_min, label_max});
					events.push_back(TimedSpike{time, payload});
					time = TimedSpike::Time(time - wait_between_events);
				}
				// add 1-packed left events (hemisphere with more events)
				size_t const one_hemisphere = labels_max_size * num_sends;
				for (size_t n = both_hemispheres; n < one_hemisphere; ++n) {
					auto const label = labels_max[n % labels_max_size];
					SpikePack1ToChip const payload(SpikePack1ToChip::labels_type{label});
					events.push_back(TimedSpike{time, payload});
					time = TimedSpike::Time(time - wait_between_events);
				}

				// clear label storage
				for (auto& l : labels) {
					l.clear();
				}

				std::sort(events.begin(), events.end(), [](auto const& a, auto const& b) {
					return a.time < b.time;
				});
			}
		}
	};

	tbb::parallel_for(tbb::blocked_range<size_t>(0, batch_size), generate_events_for_range);

	return data_map;
}

std::vector<std::vector<Int8>> ComputeSingleMAC::run(
    Activations const& inputs, hxcomm::vx::ConnectionVariant& connection)
{
	using namespace halco::hicann_dls::vx::v2;
	auto logger = log4cxx::Logger::getLogger("grenade.ComputeSingleMAC");

	// Construct map of one connection to HW
	JITGraphExecutor::Connections connections(
	    {std::pair<DLSGlobal, hxcomm::vx::ConnectionVariant&>(DLSGlobal(), connection)});

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

	hate::Timer input_timer;
	auto const input_list =
	    generate_input_events(inputs, m_synram_handles, m_num_sends, m_wait_between_events);
	LOG4CXX_DEBUG(logger, "run(): input processing time: " << input_timer.print());

	// run Graph with given inputs and return results
	auto const output_activation_map =
	    JITGraphExecutor::run(m_graph, input_list, connections, m_chip_configs);

	hate::Timer output_timer;
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

	if (m_enable_loopback) {
		boost::accumulators::accumulator_set<
		    double, boost::accumulators::features<
		                boost::accumulators::tag::mean, boost::accumulators::tag::variance>>
		    acc;
		for (auto const& l : output_activation_map.spike_event_output) {
			for (auto const& b : l.second) {
				halco::common::typed_array<std::vector<haldls::vx::ChipTime>, HemisphereOnDLS> t;
				for (auto const& s : b) {
					t[HemisphereOnDLS(s.get_label().get_neuron_label() & (1 << 13) ? 1 : 0)]
					    .push_back(s.get_chip_time());
				}
				for (auto& ht : t) {
					std::sort(ht.begin(), ht.end());
					for (size_t i = 1; i < ht.size(); ++i) {
						acc(static_cast<intmax_t>(ht.at(i)) - static_cast<intmax_t>(ht.at(i - 1)));
					}
				}
			}
		}
		std::stringstream ss;
		LOG4CXX_DEBUG(
		    logger, "run(): event inter-spike-interval statistics: "
		                << "mean(" << boost::accumulators::mean(acc) << "), std("
		                << std::sqrt(boost::accumulators::variance(acc)) << ")" << std::endl);
	}
	LOG4CXX_DEBUG(logger, "run(): output processing time: " << output_timer.print());
	return output;
}

} // namespace grenade::vx
