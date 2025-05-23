#include "grenade/vx/execution/detail/execution_instance_snippet_realtime_executor.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_config_visitor.h"
#include "grenade/vx/execution/detail/generator/madc.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "grenade/vx/execution/detail/generator/timed_spike_to_chip_sequence.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/extmem.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/signal_flow/data_snippet.h"
#include "grenade/vx/signal_flow/input_data_snippet.h"
#include "grenade/vx/signal_flow/types.h"
#include "halco/hicann-dls/vx/hemisphere.h"
#include "haldls/vx/v3/barrier.h"
#include "haldls/vx/v3/block.h"
#include "haldls/vx/v3/fpga.h"
#include "haldls/vx/v3/padi.h"
#include "hate/math.h"
#include "hate/timer.h"
#include "hate/type_index.h"
#include "hate/type_traits.h"
#include "hate/variant.h"
#include "lola/vx/ppu.h"
#include "stadls/vx/constants.h"
#include "stadls/vx/playback_generator.h"
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <boost/range/combine.hpp>
#include <boost/sort/spinsort/spinsort.hpp>
#include <boost/type_index.hpp>
#include <log4cxx/logger.h>
#include <tbb/parallel_for_each.h>

namespace grenade::vx::execution::detail {

ExecutionInstanceSnippetRealtimeExecutor::ExecutionInstanceSnippetRealtimeExecutor(
    signal_flow::Graph const& graph,
    grenade::common::ExecutionInstanceID const& execution_instance,
    std::vector<common::ChipOnConnection> const& chips_on_connection,
    signal_flow::InputDataSnippet const& input_list,
    signal_flow::DataSnippet const& data_output,
    std::map<
        common::ChipOnConnection,
        std::reference_wrapper<std::optional<lola::vx::v3::PPUElfFile::symbols_type> const>> const&
        ppu_symbols,
    std::map<
        common::ChipOnConnection,
        std::map<signal_flow::vertex::PlasticityRule::ID, size_t>> const&
        timed_recording_index_offset) :
    m_graph(graph),
    m_execution_instance(execution_instance),
    m_data(std::make_unique<ExecutionInstanceSnippetData>(input_list, data_output)),
    m_post_vertices(std::make_unique<std::vector<signal_flow::Graph::vertex_descriptor>>()),
    m_chip_executors()
{
	for (auto const& chip_on_connection : chips_on_connection) {
		m_chip_executors.emplace(
		    chip_on_connection, ExecutionInstanceChipSnippetRealtimeExecutor(
		                            graph, execution_instance, *m_data, *m_post_vertices,
		                            ppu_symbols.at(chip_on_connection).get(),
		                            timed_recording_index_offset.at(chip_on_connection)));
	}
	// check that input list provides all requested input for local graph
	if (!input_data_matches_graph(input_list)) {
		throw std::runtime_error("Graph requests unprovided input.");
	}
}

bool ExecutionInstanceSnippetRealtimeExecutor::input_data_matches_graph(
    signal_flow::InputDataSnippet const& input_data) const
{
	if (input_data.empty()) {
		return true;
	}
	if (!input_data.valid()) {
		return false;
	}
	auto const batch_size = input_data.batch_size();
	auto const execution_instance_vertex =
	    m_graph.get_execution_instance_map().right.at(m_execution_instance);
	auto const vertices = boost::make_iterator_range(
	    m_graph.get_vertex_descriptor_map().right.equal_range(execution_instance_vertex));
	return std::none_of(vertices.begin(), vertices.end(), [&](auto const& p) {
		auto const vertex = p.second;
		if (std::holds_alternative<signal_flow::vertex::ExternalInput>(
		        m_graph.get_vertex_property(vertex))) {
			if (input_data.data.find(vertex) == input_data.data.end()) {
				return true;
			}
			if (batch_size == 0) {
				return false;
			}
			auto const& input_vertex =
			    std::get<signal_flow::vertex::ExternalInput>(m_graph.get_vertex_property(vertex));
			return !signal_flow::DataSnippet::is_match(
			    input_data.data.find(vertex)->second, input_vertex.output());
		}
		return false;
	});
}

bool ExecutionInstanceSnippetRealtimeExecutor::inputs_available(
    signal_flow::Graph::vertex_descriptor const descriptor) const
{
	auto const edges = boost::make_iterator_range(boost::in_edges(descriptor, m_graph.get_graph()));
	return std::all_of(edges.begin(), edges.end(), [&](auto const& edge) -> bool {
		auto const in_vertex = boost::source(edge, m_graph.get_graph());
		return std::find(m_post_vertices->begin(), m_post_vertices->end(), in_vertex) ==
		       m_post_vertices->end();
	});
}

template <typename T>
void ExecutionInstanceSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */, T const& /* data */)
{
	// Specialize for types which are not empty
}

namespace {

template <typename T>
std::vector<common::TimedDataSequence<std::vector<T>>> apply_restriction(
    std::vector<common::TimedDataSequence<std::vector<T>>> const& value,
    signal_flow::PortRestriction const& restriction)
{
	std::vector<common::TimedDataSequence<std::vector<T>>> ret(value.size());
	for (size_t b = 0; b < ret.size(); ++b) {
		auto& local_ret = ret.at(b);
		auto const& local_value = value.at(b);
		local_ret.resize(local_value.size());
		for (size_t bb = 0; bb < local_value.size(); ++bb) {
			local_ret.at(bb).data.insert(
			    local_ret.at(bb).data.end(), local_value.at(bb).data.begin() + restriction.min(),
			    local_value.at(bb).data.begin() + restriction.max() + 1);
			local_ret.at(bb).time = local_value.at(bb).time;
		}
	}
	return ret;
}

} // namespace

template <>
void ExecutionInstanceSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::DataInput const& /* data */)
{
	using namespace lola::vx::v3;
	using namespace haldls::vx::v3;
	using namespace halco::hicann_dls::vx::v3;
	using namespace halco::common;

	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
	auto const in_vertex = boost::source(edge, m_graph.get_graph());
	auto const& input_values = m_data->at(in_vertex);

	auto const insert_in_local_data = [&](auto const& d) {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>
		    MatrixData;
		if constexpr (hate::is_in_type_list<Data, MatrixData>::value) {
			auto const port_restriction = m_graph.get_edge_property_map().at(edge);
			if (port_restriction) {
				m_data->insert(vertex, apply_restriction(d, *port_restriction));
			}
		}
		m_data->insert(vertex, in_vertex);
	};
	std::visit(insert_in_local_data, input_values);
}

namespace {

template <typename T>
void resize_rectangular(std::vector<std::vector<T>>& data, size_t size_outer, size_t size_inner)
{
	data.resize(size_outer);
	for (auto& inner : data) {
		inner.resize(size_inner);
	}
}

} // namespace

template <>
void ExecutionInstanceSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const /* vertex */,
    signal_flow::vertex::ExternalInput const& /* data */)
{
}

template <>
void ExecutionInstanceSnippetRealtimeExecutor::process(
    [[maybe_unused]] signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::DataOutput const& data)
{
	// get in edge
	assert(boost::in_degree(vertex, m_graph.get_graph()) == 1);
	auto const in_edge = *(boost::in_edges(vertex, m_graph.get_graph()).first);
	auto const in_vertex = boost::source(in_edge, m_graph.get_graph());
	auto const& local_data = m_data->at(in_vertex);
	// maybe apply port restriction
	auto const insert_in_local_data = [&](auto const& d) {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>
		    MatrixData;
		if constexpr (hate::is_in_type_list<Data, MatrixData>::value) {
			auto const port_restriction = m_graph.get_edge_property_map().at(in_edge);
			if (port_restriction) {
				auto const restricted_local_data = apply_restriction(d, *port_restriction);
				// check size match only for first because we know that the data map is valid
				assert(signal_flow::DataSnippet::is_match(restricted_local_data, data.output()));
				m_data->insert(vertex, restricted_local_data, true);
				return;
			}
		}
		m_data->insert(vertex, in_vertex, true);
	};
	std::visit(insert_in_local_data, local_data);
}

template <>
void ExecutionInstanceSnippetRealtimeExecutor::process(
    signal_flow::Graph::vertex_descriptor const vertex,
    signal_flow::vertex::Transformation const& data)
{
	// fill input value
	auto const inputs = data.inputs();
	std::vector<signal_flow::vertex::Transformation::Function::Value> value_input;
	auto edge_it = boost::in_edges(vertex, m_graph.get_graph()).first;
	for (auto const& port : inputs) {
		if (m_graph.get_edge_property_map().at(*edge_it)) {
			throw std::logic_error("Edge with port restriction unsupported.");
		}
		auto const in_vertex = boost::source(*edge_it, m_graph.get_graph());
		auto const& input_values = m_data->at(in_vertex);
		if (!signal_flow::DataSnippet::is_match(input_values, port)) {
			throw std::runtime_error("Data size does not match expectation.");
		}
		value_input.push_back(input_values);
		edge_it++;
	}
	// execute transformation
	auto value_output = data.apply(value_input);
	// process output value
	if (!signal_flow::DataSnippet::is_match(value_output, data.output())) {
		throw std::runtime_error("Data size does not match expectation.");
	}
	m_data->insert(vertex, std::move(value_output));
}

ExecutionInstanceSnippetRealtimeExecutor::Usages
ExecutionInstanceSnippetRealtimeExecutor::pre_process()
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceSnippetRealtimeExecutor");
	auto const execution_instance_vertex =
	    m_graph.get_execution_instance_map().right.at(m_execution_instance);
	// Sequential preprocessing because vertices might depend on each other.
	assert(m_data);
	assert(m_post_vertices);
	for (auto const p : boost::make_iterator_range(
	         m_graph.get_vertex_descriptor_map().right.equal_range(execution_instance_vertex))) {
		auto const vertex = p.second;
		if (inputs_available(vertex)) {
			std::visit(
			    [&](auto const& value) {
				    hate::Timer timer;
				    typedef hate::remove_all_qualifiers_t<decltype(value)> vertex_type;
				    if constexpr (std::is_base_of_v<common::EntityOnChip, vertex_type>) {
					    m_chip_executors.at(value.chip_on_executor.first).process(vertex, value);
				    } else {
					    process(vertex, value);
				    }
				    LOG4CXX_TRACE(
				        logger, "process(): Preprocessed " << hate::name<vertex_type>() << " in "
				                                           << timer.print() << ".");
			    },
			    m_graph.get_vertex_property(vertex));
		} else {
			m_post_vertices->push_back(vertex);
		}
	}

	Usages ret;
	for (auto& [chip_on_connection, chip_executor] : m_chip_executors) {
		ret.emplace(chip_on_connection, chip_executor.pre_process());
	}

	return ret;
}

ExecutionInstanceSnippetRealtimeExecutor::Result
ExecutionInstanceSnippetRealtimeExecutor::post_process(PostProcessable const& post_processable)
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceSnippetRealtimeExecutor");

	std::map<common::ChipOnConnection, std::chrono::nanoseconds> total_realtime_duration;
	for (auto& [chip_on_connection, chip_executor] : m_chip_executors) {
		total_realtime_duration[chip_on_connection] =
		    chip_executor.post_process(post_processable.at(chip_on_connection));
	}

	assert(m_data);
	assert(m_post_vertices);
	for (auto const vertex : *m_post_vertices) {
		std::visit(
		    [&](auto const& value) {
			    hate::Timer timer;
			    typedef hate::remove_all_qualifiers_t<decltype(value)> vertex_type;
			    if constexpr (std::is_base_of_v<common::EntityOnChip, vertex_type>) {
				    m_chip_executors.at(value.chip_on_executor.first).process(vertex, value);
			    } else {
				    process(vertex, value);
			    }
			    LOG4CXX_TRACE(
			        logger, "process(): Postprocessed " << hate::name<vertex_type>() << " in "
			                                            << timer.print() << ".");
		    },
		    m_graph.get_vertex_property(vertex));
	}

	return {std::move(m_data->done()), total_realtime_duration};
}

ExecutionInstanceSnippetRealtimeExecutor::Program
ExecutionInstanceSnippetRealtimeExecutor::generate(
    ExecutionInstanceSnippetRealtimeExecutor::Usages before,
    ExecutionInstanceSnippetRealtimeExecutor::Usages after)
{
	Program program;
	for (auto& [chip_on_connection, chip_executor] : m_chip_executors) {
		program.emplace(
		    chip_on_connection,
		    chip_executor.generate(before.at(chip_on_connection), after.at(chip_on_connection)));
	}
	return program;
}

} // namespace grenade::vx::execution::detail
