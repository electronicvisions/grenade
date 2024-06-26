#include "grenade/vx/compute/mac.h"

#include "grenade/vx/common/execution_instance_id.h"
#include "grenade/vx/compute/detail/range_split.h"
#include "grenade/vx/compute/detail/single_chip_execution_instance_manager.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/graph.h"
#include "grenade/vx/signal_flow/input.h"
#include "grenade/vx/signal_flow/input_data.h"
#include "grenade/vx/signal_flow/vertex/transformation/addition.h"
#include "grenade/vx/signal_flow/vertex/transformation/concatenation.h"
#include "grenade/vx/signal_flow/vertex/transformation/mac_spiketrain_generator.h"
#include "hate/math.h"
#include "hate/timer.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <log4cxx/logger.h>
#include <tbb/parallel_for_each.h>

#include <algorithm>
#include <fstream>
#include <map>

namespace grenade::vx::compute {

MAC::Weight::UnsignedWeight MAC::Weight::toExcitatory() const
{
	return UnsignedWeight(std::max(value(), static_cast<value_type>(0)));
}

MAC::Weight::UnsignedWeight MAC::Weight::toInhibitory() const
{
	return UnsignedWeight(-std::min(value(), static_cast<value_type>(0)));
}

std::pair<
    signal_flow::Graph::vertex_descriptor,
    std::optional<signal_flow::Graph::vertex_descriptor>>
MAC::insert_synram(
    signal_flow::Graph& graph,
    Weights&& weights,
    common::ExecutionInstanceID const& instance,
    halco::hicann_dls::vx::v3::HemisphereOnDLS const& hemisphere,
    std::optional<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> const& madc_recording_neuron,
    signal_flow::Graph::vertex_descriptor const crossbar_input_vertex)
{
	using namespace haldls::vx::v3;
	using namespace halco::hicann_dls::vx::v3;

	auto const x_size = weights.at(0).size();
	auto const y_size = weights.size() * 2 /* signed */;

	signal_flow::vertex::SynapseArrayView::Columns columns;
	columns.reserve(x_size);
	for (size_t o = 0; o < x_size; ++o) {
		auto const column = SynapseOnSynapseRow(o);
		columns.push_back(column);
	}

	// configure crossbar and padi busses
	auto const num_padi_bus = std::min(
	    hate::math::round_up_integer_division(y_size, 2),
	    static_cast<size_t>(PADIBusOnPADIBusBlock::size));
	std::vector<signal_flow::Input> padi_bus_vertices;
	padi_bus_vertices.reserve(num_padi_bus);
	for (size_t i = 0; i < num_padi_bus; ++i) {
		CrossbarNodeOnDLS coordinate(
		    CrossbarInputOnDLS(i + 8), CrossbarOutputOnDLS(i + (hemisphere.toEnum() * 4)));
		CrossbarNode config;
		config.set_mask(CrossbarNode::neuron_label_type(0b1 << 13));
		config.set_target(CrossbarNode::neuron_label_type((hemisphere.toEnum() << 13)));
		signal_flow::vertex::CrossbarNode crossbar_node(coordinate, config);
		auto const v1 = graph.add(crossbar_node, instance, {crossbar_input_vertex});
		padi_bus_vertices.push_back(graph.add(
		    signal_flow::vertex::PADIBus(signal_flow::vertex::PADIBus::Coordinate(
		        PADIBusOnPADIBusBlock(i), hemisphere.toPADIBusBlockOnDLS())),
		    instance, {v1}));
	}

	signal_flow::vertex::SynapseArrayView::Labels labels(y_size);
	signal_flow::vertex::SynapseArrayView::Rows rows;
	rows.reserve(y_size);
	size_t i = 0;
	for (auto& l : labels) {
		SynapseRowOnSynram const sr(i);
		rows.push_back(sr);
		l.insert(l.end(), x_size, lola::vx::v3::SynapseMatrix::Label(0));
		i++;
	}

	// add synapse driver
	assert(y_size % 2 == 0) /* always without remainder because of signed weights */;
	auto const num_syndrv = y_size / 2;
	std::vector<signal_flow::Input> synapse_driver_vertices;
	synapse_driver_vertices.reserve(num_syndrv);
	for (size_t i = 0; i < num_syndrv; ++i) {
		auto const padi_bus_vertex = padi_bus_vertices.at(i % PADIBusOnPADIBusBlock::size);
		signal_flow::vertex::SynapseDriver synapse_driver(
		    SynapseDriverOnDLS(
		        SynapseDriverOnSynapseDriverBlock(i), hemisphere.toSynapseDriverBlockOnDLS()),
		    {signal_flow::vertex::SynapseDriver::Config::RowAddressCompareMask(0b11111),
		     {signal_flow::vertex::SynapseDriver::Config::RowModes::value_type::excitatory,
		      signal_flow::vertex::SynapseDriver::Config::RowModes::value_type::inhibitory},
		     false});
		synapse_driver_vertices.push_back(graph.add(synapse_driver, instance, {padi_bus_vertex}));
	}
	// add synapse array
	signal_flow::vertex::SynapseArrayView::Weights unsigned_weights(y_size);
	for (size_t i = 0; i < weights.size(); ++i) {
		unsigned_weights.at(2 * i).reserve(x_size);
		unsigned_weights.at(2 * i + 1).reserve(x_size);
		for (size_t j = 0; j < x_size; ++j) {
			unsigned_weights.at(2 * i).push_back(weights.at(i).at(j).toInhibitory());
			unsigned_weights.at(2 * i + 1).push_back(weights.at(i).at(j).toExcitatory());
		}
	}

	signal_flow::vertex::SynapseArrayView synapse_array(
	    hemisphere.toSynramOnDLS(), std::move(rows), columns, std::move(unsigned_weights),
	    std::move(labels));
	auto const synapse_array_vertex =
	    graph.add(std::move(synapse_array), instance, synapse_driver_vertices);
	// add neurons
	signal_flow::vertex::NeuronView::Columns nrns;
	nrns.reserve(x_size);
	for (size_t o = 0; o < x_size; ++o) {
		auto const nrn = NeuronColumnOnDLS(o);
		nrns.push_back(nrn);
	}
	signal_flow::vertex::NeuronView::Config config{
	    lola::vx::v3::AtomicNeuron::EventRouting::Address(), true};
	signal_flow::vertex::NeuronView::Configs enable_resets(x_size, config);
	signal_flow::vertex::NeuronView neurons(
	    std::move(nrns), std::move(enable_resets), hemisphere.toNeuronRowOnDLS());
	auto const v1 = graph.add(std::move(neurons), instance, {synapse_array_vertex});
	// add readout
	signal_flow::vertex::CADCMembraneReadoutView::Sources sources(1);
	sources.at(0).resize(
	    columns.size(),
	    signal_flow::vertex::CADCMembraneReadoutView::Sources::value_type::value_type::membrane);
	signal_flow::vertex::CADCMembraneReadoutView readout(
	    signal_flow::vertex::CADCMembraneReadoutView::Columns({std::move(columns)}),
	    hemisphere.toSynramOnDLS(), signal_flow::vertex::CADCMembraneReadoutView::Mode::hagen,
	    std::move(sources));
	auto const v2 = graph.add(readout, instance, {v1});
	// add store
	signal_flow::vertex::DataOutput data_output(signal_flow::ConnectionType::Int8, x_size);
	// add madc recording
	if (madc_recording_neuron &&
	    hemisphere == madc_recording_neuron->toNeuronRowOnDLS().toHemisphereOnDLS() &&
	    madc_recording_neuron->toNeuronColumnOnDLS().value() < x_size) {
		signal_flow::vertex::MADCReadoutView madc_readout(
		    signal_flow::vertex::MADCReadoutView::Source{
		        *madc_recording_neuron,
		        signal_flow::vertex::MADCReadoutView::Source::Type::membrane},
		    std::nullopt, signal_flow::vertex::MADCReadoutView::SourceSelection{});
		auto const madc_readout_column = madc_recording_neuron->toNeuronColumnOnDLS().value();
		auto const v3 =
		    graph.add(madc_readout, instance, {{v1, {madc_readout_column, madc_readout_column}}});
		signal_flow::vertex::DataOutput madc_data_output(
		    signal_flow::ConnectionType::TimedMADCSampleFromChipSequence, 1);
		return {
		    graph.add(data_output, instance, {v2}), graph.add(madc_data_output, instance, {v3})};
	}
	return {graph.add(data_output, instance, {v2}), std::nullopt};
}

namespace {

auto get_hemisphere_placement(
    detail::SingleChipExecutionInstanceManager& execution_instance_manager,
    std::vector<detail::RangeSplit::SubRange> const& x_split_ranges,
    std::vector<detail::RangeSplit::SubRange> const& y_split_ranges)
{
	auto instance = common::ExecutionInstanceID();

	std::vector<std::pair<common::ExecutionInstanceID, halco::hicann_dls::vx::v3::HemisphereOnDLS>>
	    hemispheres;
	for ([[maybe_unused]] auto const& x_range : x_split_ranges) {
		for ([[maybe_unused]] auto const& y_range : y_split_ranges) {
			hemispheres.push_back({instance, execution_instance_manager.get_current_hemisphere()});
			instance = execution_instance_manager.next();
		}
	}
	return hemispheres;
}

auto get_placed_ranges(
    std::vector<detail::RangeSplit::SubRange> const& x_split_ranges,
    std::vector<detail::RangeSplit::SubRange> const& y_split_ranges,
    std::vector<std::pair<common::ExecutionInstanceID, halco::hicann_dls::vx::v3::HemisphereOnDLS>>
        hemispheres)
{
	typedef std::pair<detail::RangeSplit::SubRange, detail::RangeSplit::SubRange> XYSubRange;
	std::map<
	    common::ExecutionInstanceID,
	    std::map<halco::hicann_dls::vx::v3::HemisphereOnDLS, XYSubRange>>
	    placed_ranges;
	size_t i = 0;
	for (auto const& x_range : x_split_ranges) {
		size_t j = 0;
		for (auto const& y_range : y_split_ranges) {
			auto const [instance, hemisphere] = hemispheres.at(i * y_split_ranges.size() + j);
			placed_ranges[instance][hemisphere] = {x_range, y_range};
			j++;
		}
		i++;
	}
	return placed_ranges;
}

void set_enable_loopback(
    signal_flow::Graph& graph,
    bool const enable,
    common::ExecutionInstanceID const& instance,
    signal_flow::Graph::vertex_descriptor const crossbar_input_vertex)
{
	using namespace halco::hicann_dls::vx::v3;
	if (enable) {
		std::vector<signal_flow::Input> loopback_vertices;
		loopback_vertices.reserve(SPL1Address::size);
		for (size_t i = 0; i < SPL1Address::size; ++i) {
			CrossbarNodeOnDLS coordinate(CrossbarInputOnDLS(i + 8), CrossbarOutputOnDLS(8 + i));
			haldls::vx::v3::CrossbarNode config;
			signal_flow::vertex::CrossbarNode crossbar_node(coordinate, config);
			loopback_vertices.push_back(
			    graph.add(crossbar_node, instance, {crossbar_input_vertex}));
		}
		signal_flow::vertex::CrossbarL2Output l2_output;
		auto const vl2 = graph.add(l2_output, instance, loopback_vertices);
		signal_flow::vertex::DataOutput data_output_loopback(
		    signal_flow::ConnectionType::TimedSpikeFromChipSequence, 1);
		graph.add(data_output_loopback, instance, {vl2});
	}
}

} // namespace

void MAC::build_graph()
{
	using namespace halco::hicann_dls::vx::v3;

	if (std::adjacent_find(m_weights.begin(), m_weights.end(), [](auto const& a, auto const& b) {
		    return a.size() != b.size();
	    }) != m_weights.end()) {
		throw std::runtime_error("Synapse weight matrix is not rectangular.");
	}

	detail::RangeSplit x_split(NeuronColumnOnDLS::size);
	detail::RangeSplit y_split(SynapseDriverOnSynapseDriverBlock::size);
	auto const x_split_ranges = x_split(output_size());
	auto const y_split_ranges = y_split(input_size());

	detail::SingleChipExecutionInstanceManager execution_instance_manager;

	auto const hemispheres =
	    get_hemisphere_placement(execution_instance_manager, x_split_ranges, y_split_ranges);

	auto const placed_ranges = get_placed_ranges(x_split_ranges, y_split_ranges, hemispheres);

	signal_flow::vertex::ExternalInput external_input(
	    signal_flow::ConnectionType::DataUInt5, input_size());
	auto const external_instance = execution_instance_manager.next_index();
	m_input_vertex = m_graph.add(external_input, external_instance, {});

	std::map<common::ExecutionInstanceID, std::map<HemisphereOnDLS, signal_flow::Input>>
	    hemisphere_outputs;
	for (auto const& [instance, hs] : placed_ranges) {
		halco::common::typed_array<size_t, HemisphereOnDLS> sizes;
		sizes.fill(0);
		std::vector<signal_flow::Input> uint5_inputs;
		for (auto const& [hemisphere, xy_range] : hs) {
			auto const y_size = xy_range.second.size;
			sizes[hemisphere] = y_size;
			// Add store from (range subset of ) external data and local load
			signal_flow::vertex::DataInput external_data_input(
			    signal_flow::ConnectionType::UInt5, y_size);
			signal_flow::vertex::DataOutput external_data_output(
			    signal_flow::ConnectionType::UInt5, y_size);
			signal_flow::vertex::DataInput data_input(signal_flow::ConnectionType::UInt5, y_size);
			auto const v1 = m_graph.add(
			    external_data_input, external_instance,
			    {{m_input_vertex, {xy_range.second.offset, xy_range.second.offset + y_size - 1}}});
			auto const input_vertex = m_graph.add(external_data_output, external_instance, {v1});
			uint5_inputs.push_back(m_graph.add(data_input, instance, {input_vertex}));
		}

		// Add spiketrain generator, connect to crossbar
		auto spiketrain_generator =
		    std::make_unique<signal_flow::vertex::transformation::MACSpikeTrainGenerator>(
		        sizes, m_num_sends, m_wait_between_events);
		signal_flow::Vertex transformation(
		    signal_flow::vertex::Transformation(std::move(spiketrain_generator)));
		auto const data_input_vertex =
		    m_graph.add(std::move(transformation), instance, uint5_inputs);
		signal_flow::vertex::CrossbarL2Input crossbar_l2_input;
		signal_flow::Graph::vertex_descriptor crossbar_input_vertex =
		    m_graph.add(crossbar_l2_input, instance, {data_input_vertex});

		// Maybe enable event loopback
		set_enable_loopback(m_graph, m_enable_loopback, instance, crossbar_input_vertex);

		std::vector<signal_flow::Input> local_output_vertices;
		for (auto const& [hemisphere, xy_range] : hs) {
			auto const x_range = xy_range.first;
			auto const y_range = xy_range.second;
			Weights local_weights(y_range.size);
			for (size_t i = 0; i < local_weights.size(); ++i) {
				local_weights.at(i).insert(
				    local_weights.at(i).end(),
				    m_weights.at(i + y_range.offset).begin() + x_range.offset,
				    m_weights.at(i + y_range.offset).begin() + x_range.offset + x_range.size);
			}
			auto const [cadc_readout_data_vertex, madc_readout_data_vertex] = insert_synram(
			    m_graph, std::move(local_weights), instance, hemisphere,
			    m_madc_recording_path == ""
			        ? std::optional<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>(std::nullopt)
			        : std::optional<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>(
			              m_madc_recording_neuron),
			    crossbar_input_vertex);
			if (madc_readout_data_vertex) {
				m_madc_recording_vertices[instance] = *madc_readout_data_vertex;
			}
			hemisphere_outputs[instance].insert(
			    {hemisphere, signal_flow::Input(cadc_readout_data_vertex)});
		}
	}

	// Add vertical addition of hemispheres
	std::vector<signal_flow::Graph::vertex_descriptor> output_vertices;
	size_t i = 0;
	std::vector<size_t> x_split_sizes;
	for (auto const& x_range : x_split_ranges) {
		x_split_sizes.push_back(x_range.size);
		std::vector<signal_flow::Input> local_hemisphere_outputs;
		size_t j = 0;
		for ([[maybe_unused]] auto const& y_range : y_split_ranges) {
			auto const [instance, hemisphere] = hemispheres.at(i * y_split_ranges.size() + j);
			local_hemisphere_outputs.push_back(hemisphere_outputs.at(instance).at(hemisphere));
			j++;
		}
		i++;

		if (local_hemisphere_outputs.size() > 1) {
			auto const instance = execution_instance_manager.next_index();
			// add additions
			// load all data
			signal_flow::vertex::DataInput data_input(
			    signal_flow::ConnectionType::Int8, x_range.size);
			std::vector<signal_flow::Input> local_inputs;
			local_inputs.reserve(local_hemisphere_outputs.size());
			for (auto const vertex : local_hemisphere_outputs) {
				local_inputs.push_back(m_graph.add(data_input, instance, {vertex}));
			}
			// add all data
			signal_flow::Vertex addition(signal_flow::vertex::Transformation(
			    std::make_unique<signal_flow::vertex::transformation::Addition>(
			        local_inputs.size(), x_range.size)));
			auto const v_add = m_graph.add(std::move(addition), instance, local_inputs);
			signal_flow::vertex::DataOutput data_output(
			    signal_flow::ConnectionType::Int8, x_range.size);
			auto const v_out = m_graph.add(data_output, instance, {v_add});
			output_vertices.push_back(v_out);
		} else {
			std::transform(
			    local_hemisphere_outputs.begin(), local_hemisphere_outputs.end(),
			    std::back_inserter(output_vertices), [](auto const& in) { return in.descriptor; });
		}
	}

	// Concatenate all outputs
	auto const instance = execution_instance_manager.next_index();
	// Load all data
	std::vector<signal_flow::Input> local_inputs;
	local_inputs.reserve(output_vertices.size());
	i = 0;
	for (auto const v : output_vertices) {
		signal_flow::vertex::DataInput data_input(
		    signal_flow::ConnectionType::Int8, x_split_ranges.at(i).size);
		local_inputs.push_back(m_graph.add(data_input, instance, {v}));
		i++;
	}
	auto concatenation = std::make_unique<signal_flow::vertex::transformation::Concatenation>(
	    signal_flow::ConnectionType::Int8, x_split_sizes);
	grenade::vx::signal_flow::Vertex transformation(
	    signal_flow::vertex::Transformation(std::move(concatenation)));
	auto const vc = m_graph.add(std::move(transformation), instance, local_inputs);
	signal_flow::vertex::DataOutput data_output(signal_flow::ConnectionType::Int8, output_size());
	m_output_vertex = m_graph.add(data_output, instance, {vc});
}

size_t MAC::input_size() const
{
	return m_weights.size();
}

size_t MAC::output_size() const
{
	if (m_weights.size()) {
		return m_weights.at(0).size();
	}
	return 0;
}

std::vector<std::vector<signal_flow::Int8>> MAC::run(
    Activations const& inputs,
    lola::vx::v3::Chip const& config,
    execution::JITGraphExecutor& executor) const
{
	using namespace halco::hicann_dls::vx::v3;
	auto logger = log4cxx::Logger::getLogger("grenade.MAC");

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

	// fill graph inputs (with signal_flow::UInt5(0))
	if (batch_entry_size != input_size()) {
		throw std::runtime_error("Provided inputs size does not match MAC input size.");
	}

	hate::Timer input_timer;
	signal_flow::InputData input_list;
	std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>> timed_inputs(
	    inputs.size());
	for (size_t i = 0; i < inputs.size(); ++i) {
		timed_inputs.at(i).resize(1);
		// TODO: Think about what to do with timing information
		timed_inputs.at(i).at(0).data = inputs.at(i);
	}
	input_list.data[m_input_vertex] = timed_inputs;
	LOG4CXX_DEBUG(logger, "run(): input processing time: " << input_timer.print());

	execution::JITGraphExecutor::ChipConfigs chip_configs;
	for (auto const& [_, execution_instance] : m_graph.get_execution_instance_map()) {
		chip_configs.emplace(execution_instance, config);
	}

	// run Graph with given inputs and return results
	auto const output_activation_map = execution::run(executor, m_graph, chip_configs, input_list);

	hate::Timer output_timer;
	auto const timed_outputs =
	    std::get<std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
	        output_activation_map.data.at(m_output_vertex));
	std::vector<std::vector<signal_flow::Int8>> output(timed_outputs.size());
	for (size_t i = 0; i < output.size(); ++i) {
		assert(timed_outputs.at(i).size() == 1);
		output.at(i) = timed_outputs.at(i).at(0).data;
	}

	if (m_enable_loopback) {
		boost::accumulators::accumulator_set<
		    double, boost::accumulators::features<
		                boost::accumulators::tag::mean, boost::accumulators::tag::variance>>
		    acc;
		for (auto const& l : output_activation_map.data) {
			if (!std::holds_alternative<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
			        l.second)) {
				continue;
			}
			for (auto const& b :
			     std::get<std::vector<signal_flow::TimedSpikeFromChipSequence>>(l.second)) {
				halco::common::typed_array<std::vector<common::Time>, HemisphereOnDLS> t;
				for (auto const& s : b) {
					t[HemisphereOnDLS(s.data.get_neuron_label() & (1 << 13) ? 1 : 0)].push_back(
					    s.time);
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
	if (m_madc_recording_path != "" && !m_madc_recording_vertices.empty()) {
		if (!std::filesystem::exists(m_madc_recording_path)) {
			std::ofstream file(m_madc_recording_path);
			assert(file.is_open());
			file << "ExecutionIndex\tbatch\ttime\tvalue\n";
		}
		std::ofstream file(m_madc_recording_path, std::ios_base::app);
		assert(file.is_open());
		for (auto [instance, vertex] : m_madc_recording_vertices) {
			auto const madc_data =
			    std::get<std::vector<signal_flow::TimedMADCSampleFromChipSequence>>(
			        output_activation_map.data.at(vertex));
			for (size_t b = 0; b < output_activation_map.batch_size(); ++b) {
				auto const& local_madc_data = madc_data.at(b);
				for (auto const& sample : local_madc_data) {
					file << instance.value() << "\t" << b << "\t" << sample.time.value() << "\t"
					     << sample.data.value.value() << "\n";
				}
			}
		}
	}
	LOG4CXX_DEBUG(logger, "run(): output processing time: " << output_timer.print());
	return output;
}

} // namespace grenade::vx::compute
