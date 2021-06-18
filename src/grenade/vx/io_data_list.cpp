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
			data.push_back(map.data.at(v));
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
			map.data[v] = *it;
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
			data.push_back(map.data.at(v));
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
			map.data[v] = *it;
			++it;
		}
	}
	if (it != data.end()) {
		throw std::runtime_error("Reached unexpected end of conversion, list still has data.");
	}
	return map;
}

} // namespace grenade::vx
