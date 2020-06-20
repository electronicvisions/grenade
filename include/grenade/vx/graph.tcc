#include "grenade/vx/execution_instance.h"
#include "grenade/vx/input.h"
#include "hate/timer.h"

namespace grenade::vx {

template <typename VertexT>
Graph::vertex_descriptor Graph::add(
    VertexT&& vertex,
    coordinate::ExecutionInstance const execution_instance,
    std::vector<Input> const inputs)
{
	hate::Timer const timer;
	// check validity of inputs with regard to vertex to be added
	check_inputs(vertex, execution_instance, inputs);

	// add vertex to graph
	auto const descriptor = boost::add_vertex(m_graph);
	{
		auto const [_, success] =
		    m_vertex_property_map.emplace(descriptor, std::forward<VertexT>(vertex));
		if (!success) {
			throw std::logic_error("Adding vertex property graph unsuccessful.");
		}
	}

	// add edges
	add_edges(descriptor, execution_instance, inputs);

	// log successfull add operation of vertex
	add_log(descriptor, execution_instance, timer);

	return descriptor;
}

} // namespace grenade::vx
