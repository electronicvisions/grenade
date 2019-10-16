#include "grenade/vx/execution_instance_builder.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "grenade/vx/data_map.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/helper.h"
#include "grenade/vx/types.h"
#include "haldls/vx/padi.h"

namespace grenade::vx {

ExecutionInstanceBuilder::ExecutionInstanceBuilder(
    Graph const& graph,
    DataMap const& input_list,
    DataMap const& data_output,
    halco::hicann_dls::vx::HemisphereOnDLS hemisphere,
    HemisphereConfig const& hemisphere_config) :
    m_graph(graph),
    m_input_list(input_list),
    m_data_output(data_output),
    m_local_external_data(),
    m_config(),
    m_builder_input(),
    m_builder_neuron_reset(),
    m_hemisphere(hemisphere),
    m_post_vertices(),
    m_local_data(),
    m_local_data_output(),
    m_ticket()
{
	*m_config = hemisphere_config;
	m_postprocessing = false;
}

bool ExecutionInstanceBuilder::inputs_available(Graph::vertex_descriptor const descriptor) const
{
	auto const edges = boost::make_iterator_range(boost::in_edges(descriptor, m_graph.get_graph()));
	return std::all_of(edges.begin(), edges.end(), [&](auto const& edge) -> bool {
		auto const in_vertex = boost::source(edge, m_graph.get_graph());
		return std::find(m_post_vertices.begin(), m_post_vertices.end(), in_vertex) ==
		       m_post_vertices.end();
	});
}

typename std::map<Graph::vertex_descriptor, std::vector<UInt5>>::const_reference
ExecutionInstanceBuilder::get_input_activations(Graph::vertex_descriptor descriptor)
{
	auto const& vertex_map = boost::get(Graph::VertexPropertyTag(), m_graph.get_graph());
	return std::visit(
	    [&](auto const& vertex) ->
	    typename std::map<Graph::vertex_descriptor, std::vector<UInt5>>::const_reference {
		    if (vertex.inputs().size() > 1) {
			    throw std::logic_error(
			        "get_input_activations can only be used with input count one.");
		    }
		    size_t const size_expected = vertex.inputs().front().size;
		    auto const edge =
		        *(boost::make_iterator_range(boost::in_edges(descriptor, m_graph.get_graph()))
		              .begin());
		    auto const in_vertex = boost::source(edge, m_graph.get_graph());
		    if (std::holds_alternative<vertex::ExternalInput>(vertex_map[in_vertex])) {
			    auto const& ret = *(m_local_external_data.uint5.find(in_vertex));
			    if (ret.second.size() != size_expected) {
				    throw std::logic_error(
				        "Shape of data to be loaded does not match expectation.");
			    }
			    return ret;
		    } else {
			    auto const& ret = *(m_data_output.uint5.find(in_vertex));
			    if (ret.second.size() != size_expected) {
				    throw std::logic_error(
				        "Shape of data to be loaded does not match expectation.");
			    }
			    return ret;
		    }
	    },
	    vertex_map[descriptor]);
}

std::vector<Int8> ExecutionInstanceBuilder::get_input_data(Graph::vertex_descriptor descriptor)
{
	auto const& vertex_map = boost::get(Graph::VertexPropertyTag(), m_graph.get_graph());
	return std::visit(
	    [&](auto const& vertex) -> std::vector<Int8> {
		    std::vector<Int8> activation(vertex.inputs().front().size);
		    size_t offset = 0;
		    for (auto const edge :
		         boost::make_iterator_range(boost::in_edges(descriptor, m_graph.get_graph()))) {
			    auto const in_vertex = boost::source(edge, m_graph.get_graph());
			    if (std::holds_alternative<vertex::ExternalInput>(vertex_map[in_vertex])) {
				    for (size_t i = 0; i < vertex.inputs().front().size; ++i) {
					    activation.at(i + offset) = m_local_external_data.int8.at(in_vertex).at(i);
				    }
				    offset += std::get<vertex::ExternalInput>(vertex_map[in_vertex]).output().size;
			    } else {
				    for (size_t i = 0; i < vertex.inputs().front().size; ++i) {
					    activation.at(i + offset) = m_data_output.int8.at(in_vertex).at(i);
				    }
				    offset += std::get<vertex::DataOutput>(vertex_map[in_vertex]).output().size;
			    }
		    }
		    return activation;
	    },
	    vertex_map[descriptor]);
}

template <typename F>
void ExecutionInstanceBuilder::visit_columns(Graph::vertex_descriptor const vertex, F&& f)
{
	// get column mapping from outgoing NeuronView(s)
	auto const out_edges = boost::out_edges(vertex, m_graph.get_graph());
	auto const& vertex_map = boost::get(Graph::VertexPropertyTag(), m_graph.get_graph());
	for (auto const out_edge : boost::make_iterator_range(out_edges)) {
		auto const& neuron_view =
		    std::get<vertex::NeuronView>(vertex_map[boost::target(out_edge, m_graph.get_graph())]);
		for (size_t i = 0; i < neuron_view.output().size; ++i) {
			f(i, (neuron_view.begin() + i)->toSynapseOnSynapseRow());
		}
	}
}

std::unordered_map<size_t, halco::hicann_dls::vx::SynapseOnSynapseRow>
ExecutionInstanceBuilder::get_column_map(Graph::vertex_descriptor const vertex)
{
	// get column mapping from outgoing NeuronView(s)
	auto const out_edges = boost::out_edges(vertex, m_graph.get_graph());
	auto const& vertex_map = boost::get(Graph::VertexPropertyTag(), m_graph.get_graph());
	std::unordered_map<size_t, halco::hicann_dls::vx::SynapseOnSynapseRow> column_map;
	size_t offset = 0;
	for (auto const out_edge : boost::make_iterator_range(out_edges)) {
		auto const& neuron_view =
		    std::get<vertex::NeuronView>(vertex_map[boost::target(out_edge, m_graph.get_graph())]);
		for (size_t i = 0; i < neuron_view.output().size; ++i) {
			column_map[i + offset] = (neuron_view.begin() + i)->toSynapseOnSynapseRow();
		}
		offset += neuron_view.output().size;
	}
	return column_map;
}

void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::SynapseArrayView const& data)
{
	auto& config = *m_config;
	// set synapse driver configuration
	for (auto const& synapse_row : data.synapse_rows()) {
		auto const synapse_driver = synapse_row.coordinate.toSynapseDriverOnSynapseDriverBlock();
		auto& synapse_driver_config = config.synapse_driver_block[synapse_driver];
		if (synapse_row.coordinate.toEnum() % 2) {
			synapse_driver_config.set_row_mode_top(synapse_row.mode);
		} else {
			synapse_driver_config.set_row_mode_bottom(synapse_row.mode);
		}
	}
	// set synaptic weights
	visit_columns(
	    vertex, [&](size_t const index, halco::hicann_dls::vx::SynapseOnSynapseRow const column) {
		    for (auto const& synapse_row : data.synapse_rows()) {
			    config.synapse_matrix.weights[synapse_row.coordinate][column] =
			        synapse_row.weights.at(index);
		    }
	    });
}

void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::DataInput const& data)
{
	using namespace lola::vx;
	using namespace haldls::vx;
	using namespace halco::hicann_dls::vx;
	using namespace halco::common;
	if (data.output().type == ConnectionType::SynapseInputLabel) {
		// get data input values
		auto const& [_, activations] = get_input_activations(vertex);

		// get row mapping from outgoing SynapseArrayView(s)
		auto& config = *m_config;
		auto const out_edges = boost::out_edges(vertex, m_graph.get_graph());
		auto const& vertex_map = boost::get(Graph::VertexPropertyTag(), m_graph.get_graph());
		for (auto const out_edge : boost::make_iterator_range(out_edges)) {
			auto const target = boost::target(out_edge, m_graph.get_graph());
			auto const& synapse_array_view = std::get<vertex::SynapseArrayView>(vertex_map[target]);
			for (size_t send = 0; send < synapse_array_view.num_sends; ++send) {
				auto const column_map = get_column_map(target);
				for (size_t i = 0; i < synapse_array_view.inputs().front().size; ++i) {
					// set synapse labels (bug workaround)
					auto const synapse_row =
					    (synapse_array_view.synapse_rows().begin() + i)->coordinate;
					auto const label = get_address(synapse_row, activations.at(i));
					if (label) {
						for (auto const& column : column_map) {
							config.synapse_matrix.labels[synapse_row][column.second] = *label;
						}

						// add event
						m_builder_input.write(
						    PADIEventOnDLS(m_hemisphere),
						    get_padi_event(
						        synapse_row.toSynapseDriverOnSynapseDriverBlock(), *label));
					}
				}
			}
		}
	} else if (data.output().type == ConnectionType::Int8) {
		m_local_data.int8[vertex] = get_input_data(vertex);
	} else {
		throw std::logic_error("DataInput output connection type processing not implemented.");
	}
}

void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::CADCMembraneReadoutView const& data)
{
	using namespace halco::hicann_dls::vx;
	using namespace lola::vx;
	if (!m_postprocessing) { // pre-hw-run processing
		if (!m_ticket) {
			m_ticket_baseline = m_builder_cadc_readout_baseline.read(
			    CADCSampleRowOnDLS(SynapseRowOnSynram(), SynramOnDLS(m_hemisphere)));
			m_ticket = m_builder_cadc_readout.read(
			    CADCSampleRowOnDLS(SynapseRowOnSynram(), SynramOnDLS(m_hemisphere)));
		}
		// results need hardware execution
		m_post_vertices.push_back(vertex);
	} else { // post-hw-run processing
		auto const& vertex_map = boost::get(Graph::VertexPropertyTag(), m_graph.get_graph());
		// extract Int8 values
		auto const cadc_sample_row = std::make_unique<CADCSampleRow>(m_ticket->get());
		auto const cadc_sample_row_baseline =
		    std::make_unique<CADCSampleRow>(m_ticket_baseline->get());
		std::vector<Int8> samples(data.output().size);

		// get samples via neuron mapping from incoming NeuronView(s)
		size_t offset = 0;
		auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
		for (auto const in_edge : boost::make_iterator_range(in_edges)) {
			auto const& neuron_view = std::get<vertex::NeuronView>(
			    vertex_map[boost::source(in_edge, m_graph.get_graph())]);
			for (size_t i = 0; i < neuron_view.output().size; ++i) {
				samples.at(i + offset) = Int8(
				    static_cast<intmax_t>(
				        cadc_sample_row->causal[(neuron_view.begin() + i)->toSynapseOnSynapseRow()]
				            .value()) -
				    static_cast<intmax_t>(
				        cadc_sample_row_baseline
				            ->causal[(neuron_view.begin() + i)->toSynapseOnSynapseRow()]
				            .value()));
			}
			offset += neuron_view.output().size;
		}
		m_local_data.int8[vertex] = samples;
	}
}

void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const /*vertex*/, vertex::NeuronView const& data)
{
	using namespace halco::hicann_dls::vx;
	using namespace haldls::vx;
	// TODO: This reset does not really belong here / how do we say when and whether it should
	// be triggered?
	// result: -> make property of each neuron view entry
	for (auto const neuron : data) {
		auto const neuron_reset =
		    AtomicNeuronOnDLS(neuron, NeuronRowOnDLS(m_hemisphere)).toNeuronResetOnDLS();
		m_builder_neuron_reset.write(neuron_reset, NeuronReset());
	}
	// TODO: once we have neuron configuration, it should be placed here
}

void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::ExternalInput const& data)
{
	if (data.output().type == ConnectionType::DataOutputUInt5) {
		m_local_external_data.uint5[vertex] = m_input_list.uint5.at(vertex);
	} else if (data.output().type == ConnectionType::DataOutputInt8) {
		m_local_external_data.int8[vertex] = m_input_list.int8.at(vertex);
	} else {
		throw std::runtime_error("ExternalInput output type processing not implemented.");
	}
}

void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::ConvertInt8ToSynapseInputLabel const& data)
{
	std::vector<UInt5> activations(data.output().size);

	// get input value mapping
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	size_t offset = 0;
	for (auto const in_edge : boost::make_iterator_range(in_edges)) {
		for (size_t i = 0; i < data.output().size; ++i) {
			// perform conversion
			activations.at(i + offset) = UInt5(static_cast<uint8_t>(
			    m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph())).at(i) >> 3));
		}
		offset += data.output().size;
	}
	m_local_data.uint5[vertex] = activations;
}

void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::Addition const& data)
{
	std::vector<Int8> values(data.output().size);
	std::fill(values.begin(), values.end(), Int8(0));
	auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
	for (size_t i = 0; i < data.output().size; ++i) {
		intmax_t tmp = 0;
		for (auto const in_edge : boost::make_iterator_range(in_edges)) {
			tmp += m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph())).at(i);
		}
		// restrict to range [-128,127]
		values.at(i) = Int8(std::min(std::max(tmp, intmax_t(-128)), intmax_t(127)));
	}
	m_local_data.int8[vertex] = values;
}

void ExecutionInstanceBuilder::process(
    Graph::vertex_descriptor const vertex, vertex::DataOutput const& data)
{
	if (data.inputs().front().type == ConnectionType::SynapseInputLabel) {
		// filter data output from activation values
		std::vector<UInt5> activations(data.output().size);

		// get activation mapping
		auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
		for (auto const in_edge : boost::make_iterator_range(in_edges)) {
			for (size_t i = 0; i < activations.size(); ++i) {
				// perform lookup
				activations.at(i) =
				    m_local_data.uint5.at(boost::source(in_edge, m_graph.get_graph())).at(i);
			}
		}
		m_local_data_output.uint5[vertex] = activations;
	} else if (data.inputs().front().type == ConnectionType::Int8) {
		std::vector<Int8> values(data.output().size);
		// get mapping
		auto const in_edges = boost::in_edges(vertex, m_graph.get_graph());
		for (auto const in_edge : boost::make_iterator_range(in_edges)) {
			for (size_t i = 0; i < values.size(); ++i) {
				// perform lookup
				values.at(i) =
				    m_local_data.int8.at(boost::source(in_edge, m_graph.get_graph())).at(i);
			}
		}
		m_local_data_output.int8[vertex] = values;
	} else {
		throw std::logic_error("DataOutput data type not implemented.");
	}
}

void ExecutionInstanceBuilder::process(Graph::vertex_descriptor const vertex)
{
	if (inputs_available(vertex)) {
		auto const& vertex_map = boost::get(Graph::VertexPropertyTag(), m_graph.get_graph());
		std::visit([&](auto const& value) { process(vertex, value); }, vertex_map[vertex]);
	} else {
		m_post_vertices.push_back(vertex);
	}
}

void ExecutionInstanceBuilder::post_process(Graph::vertex_descriptor const vertex)
{
	auto const& vertex_map = boost::get(Graph::VertexPropertyTag(), m_graph.get_graph());
	std::visit([&](auto const& value) { process(vertex, value); }, vertex_map[vertex]);
}

DataMap ExecutionInstanceBuilder::post_process()
{
	m_postprocessing = true;
	for (auto const vertex : m_post_vertices) {
		post_process(vertex);
	}
	m_postprocessing = false;
	m_ticket.reset();
	m_ticket_baseline.reset();
	m_post_vertices.clear();
	m_local_external_data.clear();
	m_local_data.clear();
	return std::move(m_local_data_output);
}

stadls::vx::PlaybackProgramBuilder ExecutionInstanceBuilder::generate()
{
	using namespace halco::common;
	using namespace halco::hicann_dls::vx;
	using namespace haldls::vx;
	using namespace stadls::vx;
	using namespace lola::vx;

	PlaybackProgramBuilder builder;
	PlaybackProgramBuilder driver_builder;
	// write static configuration
	if (!m_config.has_history()) {
		builder.write(SynramOnDLS(m_hemisphere), m_config->synapse_matrix);
		for (auto const synapse_driver : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
			driver_builder.write(
			    SynapseDriverOnDLS(synapse_driver, SynapseDriverBlockOnDLS(m_hemisphere)),
			    m_config->synapse_driver_block[synapse_driver]);
		}
	} else {
		builder.write(
		    SynramOnDLS(m_hemisphere), m_config->synapse_matrix,
		    m_config.get_history().synapse_matrix);
		for (auto const synapse_driver : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
			driver_builder.write(
			    SynapseDriverOnDLS(synapse_driver, SynapseDriverBlockOnDLS(m_hemisphere)),
			    m_config->synapse_driver_block[synapse_driver],
			    m_config.get_history().synapse_driver_block[synapse_driver]);
		}
	}
	m_config.save();
	// reset
	{
		auto const new_matrix = std::make_unique<lola::vx::SynapseMatrix>();
		m_config->synapse_matrix = *new_matrix;
	}
	for (auto const drv : iter_all<SynapseDriverOnSynapseDriverBlock>()) {
		m_config->synapse_driver_block[drv].set_row_mode_top(
		    haldls::vx::SynapseDriverConfig::RowMode::disabled);
		m_config->synapse_driver_block[drv].set_row_mode_bottom(
		    haldls::vx::SynapseDriverConfig::RowMode::disabled);
	}
	builder.merge_back(driver_builder);
	// reset neurons (baseline read)
	builder.copy_back(m_builder_neuron_reset);
	// wait sufficient amount of time (30us) before baseline reads for membrane to settle
	if (!builder.empty()) {
		builder.write(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer());
		builder.wait_until(
		    halco::hicann_dls::vx::TimerOnDLS(),
		    haldls::vx::Timer::Value(30 * haldls::vx::Timer::Value::fpga_clock_cycles_per_us));
	}
	// readout baselines of neurons
	builder.merge_back(m_builder_cadc_readout_baseline);
	// reset neurons
	builder.merge_back(m_builder_neuron_reset);
	// send input
	builder.merge_back(m_builder_input);
	// read out neuron membranes
	builder.merge_back(m_builder_cadc_readout);
	// wait for response data
	if (!builder.empty()) {
		builder.write(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer());
		builder.wait_until(halco::hicann_dls::vx::TimerOnDLS(), haldls::vx::Timer::Value(100000));
	}

	return builder;
}

} // namespace grenade::vx
