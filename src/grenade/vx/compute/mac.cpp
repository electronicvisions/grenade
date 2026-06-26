#include "grenade/vx/compute/mac.h"

#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/compute/detail/range_split.h"
#include "grenade/vx/compute/detail/single_chip_execution_instance_manager.h"
#include "grenade/vx/execution/jit_graph_executor.h"
#include "grenade/vx/execution/run.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/chip.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_input.h"
#include "grenade/vx/signal_flow/vertex/crossbar_l2_output.h"
#include "grenade/vx/signal_flow/vertex/crossbar_node.h"
#include "grenade/vx/signal_flow/vertex/madc_readout.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "grenade/vx/signal_flow/vertex/padi_bus.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"
#include "grenade/vx/signal_flow/vertex/synapse_driver.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "grenade/vx/signal_flow/vertex/transformation/addition.h"
#include "grenade/vx/signal_flow/vertex/transformation/concatenation.h"
#include "grenade/vx/signal_flow/vertex/transformation/identity.h"
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

std::tuple<
    grenade::common::VertexOnTopology,
    std::optional<grenade::common::VertexOnTopology>,
    grenade::common::VertexOnTopology>
MAC::insert_synram(
    grenade::common::Topology& topology,
    grenade::common::InputData& parameterization,
    Weights&& weights,
    grenade::common::ExecutionInstanceOnExecutor const& instance,
    halco::hicann_dls::vx::v3::HemisphereOnDLS const& hemisphere,
    std::optional<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> const& madc_recording_neuron,
    grenade::common::VertexOnTopology const crossbar_input_vertex)
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
	std::vector<grenade::common::VertexOnTopology> padi_bus_vertices;
	padi_bus_vertices.reserve(num_padi_bus);
	for (size_t i = 0; i < num_padi_bus; ++i) {
		CrossbarNodeOnDLS coordinate(
		    CrossbarInputOnDLS(i + 8), CrossbarOutputOnDLS(i + (hemisphere.toEnum() * 4)));
		CrossbarNode config;
		config.set_mask(CrossbarNode::neuron_label_type(0b1 << 13));
		config.set_target(CrossbarNode::neuron_label_type((hemisphere.toEnum() << 13)));
		signal_flow::vertex::CrossbarNode crossbar_node(
		    coordinate, common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(),
		    instance);
		auto const v1 = topology.add_vertex(crossbar_node);
		parameterization.ports.set(
		    {v1, 1}, signal_flow::vertex::CrossbarNode::Parameterization(config));
		topology.add_edge(
		    crossbar_input_vertex, v1,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
		auto const v2 = topology.add_vertex(signal_flow::vertex::PADIBus(
		    signal_flow::vertex::PADIBus::Coordinate(
		        PADIBusOnPADIBusBlock(i), hemisphere.toPADIBusBlockOnDLS()),
		    common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(), instance));
		topology.add_edge(
		    v1, v2,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
		padi_bus_vertices.push_back(v2);
	}

	signal_flow::vertex::SynapseArrayView::Parameterization::Labels labels(y_size);
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
	std::vector<grenade::common::VertexOnTopology> synapse_driver_vertices;
	synapse_driver_vertices.reserve(num_syndrv);
	for (size_t i = 0; i < num_syndrv; ++i) {
		auto const padi_bus_vertex = padi_bus_vertices.at(i % PADIBusOnPADIBusBlock::size);
		signal_flow::vertex::SynapseDriver synapse_driver(
		    SynapseDriverOnDLS(
		        SynapseDriverOnSynapseDriverBlock(i), hemisphere.toSynapseDriverBlockOnDLS()),
		    common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(), instance);
		signal_flow::vertex::SynapseDriver::Parameterization synapse_driver_parameterization;
		synapse_driver_parameterization.row_address_compare_mask =
		    signal_flow::vertex::SynapseDriver::Parameterization::RowAddressCompareMask(0b11111);
		synapse_driver_parameterization.row_modes = {
		    signal_flow::vertex::SynapseDriver::Parameterization::RowModes::value_type::excitatory,
		    signal_flow::vertex::SynapseDriver::Parameterization::RowModes::value_type::inhibitory};
		synapse_driver_parameterization.enable_address_out = false;
		auto const v1 = topology.add_vertex(synapse_driver);
		parameterization.ports.set({v1, 1}, synapse_driver_parameterization);
		topology.add_edge(
		    padi_bus_vertex, v1,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
		synapse_driver_vertices.push_back(v1);
	}
	// add synapse array
	signal_flow::vertex::SynapseArrayView::Parameterization::Weights unsigned_weights(y_size);
	for (size_t i = 0; i < weights.size(); ++i) {
		unsigned_weights.at(2 * i).reserve(x_size);
		unsigned_weights.at(2 * i + 1).reserve(x_size);
		for (size_t j = 0; j < x_size; ++j) {
			unsigned_weights.at(2 * i).push_back(weights.at(i).at(j).toInhibitory());
			unsigned_weights.at(2 * i + 1).push_back(weights.at(i).at(j).toExcitatory());
		}
	}

	signal_flow::vertex::SynapseArrayView synapse_array(
	    hemisphere.toSynramOnDLS(), std::move(rows), columns, common::ChipOnConnection(),
	    grenade::common::TimeDomainOnTopology(), instance);
	signal_flow::vertex::SynapseArrayView::Parameterization synapse_array_parameterization(
	    std::move(labels), std::move(unsigned_weights));
	auto const synapse_array_vertex = topology.add_vertex(std::move(synapse_array));
	parameterization.ports.set({synapse_array_vertex, 1}, synapse_array_parameterization);
	for (size_t i = 0; auto const& synapse_driver_vertex : synapse_driver_vertices) {
		topology.add_edge(
		    synapse_driver_vertex, synapse_array_vertex,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({2}),
		        grenade::common::CuboidMultiIndexSequence(
		            {2}, grenade::common::MultiIndex({i * 2})),
		        0, 0));

		i++;
	}
	// add neurons
	signal_flow::vertex::NeuronView::Columns nrns;
	nrns.reserve(x_size);
	for (size_t o = 0; o < x_size; ++o) {
		auto const nrn = NeuronColumnOnDLS(o);
		nrns.push_back(nrn);
	}
	signal_flow::vertex::NeuronView neurons(
	    std::move(nrns), hemisphere.toNeuronRowOnDLS(), common::ChipOnConnection(),
	    grenade::common::TimeDomainOnTopology(), instance);
	auto const v1 = topology.add_vertex(std::move(neurons));
	topology.add_edge(
	    synapse_array_vertex, v1,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({x_size}),
	        grenade::common::CuboidMultiIndexSequence({x_size}), 0, 0));
	// add readout
	signal_flow::vertex::CADCMembraneReadoutView readout(
	    signal_flow::vertex::CADCMembraneReadoutView::Columns({std::move(columns)}),
	    hemisphere.toSynramOnDLS(), signal_flow::vertex::CADCMembraneReadoutView::Mode::hagen,
	    common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(), instance);
	auto const v2 = topology.add_vertex(readout);
	topology.add_edge(
	    v1, v2,
	    grenade::common::Edge(
	        grenade::common::CuboidMultiIndexSequence({x_size}),
	        grenade::common::CuboidMultiIndexSequence({x_size}), 0, 0));
	// add madc recording
	if (madc_recording_neuron &&
	    hemisphere == madc_recording_neuron->toNeuronRowOnDLS().toHemisphereOnDLS() &&
	    madc_recording_neuron->toNeuronColumnOnDLS().value() < x_size) {
		signal_flow::vertex::MADCReadoutView madc_readout(
		    *madc_recording_neuron, std::nullopt, common::ChipOnConnection(),
		    grenade::common::TimeDomainOnTopology(), instance);
		auto const madc_readout_column = madc_recording_neuron->toNeuronColumnOnDLS().value();
		auto const v3 = topology.add_vertex(madc_readout);
		signal_flow::vertex::MADCReadoutView::Parameterization madc_parameterization;
		parameterization.ports.set({v3, 1}, madc_parameterization);
		topology.add_edge(
		    v1, v3,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence(
		            {1}, grenade::common::MultiIndex({madc_readout_column})),
		        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
		return {v2, v3, v1};
	}
	return {v2, std::nullopt, v1};
}

namespace {

auto get_hemisphere_placement(
    detail::SingleChipExecutionInstanceManager& execution_instance_manager,
    std::vector<detail::RangeSplit::SubRange> const& x_split_ranges,
    std::vector<detail::RangeSplit::SubRange> const& y_split_ranges)
{
	auto instance = grenade::common::ExecutionInstanceOnExecutor();

	std::vector<std::pair<
	    grenade::common::ExecutionInstanceOnExecutor, halco::hicann_dls::vx::v3::HemisphereOnDLS>>
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
    std::vector<std::pair<
        grenade::common::ExecutionInstanceOnExecutor,
        halco::hicann_dls::vx::v3::HemisphereOnDLS>> hemispheres)
{
	typedef std::pair<detail::RangeSplit::SubRange, detail::RangeSplit::SubRange> XYSubRange;
	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
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
    grenade::common::Topology& topology,
    grenade::common::InputData& parameterization,
    bool const enable,
    grenade::common::ExecutionInstanceOnExecutor const& instance,
    grenade::common::VertexOnTopology const crossbar_input_vertex)
{
	using namespace halco::hicann_dls::vx::v3;
	if (enable) {
		std::vector<grenade::common::VertexOnTopology> loopback_vertices;
		loopback_vertices.reserve(SPL1Address::size);
		for (size_t i = 0; i < SPL1Address::size; ++i) {
			CrossbarNodeOnDLS coordinate(CrossbarInputOnDLS(i + 8), CrossbarOutputOnDLS(8 + i));
			haldls::vx::v3::CrossbarNode config;
			signal_flow::vertex::CrossbarNode crossbar_node(
			    coordinate, common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(),
			    instance);
			auto const v1 = topology.add_vertex(crossbar_node);
			parameterization.ports.set(
			    {v1, 1}, signal_flow::vertex::CrossbarNode::Parameterization(config));
			topology.add_edge(
			    crossbar_input_vertex, v1,
			    grenade::common::Edge(
			        grenade::common::CuboidMultiIndexSequence({1}),
			        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
			loopback_vertices.push_back(v1);
		}
		signal_flow::vertex::CrossbarL2Output l2_output(
		    true, common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(), instance);
		auto const vl2 = topology.add_vertex(l2_output);
		for (auto const& loopback_vertex : loopback_vertices) {
			topology.add_edge(
			    loopback_vertex, vl2,
			    grenade::common::Edge(
			        grenade::common::CuboidMultiIndexSequence({1}),
			        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));
		}
	}
}

} // namespace

void MAC::build_topology()
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

	auto const external_instance = execution_instance_manager.next_index();
	signal_flow::vertex::Transformation external_input(
	    signal_flow::vertex::transformation::Identity(
	        {signal_flow::vertex::transformation::Identity::Port(
	            signal_flow::VertexPortType(signal_flow::ConnectionType::UInt5),
	            grenade::common::CuboidMultiIndexSequence({input_size()}))}),
	    external_instance);
	m_input_vertex = m_topology->add_vertex(external_input);

	std::map<
	    grenade::common::ExecutionInstanceOnExecutor,
	    std::map<HemisphereOnDLS, grenade::common::VertexOnTopology>>
	    hemisphere_outputs;
	for (auto const& [instance, hs] : placed_ranges) {
		halco::common::typed_array<size_t, HemisphereOnDLS> sizes;
		sizes.fill(0);
		for (auto const& [hemisphere, xy_range] : hs) {
			auto const y_size = xy_range.second.size;
			sizes[hemisphere] = y_size;
		}

		// Add spiketrain generator, connect to crossbar
		signal_flow::vertex::Transformation spiketrain_generator(
		    signal_flow::vertex::transformation::MACSpikeTrainGenerator(
		        sizes, m_num_sends, m_wait_between_events),
		    execution_instance_manager.next_index());
		auto const data_input_vertex = m_topology->add_vertex(std::move(spiketrain_generator));
		for (auto const& [hemisphere, xy_range] : hs) {
			m_topology->add_edge(
			    m_input_vertex, data_input_vertex,
			    grenade::common::Edge(
			        grenade::common::CuboidMultiIndexSequence(
			            {xy_range.second.size},
			            grenade::common::MultiIndex({xy_range.second.offset})),
			        grenade::common::CuboidMultiIndexSequence({xy_range.second.size}), 0,
			        hemisphere.value()));
		}

		signal_flow::vertex::CrossbarL2Input crossbar_l2_input(
		    false, common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(), instance);
		auto const crossbar_input_vertex = m_topology->add_vertex(crossbar_l2_input);
		m_topology->add_edge(
		    data_input_vertex, crossbar_input_vertex,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({1}),
		        grenade::common::CuboidMultiIndexSequence({1}), 0, 0));

		// Maybe enable event loopback
		set_enable_loopback(
		    *m_topology, m_parameterization, m_enable_loopback, instance, crossbar_input_vertex);

		m_chip_vertices[instance] = m_topology->add_vertex(signal_flow::vertex::Chip(
		    common::ChipOnConnection(), grenade::common::TimeDomainOnTopology(), instance));

		std::vector<grenade::common::VertexOnTopology> local_output_vertices;
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
			auto const [cadc_readout_data_vertex, madc_readout_data_vertex, neuron_view_vertex] =
			    insert_synram(
			        *m_topology, m_parameterization, std::move(local_weights), instance, hemisphere,
			        m_madc_recording_path == ""
			            ? std::optional<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>(std::nullopt)
			            : std::optional<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS>(
			                  m_madc_recording_neuron),
			        crossbar_input_vertex);
			if (madc_readout_data_vertex) {
				m_madc_recording_vertices[instance] = *madc_readout_data_vertex;
			}
			hemisphere_outputs[instance].insert({hemisphere, cadc_readout_data_vertex});
			m_neuron_vertices.push_back(neuron_view_vertex);
		}
	}

	// Add vertical addition of hemispheres
	std::vector<grenade::common::VertexOnTopology> output_vertices;
	size_t i = 0;
	std::vector<size_t> x_split_sizes;
	for (auto const& x_range : x_split_ranges) {
		x_split_sizes.push_back(x_range.size);
		std::vector<grenade::common::VertexOnTopology> local_hemisphere_outputs;
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
			// add all data
			signal_flow::vertex::Transformation addition(
			    signal_flow::vertex::transformation::Addition(
			        local_hemisphere_outputs.size(), x_range.size),
			    instance);
			auto const v_add = m_topology->add_vertex(std::move(addition));
			for (size_t i = 0; auto const& local_hemisphere_output : local_hemisphere_outputs) {
				m_topology->add_edge(
				    local_hemisphere_output, v_add,
				    grenade::common::Edge(
				        grenade::common::CuboidMultiIndexSequence({x_range.size}),
				        grenade::common::CuboidMultiIndexSequence({x_range.size}), 0, i));
				i++;
			}
			output_vertices.push_back(v_add);
		} else {
			std::transform(
			    local_hemisphere_outputs.begin(), local_hemisphere_outputs.end(),
			    std::back_inserter(output_vertices), [](auto const& in) { return in; });
		}
	}

	// Concatenate all outputs
	auto const instance = execution_instance_manager.next_index();
	// Load all data
	std::vector<grenade::common::VertexOnTopology> local_inputs;
	local_inputs.reserve(output_vertices.size());
	i = 0;
	signal_flow::vertex::Transformation concatenation(
	    signal_flow::vertex::transformation::Concatenation(
	        signal_flow::ConnectionType::Int8, x_split_sizes),
	    instance);
	m_output_vertex = m_topology->add_vertex(std::move(concatenation));
	for (size_t i = 0; auto const& output : output_vertices) {
		m_topology->add_edge(
		    output, m_output_vertex,
		    grenade::common::Edge(
		        grenade::common::CuboidMultiIndexSequence({x_split_sizes.at(i)}),
		        grenade::common::CuboidMultiIndexSequence({x_split_sizes.at(i)}), 0, i));
		i++;
	}
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

	// fill topology inputs (with signal_flow::UInt5(0))
	if (batch_entry_size != input_size()) {
		throw std::runtime_error("Provided inputs size does not match MAC input size.");
	}

	hate::Timer input_timer;
	grenade::common::InputData input_data;
	std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>> timed_inputs(
	    inputs.size());
	for (size_t i = 0; i < inputs.size(); ++i) {
		timed_inputs.at(i).resize(1);
		// TODO: Think about what to do with timing information
		timed_inputs.at(i).at(0).data = inputs.at(i);
	}
	input_data.ports.set(
	    {m_input_vertex, 0}, signal_flow::vertex::Transformation::Dynamics(timed_inputs));
	std::vector<std::optional<common::Time>> runtimes(inputs.size());
	input_data.time_domain_runtimes.set(
	    grenade::common::TimeDomainOnTopology(),
	    network::abstract::ClockCycleTimeDomainRuntimes(runtimes, common::Time()));
	LOG4CXX_DEBUG(logger, "run(): input processing time: " << input_timer.print());

	for (auto const& [d, data] : m_parameterization.ports) {
		input_data.ports.set(d, data);
	}
	for (auto const& neuron_vertex : m_neuron_vertices) {
		auto const& vertex =
		    dynamic_cast<signal_flow::vertex::NeuronView const&>(m_topology->get(neuron_vertex));
		std::vector<signal_flow::vertex::NeuronView::Parameterization::Config> configs(
		    vertex.get_columns().size());
		for (size_t i = 0; i < configs.size(); ++i) {
			configs.at(i).enable_reset = true;
			configs.at(i).atomic_neuron_config = config.neuron_block.atomic_neurons.at(
			    AtomicNeuronOnDLS(vertex.get_columns().at(i), vertex.row));
		}
		signal_flow::vertex::NeuronView::Parameterization neuron_parameterization(configs);
		input_data.ports.set({neuron_vertex, 1}, neuron_parameterization);
	}
	for (auto const& [_, chip_vertex] : m_chip_vertices) {
		signal_flow::vertex::Chip::Parameterization chip_parameterization(config);
		input_data.ports.set({chip_vertex, 0}, chip_parameterization);
	}

	// run Graph with given inputs and return results
	auto const output_activation_map = execution::run(executor, m_topology, input_data);

	hate::Timer output_timer;
	auto const timed_outputs =
	    std::get<std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>(
	        dynamic_cast<signal_flow::vertex::Transformation::Results const&>(
	            output_activation_map.ports.get({m_output_vertex, 0}))
	            .value);
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
		for (auto const& port_data : output_activation_map.ports) {
			if (!dynamic_cast<signal_flow::vertex::Transformation::Results const*>(
			        &port_data.second)) {
				continue;
			}
			if (!std::holds_alternative<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
			        dynamic_cast<signal_flow::vertex::Transformation::Results const&>(
			            port_data.second)
			            .value)) {
				continue;
			}
			if (!std::holds_alternative<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
			        dynamic_cast<signal_flow::vertex::Transformation::Results const&>(
			            port_data.second)
			            .value)) {
				continue;
			}
			for (auto const& batch_entry :
			     std::get<std::vector<signal_flow::TimedSpikeFromChipSequence>>(
			         dynamic_cast<signal_flow::vertex::Transformation::Results const&>(
			             port_data.second)
			             .value)) {
				halco::common::typed_array<std::vector<common::Time>, HemisphereOnDLS> times;
				for (auto const& spike : batch_entry) {
					times[HemisphereOnDLS(spike.data.get_neuron_label() & (1 << 13) ? 1 : 0)]
					    .push_back(spike.time);
				}
				for (auto& hemisphere_times : times) {
					std::sort(hemisphere_times.begin(), hemisphere_times.end());
					for (size_t i = 1; i < hemisphere_times.size(); ++i) {
						acc(static_cast<intmax_t>(hemisphere_times.at(i)) -
						    static_cast<intmax_t>(hemisphere_times.at(i - 1)));
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
			    dynamic_cast<signal_flow::vertex::MADCReadoutView::Results const&>(
			        output_activation_map.ports.get({vertex, 0}))
			        .samples;
			for (size_t b = 0; b < output_activation_map.batch_size(); ++b) {
				auto const& local_madc_data = madc_data.at(b);
				for (auto const& sample : local_madc_data) {
					file << instance.execution_instance_id.value() << "\t" << b << "\t"
					     << sample.time.value() << "\t" << sample.data.value.value() << "\n";
				}
			}
		}
	}
	LOG4CXX_DEBUG(logger, "run(): output processing time: " << output_timer.print());
	return output;
}

} // namespace grenade::vx::compute
