#include "grenade/vx/execution/detail/execution_instance_snippet_realtime_executor.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/port_data.h"
#include "grenade/common/port_on_topology.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/execution/detail/execution_instance_chip_snippet_config_visitor.h"
#include "grenade/vx/execution/detail/generator/madc.h"
#include "grenade/vx/execution/detail/generator/ppu.h"
#include "grenade/vx/execution/detail/generator/timed_spike_to_chip_sequence.h"
#include "grenade/vx/ppu.h"
#include "grenade/vx/ppu/detail/extmem.h"
#include "grenade/vx/ppu/detail/status.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/cadc_membrane_readout_view.h"
#include "grenade/vx/signal_flow/vertex/madc_readout.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
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
#include <boost/type_index.hpp>
#include <log4cxx/logger.h>
#include <tbb/parallel_for_each.h>

namespace grenade::vx::execution::detail {

ExecutionInstanceSnippetRealtimeExecutor::ExecutionInstanceSnippetRealtimeExecutor(
    grenade::common::LinkedTopology const& topology,
    grenade::common::VertexOnTopology const& execution_instance_vertex_descriptor,
    std::vector<common::ChipOnConnection> const& chips_on_connection,
    grenade::common::InputData const& input_data,
    grenade::common::OutputData const& output_data,
    std::map<
        common::ChipOnConnection,
        std::reference_wrapper<std::optional<lola::vx::v3::PPUElfFile::symbols_type> const>> const&
        ppu_symbols,
    std::map<
        common::ChipOnConnection,
        std::map<signal_flow::vertex::PlasticityRule::ID, size_t>> const&
        timed_recording_index_offset) :
    m_topology(topology),
    m_execution_instance_vertex_descriptor(execution_instance_vertex_descriptor),
    m_input_data(input_data),
    m_data(std::make_unique<ExecutionInstanceSnippetData>(output_data)),
    m_post_vertices(std::make_unique<std::vector<grenade::common::VertexOnTopology>>()),
    m_chip_executors(),
    m_logger(log4cxx::Logger::getLogger("grenade.ExecutionInstanceSnippetRealtimeExecutor"))
{
	for (auto const& chip_on_connection : chips_on_connection) {
		m_chip_executors.emplace(
		    chip_on_connection,
		    ExecutionInstanceChipSnippetRealtimeExecutor(
		        m_topology, execution_instance_vertex_descriptor, m_input_data, *m_data,
		        *m_post_vertices, ppu_symbols.at(chip_on_connection).get(),
		        timed_recording_index_offset.at(chip_on_connection)));
	}
	register_process<signal_flow::vertex::NeuronView>();
	register_process<signal_flow::vertex::CrossbarL2Input>();
	register_process<signal_flow::vertex::CrossbarL2Output>();
	register_process<signal_flow::vertex::CADCMembraneReadoutView>();
	register_process<signal_flow::vertex::MADCReadoutView>();
	register_process<signal_flow::vertex::PlasticityRule>();
	register_process<signal_flow::vertex::Transformation>();
}

bool ExecutionInstanceSnippetRealtimeExecutor::inputs_available(
    grenade::common::VertexOnTopology const descriptor) const
{
	auto const edges = m_topology.get_reference().in_edges(descriptor);
	return std::all_of(edges.begin(), edges.end(), [&](auto const& edge) -> bool {
		auto const in_vertex = m_topology.get_reference().source(edge);
		return std::find(m_post_vertices->begin(), m_post_vertices->end(), in_vertex) ==
		       m_post_vertices->end();
	});
}

template <typename T>
void ExecutionInstanceSnippetRealtimeExecutor::register_process()
{
	m_visitor.emplace(
	    std::type_index(typeid(T)), [&](grenade::common::VertexOnTopology const& vertex_descriptor,
	                                    grenade::common::Vertex const& vertex) {
		    hate::Timer timer;
		    if constexpr (std::is_base_of_v<signal_flow::vertex::EntityOnChip, T>) {
			    auto const& t_vertex = static_cast<T const&>(vertex);
			    m_chip_executors.at(t_vertex.chip_on_connection)
			        .process(vertex_descriptor, t_vertex);
		    } else {
			    process(vertex_descriptor, static_cast<T const&>(vertex));
		    }
		    LOG4CXX_TRACE(
		        m_logger, "process(): Processed " << hate::name<hate::remove_all_qualifiers_t<T>>()
		                                          << " in " << timer.print() << ".");
	    });
}

template <typename T>
void ExecutionInstanceSnippetRealtimeExecutor::process(
    grenade::common::VertexOnTopology const /* vertex */, T const& /* data */)
{
	// Specialize for types which are not empty
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
    grenade::common::VertexOnTopology const vertex, signal_flow::vertex::Transformation const& data)
{
	// fill input value
	auto const input_ports = data.get_input_ports();
	std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>
	    value_input_optional(input_ports.size());
	// use list for stability of element location in memory
	std::list<signal_flow::vertex::Transformation::Dynamics> value_input_storage;
	for (size_t input_port_on_vertex = 0; input_port_on_vertex < input_ports.size();
	     ++input_port_on_vertex) {
		if (m_input_data.ports.contains({vertex, input_port_on_vertex})) {
			value_input_optional.at(input_port_on_vertex) =
			    std::cref(m_input_data.ports.get({vertex, input_port_on_vertex}));
		}
	}
	for (auto const& in_edge_descriptor : m_topology.get_reference().in_edges(vertex)) {
		auto const& in_edge = m_topology.get_reference().get(in_edge_descriptor);
		if (in_edge.get_channels_on_target().size() !=
		    input_ports.at(in_edge.port_on_target).get_channels().size()) {
			throw std::logic_error("Edge with port restriction unsupported.");
		}
		auto const& source_descriptor = m_topology.get_reference().source(in_edge_descriptor);
		auto const& source = m_topology.get_reference().get(source_descriptor);
		auto const source_port = source.get_output_ports().at(in_edge.port_on_source);
		assert(m_data);
		auto const& source_results =
		    m_data->at(grenade::common::PortOnTopology{source_descriptor, in_edge.port_on_source});
		if (auto const crossbar_l2_output_results =
		        dynamic_cast<signal_flow::vertex::CrossbarL2Output::Results const*>(
		            &source_results);
		    crossbar_l2_output_results) {
			value_input_storage.push_back(
			    signal_flow::vertex::Transformation::Dynamics(crossbar_l2_output_results->spikes));
		} else if (auto const madc_results =
		               dynamic_cast<signal_flow::vertex::MADCReadoutView::Results const*>(
		                   &source_results);
		           madc_results) {
			value_input_storage.push_back(
			    signal_flow::vertex::Transformation::Dynamics(madc_results->samples));
		} else if (auto const cadc_results =
		               dynamic_cast<signal_flow::vertex::CADCMembraneReadoutView::Results const*>(
		                   &source_results);
		           cadc_results) {
			value_input_storage.push_back(
			    signal_flow::vertex::Transformation::Dynamics(cadc_results->samples));
		} else if (auto const transformation_results =
		               dynamic_cast<signal_flow::vertex::Transformation::Results const*>(
		                   &source_results);
		           transformation_results) {
			value_input_storage.push_back(signal_flow::vertex::Transformation::Dynamics(
			    transformation_results
			        ->get_section(
			            *grenade::common::CuboidMultiIndexSequence(
			                 {source_port.get_channels().size()})
			                 .related_sequence_subset_restriction(
			                     source_port.get_channels(), in_edge.get_channels_on_source()))
			        ->value));
		} else {
			throw std::logic_error("Input data type not supported.");
		}
		value_input_optional.at(in_edge.port_on_target) = std::cref(value_input_storage.back());
	}
	if (!std::all_of(value_input_optional.begin(), value_input_optional.end(), [](auto const& o) {
		    return static_cast<bool>(o);
	    })) {
		std::stringstream ss;
		ss << "Not all input to transformation is provided:\n";
		ss << vertex << ": " << data;
		throw std::runtime_error(ss.str());
	}
	std::vector<std::reference_wrapper<grenade::common::PortData const>> value_input;
	for (auto const& vo : value_input_optional) {
		value_input.push_back(*vo);
	}

	// execute transformation
	auto value_output = data.get_function().apply(value_input);
	// process output value
	for (size_t i = 0; i < value_output.size(); ++i) {
		m_data->insert(
		    grenade::common::PortOnTopology{vertex, i},
		    signal_flow::vertex::Transformation::Results(std::move(value_output.at(i))));
	}
}

ExecutionInstanceSnippetRealtimeExecutor::Usages
ExecutionInstanceSnippetRealtimeExecutor::pre_process()
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceSnippetRealtimeExecutor");
	// Sequential preprocessing because vertices might depend on each other.
	assert(m_data);
	assert(m_post_vertices);
	for (auto const inter_topology_hyper_edge_descriptor :
	     m_topology.inter_graph_hyper_edges_by_linked(m_execution_instance_vertex_descriptor)) {
		for (auto const& vertex_descriptor :
		     m_topology.references(inter_topology_hyper_edge_descriptor)) {
			if (inputs_available(vertex_descriptor)) {
				auto const& vertex = m_topology.get_reference().get(vertex_descriptor);
				if (m_visitor.contains(std::type_index(typeid(vertex)))) {
					m_visitor.at(std::type_index(typeid(vertex)))(vertex_descriptor, vertex);
				}
			} else {
				m_post_vertices->push_back(vertex_descriptor);
			}
		}
	}

	Usages ret;
	for (auto& [chip_on_connection, chip_executor] : m_chip_executors) {
		ret.emplace(chip_on_connection, chip_executor.pre_process());
	}

	return ret;
}

ExecutionInstanceSnippetRealtimeExecutor::Result
ExecutionInstanceSnippetRealtimeExecutor::post_process(PostProcessable&& post_processable)
{
	auto logger = log4cxx::Logger::getLogger("grenade.ExecutionInstanceSnippetRealtimeExecutor");

	std::map<common::ChipOnConnection, std::chrono::nanoseconds> total_realtime_duration;
	for (auto& [chip_on_connection, chip_executor] : m_chip_executors) {
		total_realtime_duration[chip_on_connection] =
		    chip_executor.post_process(std::move(post_processable.at(chip_on_connection)));
	}

	assert(m_data);
	assert(m_post_vertices);
	for (auto const vertex_descriptor : *m_post_vertices) {
		auto const& vertex = m_topology.get_reference().get(vertex_descriptor);
		if (m_visitor.contains(std::type_index(typeid(vertex)))) {
			m_visitor.at(std::type_index(typeid(vertex)))(vertex_descriptor, vertex);
		}
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
