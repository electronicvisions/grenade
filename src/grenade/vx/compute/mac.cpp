#include "grenade/vx/compute/mac.h"

#include "grenade/cerealization.h"
#include "grenade/vx/backend/connection.h"
#include "grenade/vx/config.h"
#include "grenade/vx/event.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/range_split.h"
#include "grenade/vx/single_chip_execution_instance_manager.h"
#include "grenade/vx/transformation/concatenation.h"
#include "grenade/vx/transformation/mac_spiketrain_generator.h"
#include "halco/common/cerealization_geometry.h"
#include "hate/math.h"
#include "hate/timer.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <cereal/types/vector.hpp>
#include <log4cxx/logger.h>
#include <tbb/parallel_for_each.h>

#include <algorithm>
#include <map>
#include <unordered_map>

namespace grenade::vx::compute {

MAC::Weight::UnsignedWeight MAC::Weight::toExcitatory() const
{
	return UnsignedWeight(std::max(value(), static_cast<value_type>(0)));
}

MAC::Weight::UnsignedWeight MAC::Weight::toInhibitory() const
{
	return UnsignedWeight(-std::min(value(), static_cast<value_type>(0)));
}

Graph::vertex_descriptor MAC::insert_synram(
    Graph& graph,
    Weights&& weights,
    coordinate::ExecutionInstance const& instance,
    halco::hicann_dls::vx::v2::HemisphereOnDLS const& hemisphere,
    Graph::vertex_descriptor const crossbar_input_vertex)
{
	using namespace haldls::vx::v2;
	using namespace halco::hicann_dls::vx::v2;

	auto const x_size = weights.at(0).size();
	auto const y_size = weights.size() * 2 /* signed */;

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
		l.insert(l.end(), x_size, lola::vx::v2::SynapseMatrix::Label(0));
		i++;
	}

	// add synapse driver
	assert(y_size % 2 == 0) /* always without remainder because of signed weights */;
	auto const num_syndrv = y_size / 2;
	std::vector<Input> synapse_driver_vertices;
	synapse_driver_vertices.reserve(num_syndrv);
	for (size_t i = 0; i < num_syndrv; ++i) {
		auto const padi_bus_vertex = padi_bus_vertices.at(i % PADIBusOnPADIBusBlock::size);
		vertex::SynapseDriver synapse_driver(
		    SynapseDriverOnDLS(
		        SynapseDriverOnSynapseDriverBlock(i), hemisphere.toSynapseDriverBlockOnDLS()),
		    {vertex::SynapseDriver::Config::RowAddressCompareMask(0b11111),
		     {vertex::SynapseDriver::Config::RowModes::value_type::excitatory,
		      vertex::SynapseDriver::Config::RowModes::value_type::inhibitory},
		     false});
		synapse_driver_vertices.push_back(graph.add(synapse_driver, instance, {padi_bus_vertex}));
	}
	// add synapse array
	vertex::SynapseArrayView::Weights unsigned_weights(y_size);
	for (size_t i = 0; i < weights.size(); ++i) {
		unsigned_weights.at(2 * i).reserve(x_size);
		unsigned_weights.at(2 * i + 1).reserve(x_size);
		for (size_t j = 0; j < x_size; ++j) {
			unsigned_weights.at(2 * i).push_back(weights.at(i).at(j).toInhibitory());
			unsigned_weights.at(2 * i + 1).push_back(weights.at(i).at(j).toExcitatory());
		}
	}

	vertex::SynapseArrayView synapse_array(
	    hemisphere.toSynramOnDLS(), std::move(rows), columns, std::move(unsigned_weights),
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
	vertex::NeuronView::Config config{lola::vx::v2::AtomicNeuron::EventRouting::Address(), true};
	vertex::NeuronView::Configs enable_resets(x_size, config);
	vertex::NeuronView neurons(
	    std::move(nrns), std::move(enable_resets), hemisphere.toNeuronRowOnDLS());
	auto const v1 = graph.add(std::move(neurons), instance, {synapse_array_vertex});
	// add readout
	vertex::CADCMembraneReadoutView readout(
	    std::move(vertex::CADCMembraneReadoutView::Columns({std::move(columns)})),
	    hemisphere.toSynramOnDLS());
	auto const v2 = graph.add(readout, instance, {v1});
	// add store
	vertex::DataOutput data_output(ConnectionType::Int8, x_size);
	return graph.add(data_output, instance, {v2});
}

namespace {

auto get_hemisphere_placement(
    SingleChipExecutionInstanceManager& execution_instance_manager,
    std::vector<RangeSplit::SubRange> const& x_split_ranges,
    std::vector<RangeSplit::SubRange> const& y_split_ranges)
{
	auto instance = coordinate::ExecutionInstance();

	std::vector<
	    std::pair<coordinate::ExecutionInstance, halco::hicann_dls::vx::v2::HemisphereOnDLS>>
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
    std::vector<RangeSplit::SubRange> const& x_split_ranges,
    std::vector<RangeSplit::SubRange> const& y_split_ranges,
    std::vector<
        std::pair<coordinate::ExecutionInstance, halco::hicann_dls::vx::v2::HemisphereOnDLS>>
        hemispheres)
{
	typedef std::pair<RangeSplit::SubRange, RangeSplit::SubRange> XYSubRange;
	std::unordered_map<
	    coordinate::ExecutionInstance,
	    std::map<halco::hicann_dls::vx::v2::HemisphereOnDLS, XYSubRange>>
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
    Graph& graph,
    bool const enable,
    coordinate::ExecutionInstance const& instance,
    Graph::vertex_descriptor const crossbar_input_vertex)
{
	using namespace halco::hicann_dls::vx::v2;
	if (enable) {
		std::vector<Input> loopback_vertices;
		loopback_vertices.reserve(SPL1Address::size);
		for (size_t i = 0; i < SPL1Address::size; ++i) {
			CrossbarNodeOnDLS coordinate(CrossbarInputOnDLS(i + 8), CrossbarOutputOnDLS(8 + i));
			haldls::vx::v2::CrossbarNode config;
			vertex::CrossbarNode crossbar_node(coordinate, config);
			loopback_vertices.push_back(
			    graph.add(crossbar_node, instance, {crossbar_input_vertex}));
		}
		vertex::CrossbarL2Output l2_output;
		auto const vl2 = graph.add(l2_output, instance, loopback_vertices);
		vertex::DataOutput data_output_loopback(ConnectionType::TimedSpikeFromChipSequence, 1);
		graph.add(data_output_loopback, instance, {vl2});
	}
}

} // namespace

void MAC::build_graph()
{
	using namespace halco::hicann_dls::vx::v2;

	if (std::adjacent_find(m_weights.begin(), m_weights.end(), [](auto const& a, auto const& b) {
		    return a.size() != b.size();
	    }) != m_weights.end()) {
		throw std::runtime_error("Synapse weight matrix is not rectangular.");
	}

	RangeSplit x_split(NeuronColumnOnDLS::size);
	RangeSplit y_split(SynapseDriverOnSynapseDriverBlock::size);
	auto const x_split_ranges = x_split(output_size());
	auto const y_split_ranges = y_split(input_size());

	SingleChipExecutionInstanceManager execution_instance_manager;

	auto const hemispheres =
	    get_hemisphere_placement(execution_instance_manager, x_split_ranges, y_split_ranges);

	auto const placed_ranges = get_placed_ranges(x_split_ranges, y_split_ranges, hemispheres);

	vertex::ExternalInput external_input(ConnectionType::DataUInt5, input_size());
	auto const external_instance = execution_instance_manager.next_index();
	m_input_vertex = m_graph.add(external_input, external_instance, {});

	std::unordered_map<coordinate::ExecutionInstance, std::map<HemisphereOnDLS, Input>>
	    hemisphere_outputs;
	for (auto const& [instance, hs] : placed_ranges) {
		halco::common::typed_array<size_t, HemisphereOnDLS> sizes;
		sizes.fill(0);
		std::vector<Input> uint5_inputs;
		for (auto const& [hemisphere, xy_range] : hs) {
			auto const y_size = xy_range.second.size;
			sizes[hemisphere] = y_size;
			// Add store from (range subset of ) external data and local load
			vertex::DataInput external_data_input(ConnectionType::UInt5, y_size);
			vertex::DataOutput external_data_output(ConnectionType::UInt5, y_size);
			vertex::DataInput data_input(ConnectionType::UInt5, y_size);
			auto const v1 = m_graph.add(
			    external_data_input, external_instance,
			    {{m_input_vertex, {xy_range.second.offset, xy_range.second.offset + y_size - 1}}});
			auto const input_vertex = m_graph.add(external_data_output, external_instance, {v1});
			uint5_inputs.push_back(m_graph.add(data_input, instance, {input_vertex}));
		}

		// Add spiketrain generator, connect to crossbar
		auto spiketrain_generator = std::make_unique<transformation::MACSpikeTrainGenerator>(
		    sizes, m_num_sends, m_wait_between_events);
		Vertex transformation(std::move(vertex::Transformation(std::move(spiketrain_generator))));
		auto const data_input_vertex =
		    m_graph.add(std::move(transformation), instance, uint5_inputs);
		vertex::CrossbarL2Input crossbar_l2_input;
		Graph::vertex_descriptor crossbar_input_vertex =
		    m_graph.add(crossbar_l2_input, instance, {data_input_vertex});

		// Maybe enable event loopback
		set_enable_loopback(m_graph, m_enable_loopback, instance, crossbar_input_vertex);

		std::vector<Input> local_output_vertices;
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

			hemisphere_outputs[instance].insert(
			    {hemisphere, Input(insert_synram(
			                     m_graph, std::move(local_weights), instance, hemisphere,
			                     crossbar_input_vertex))});
		}
	}

	// Add vertical addition of hemispheres
	std::vector<Graph::vertex_descriptor> output_vertices;
	size_t i = 0;
	std::vector<size_t> x_split_sizes;
	for (auto const& x_range : x_split_ranges) {
		x_split_sizes.push_back(x_range.size);
		std::vector<Input> local_hemisphere_outputs;
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
			vertex::DataInput data_input(ConnectionType::Int8, x_range.size);
			std::vector<Input> local_inputs;
			local_inputs.reserve(local_hemisphere_outputs.size());
			for (auto const vertex : local_hemisphere_outputs) {
				local_inputs.push_back(m_graph.add(data_input, instance, {vertex}));
			}
			// add all data
			vertex::Addition addition(x_range.size);
			auto const v_add = m_graph.add(addition, instance, local_inputs);
			vertex::DataOutput data_output(ConnectionType::Int8, x_range.size);
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
	std::vector<Input> local_inputs;
	local_inputs.reserve(output_vertices.size());
	i = 0;
	for (auto const v : output_vertices) {
		vertex::DataInput data_input(ConnectionType::Int8, x_split_ranges.at(i).size);
		local_inputs.push_back(m_graph.add(data_input, instance, {v}));
		i++;
	}
	auto concatenation =
	    std::make_unique<transformation::Concatenation>(ConnectionType::Int8, x_split_sizes);
	Vertex transformation(std::move(vertex::Transformation(std::move(concatenation))));
	auto const vc = m_graph.add(std::move(transformation), instance, local_inputs);
	vertex::DataOutput data_output(ConnectionType::Int8, output_size());
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

std::vector<std::vector<Int8>> MAC::run(
    Activations const& inputs, ChipConfig const& config, backend::Connection& connection) const
{
	using namespace halco::hicann_dls::vx::v2;
	auto logger = log4cxx::Logger::getLogger("grenade.MAC");

	// Construct map of one connection to HW
	JITGraphExecutor::Connections connections;
	connections.insert(std::pair<DLSGlobal, backend::Connection&>(DLSGlobal(), connection));

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
	IODataMap input_list;
	input_list.data[m_input_vertex] = inputs;
	LOG4CXX_DEBUG(logger, "run(): input processing time: " << input_timer.print());

	JITGraphExecutor::ChipConfigs chip_configs(
	    {std::make_pair(halco::hicann_dls::vx::DLSGlobal(), config)});

	// run Graph with given inputs and return results
	auto const output_activation_map =
	    JITGraphExecutor::run(m_graph, input_list, connections, chip_configs);

	hate::Timer output_timer;
	auto const output =
	    std::get<std::vector<std::vector<Int8>>>(output_activation_map.data.at(m_output_vertex));

	if (m_enable_loopback) {
		boost::accumulators::accumulator_set<
		    double, boost::accumulators::features<
		                boost::accumulators::tag::mean, boost::accumulators::tag::variance>>
		    acc;
		for (auto const& l : output_activation_map.data) {
			if (!std::holds_alternative<std::vector<TimedSpikeFromChipSequence>>(l.second)) {
				continue;
			}
			for (auto const& b : std::get<std::vector<TimedSpikeFromChipSequence>>(l.second)) {
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

template <typename Archive>
void MAC::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_enable_loopback);
	ar(m_graph);
	ar(m_input_vertex);
	ar(m_output_vertex);
	ar(m_weights);
	ar(m_num_sends);
	ar(m_wait_between_events);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::MAC)
CEREAL_CLASS_VERSION(grenade::vx::compute::MAC, 0)
