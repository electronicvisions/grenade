#include "grenade/vx/io_data_list.h"

#include "grenade/vx/graph.h"
#include "grenade/vx/io_data_map.h"

namespace grenade::vx {

void IODataList::from_output_map(IODataMap const& map, Graph const& graph, bool only_unconnected)
{
	data.clear();
	for (auto const v : boost::make_iterator_range(boost::vertices(graph.get_graph()))) {
		if (auto const ptr = std::get_if<vertex::DataOutput>(&graph.get_vertex_property(v));
		    ptr && ((only_unconnected && (boost::out_degree(v, graph.get_graph()) == 0)) ||
		            !only_unconnected)) {
			switch (ptr->output().type) {
				case ConnectionType::UInt32: {
					data.push_back(map.uint32.at(v));
					break;
				}
				case ConnectionType::UInt5: {
					data.push_back(map.uint5.at(v));
					break;
				}
				case ConnectionType::Int8: {
					data.push_back(map.int8.at(v));
					break;
				}
				case ConnectionType::DataOutputUInt16: {
					data.push_back(map.spike_event_output.at(v));
					break;
				}
				default: {
					throw std::logic_error("Data type handling not implemented.");
				}
			}
		}
	}
}

IODataMap IODataList::to_output_map(Graph const& graph, bool only_unconnected) const
{
	IODataMap map;
	auto it = data.begin();
	for (auto const v : boost::make_iterator_range(boost::vertices(graph.get_graph()))) {
		if (auto const ptr = std::get_if<vertex::DataOutput>(&graph.get_vertex_property(v));
		    ptr && ((only_unconnected && (boost::out_degree(v, graph.get_graph()) == 0)) ||
		            !only_unconnected)) {
			if (it == data.end()) {
				throw std::runtime_error("Reached unexpected end of data list data.");
			}
			switch (ptr->output().type) {
				case ConnectionType::UInt32: {
					map.uint32[v] = std::get<decltype(map.uint32)::mapped_type>(*it);
					break;
				}
				case ConnectionType::UInt5: {
					map.uint5[v] = std::get<decltype(map.uint5)::mapped_type>(*it);
					break;
				}
				case ConnectionType::Int8: {
					map.int8[v] = std::get<decltype(map.int8)::mapped_type>(*it);
					break;
				}
				case ConnectionType::DataOutputUInt16: {
					map.spike_event_output[v] =
					    std::get<decltype(map.spike_event_output)::mapped_type>(*it);
					break;
				}
				default: {
					throw std::logic_error("Data type handling not implemented.");
				}
			}
			++it;
		}
	}
	if (it != data.end()) {
		throw std::runtime_error("Reached unexpected end of conversion, list still has data.");
	}
	return map;
}

void IODataList::from_input_map(IODataMap const& map, Graph const& graph)
{
	data.clear();
	for (auto const v : boost::make_iterator_range(boost::vertices(graph.get_graph()))) {
		if (auto const ptr = std::get_if<vertex::ExternalInput>(&graph.get_vertex_property(v));
		    ptr) {
			switch (ptr->output().type) {
				case ConnectionType::UInt32: {
					data.push_back(map.uint32.at(v));
					break;
				}
				case ConnectionType::UInt5: {
					data.push_back(map.uint5.at(v));
					break;
				}
				case ConnectionType::Int8: {
					data.push_back(map.int8.at(v));
					break;
				}
				case ConnectionType::DataInputUInt16: {
					data.push_back(map.spike_events.at(v));
					break;
				}
				default: {
					throw std::logic_error("Data type handling not implemented.");
				}
			}
		}
	}
}

IODataMap IODataList::to_input_map(Graph const& graph) const
{
	IODataMap map;
	auto it = data.begin();
	for (auto const v : boost::make_iterator_range(boost::vertices(graph.get_graph()))) {
		if (auto const ptr = std::get_if<vertex::ExternalInput>(&graph.get_vertex_property(v));
		    ptr) {
			if (it == data.end()) {
				throw std::runtime_error("Reached unexpected end of list of data entries.");
			}
			switch (ptr->output().type) {
				case ConnectionType::UInt32: {
					map.uint32[v] = std::get<decltype(map.uint32)::mapped_type>(*it);
					break;
				}
				case ConnectionType::UInt5: {
					map.uint5[v] = std::get<decltype(map.uint5)::mapped_type>(*it);
					break;
				}
				case ConnectionType::Int8: {
					map.int8[v] = std::get<decltype(map.int8)::mapped_type>(*it);
					break;
				}
				case ConnectionType::DataInputUInt16: {
					map.spike_events[v] = std::get<decltype(map.spike_events)::mapped_type>(*it);
					break;
				}
				default: {
					throw std::logic_error("Data type handling not implemented.");
				}
			}
			++it;
		}
	}
	if (it != data.end()) {
		throw std::runtime_error("Reached unexpected end of conversion, list still has data.");
	}
	return map;
}

} // namespace grenade::vx
