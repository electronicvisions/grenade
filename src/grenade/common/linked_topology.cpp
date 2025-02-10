#include "grenade/common/linked_topology.h"

#include "grenade/common/input_data.h"
#include "grenade/common/inter_topology_hyper_edge.h"
#include "grenade/common/inter_topology_hyper_edge_on_linked_topology.h"
#include "grenade/common/linked_graph_impl.tcc"
#include "grenade/common/output_data.h"
#include "grenade/common/port_data.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex.h"
#include "hate/indent.h"
#include "hate/join.h"
#include <stdexcept>

namespace grenade::common {

template struct LinkedGraph<
    InterTopologyHyperEdgeOnLinkedTopology,
    InterTopologyHyperEdge,
    Topology,
    Topology>;

LinkedTopology::LinkedTopology(std::shared_ptr<Topology const> reference) :
    LinkedGraph(), m_reference(std::move(reference))
{
}

InputData LinkedTopology::map_reference_input_data(InputData const& reference_input_data) const
{
	if (!reference_input_data.valid(get_reference())) {
		throw std::invalid_argument("Given input_data not valid for reference topology.");
	}
	InputData linked_input_data;
	linked_input_data.time_domain_runtimes = reference_input_data.time_domain_runtimes;
	for (auto const& inter_graph_hyper_edge_descriptor : inter_graph_hyper_edges()) {
		auto const& inter_graph_hyper_edge = get(inter_graph_hyper_edge_descriptor);
		auto const& reference_vertex_descriptors = references(inter_graph_hyper_edge_descriptor);
		auto const& link_vertex_descriptors = links(inter_graph_hyper_edge_descriptor);
		std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>>
		    reference_vertex_input_data;
		for (auto const& vertex_descriptor : reference_vertex_descriptors) {
			auto const input_ports = get_reference().get(vertex_descriptor).get_input_ports();
			reference_vertex_input_data.push_back({});
			for (size_t input_port_on_vertex = 0; input_port_on_vertex < input_ports.size();
			     ++input_port_on_vertex) {
				if (input_ports.at(input_port_on_vertex).requires_or_generates_data ==
				    Vertex::Port::RequiresOrGeneratesData::yes) {
					bool has_in_edge = false;
					for (auto const in_edge_descriptor :
					     get_reference().in_edges(vertex_descriptor)) {
						if (get_reference().get(in_edge_descriptor).port_on_target ==
						    input_port_on_vertex) {
							has_in_edge = true;
						}
					}
					if (!has_in_edge) {
						reference_vertex_input_data.back().emplace_back(
						    reference_input_data.ports.get(
						        std::pair{vertex_descriptor, input_port_on_vertex}));
					} else {
						reference_vertex_input_data.back().emplace_back(std::nullopt);
					}
				} else {
					reference_vertex_input_data.back().emplace_back(std::nullopt);
				}
			}
		}

		auto link_vertex_input_data = inter_graph_hyper_edge.map_input_data(
		    reference_vertex_input_data, link_vertex_descriptors, reference_vertex_descriptors,
		    *this);
		for (size_t i = 0; i < link_vertex_descriptors.size(); ++i) {
			for (size_t input_port_on_vertex = 0;
			     input_port_on_vertex < link_vertex_input_data.at(i).size();
			     ++input_port_on_vertex) {
				if (link_vertex_input_data.at(i).at(input_port_on_vertex)) {
					linked_input_data.ports.set(
					    std::pair{link_vertex_descriptors.at(i), input_port_on_vertex},
					    std::move(*link_vertex_input_data.at(i).at(input_port_on_vertex)));
				}
			}
		}
	}
	return linked_input_data;
}

OutputData LinkedTopology::map_reference_output_data(OutputData const& linked_output_data) const
{
	if (!linked_output_data.valid(*this)) {
		throw std::invalid_argument("Given output_data not valid for linked topology.");
	}
	OutputData reference_output_data;
	if (linked_output_data.has_global()) {
		reference_output_data.set_global(linked_output_data.get_global());
	}
	for (auto const& inter_graph_hyper_edge_descriptor : inter_graph_hyper_edges()) {
		auto const& inter_graph_hyper_edge = get(inter_graph_hyper_edge_descriptor);
		auto const& reference_vertex_descriptors = references(inter_graph_hyper_edge_descriptor);
		auto const& link_vertex_descriptors = links(inter_graph_hyper_edge_descriptor);
		std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>>
		    link_vertex_output_data;
		for (auto const& vertex_descriptor : link_vertex_descriptors) {
			auto const output_ports = get(vertex_descriptor).get_output_ports();
			link_vertex_output_data.push_back({});
			for (size_t output_port_on_vertex = 0; output_port_on_vertex < output_ports.size();
			     ++output_port_on_vertex) {
				if (output_ports.at(output_port_on_vertex).requires_or_generates_data ==
				    Vertex::Port::RequiresOrGeneratesData::yes) {
					link_vertex_output_data.back().emplace_back(linked_output_data.ports.get(
					    std::pair{vertex_descriptor, output_port_on_vertex}));
				} else {
					link_vertex_output_data.back().emplace_back(std::nullopt);
				}
			}
		}

		auto reference_vertex_output_data = inter_graph_hyper_edge.map_output_data(
		    link_vertex_output_data, link_vertex_descriptors, reference_vertex_descriptors, *this);
		for (size_t i = 0; i < reference_vertex_descriptors.size(); ++i) {
			auto const output_ports =
			    get_reference().get(reference_vertex_descriptors.at(i)).get_output_ports();
			for (size_t output_port_on_vertex = 0; output_port_on_vertex < output_ports.size();
			     ++output_port_on_vertex) {
				if (reference_vertex_output_data.at(i).at(output_port_on_vertex)) {
					reference_output_data.ports.set(
					    std::pair{reference_vertex_descriptors.at(i), output_port_on_vertex},
					    std::move(*reference_vertex_output_data.at(i).at(output_port_on_vertex)));
				}
			}
		}
	}
	return reference_output_data;
}

InputData LinkedTopology::map_root_input_data(InputData const& root_input_data) const
{
	if (auto const linked_reference = dynamic_cast<LinkedTopology const*>(&get_reference());
	    linked_reference) {
		return map_reference_input_data(linked_reference->map_root_input_data(root_input_data));
	}
	return map_reference_input_data(root_input_data);
}

OutputData LinkedTopology::map_root_output_data(OutputData const& linked_output_data) const
{
	if (auto const linked_reference = dynamic_cast<LinkedTopology const*>(&get_reference());
	    linked_reference) {
		return linked_reference->map_root_output_data(
		    map_reference_output_data(linked_output_data));
	}
	return map_reference_output_data(linked_output_data);
}

Topology const& LinkedTopology::get_reference() const
{
	if (!m_reference) {
		throw std::logic_error("Access to moved-from type.");
	}
	return *m_reference;
}

Topology const& LinkedTopology::get_root() const
{
	if (auto const linked_reference = dynamic_cast<LinkedTopology const*>(&get_reference());
	    linked_reference) {
		return linked_reference->get_root();
	}
	return get_reference();
}

std::shared_ptr<Topology const> LinkedTopology::_get_root() const
{
	if (auto const linked_reference = dynamic_cast<LinkedTopology const*>(&get_reference());
	    linked_reference) {
		return linked_reference->_get_root();
	}
	return m_reference;
}

LinkedTopology LinkedTopology::deepcopy() const
{
	LinkedTopology ret(*this);
	if (!m_reference) {
		return ret;
	}
	if (auto const linked_reference = dynamic_cast<LinkedTopology const*>(ret.m_reference.get());
	    linked_reference) {
		ret.m_reference = std::make_shared<LinkedTopology>(linked_reference->deepcopy());
	} else {
		ret.m_reference = std::make_shared<Topology>(*ret.m_reference);
	}
	return ret;
}

std::ostream& LinkedTopology::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "LinkedTopology(\n";
	ios << hate::Indentation("\t");
	ios << "link: ";
	Topology::print(ios) << "\n";
	ios << "inter_topology_hyper_edges:\n";
	ios << hate::Indentation("\t\t");
	for (auto const inter_graph_hyper_edge_descriptor : inter_graph_hyper_edges()) {
		auto const& reference_values = references(inter_graph_hyper_edge_descriptor);
		auto const& link_values = links(inter_graph_hyper_edge_descriptor);
		ios << inter_graph_hyper_edge_descriptor << " (references: ["
		    << hate::join(reference_values, ", ") << "], links: [" << hate::join(link_values, ", ")
		    << "]): " << get(inter_graph_hyper_edge_descriptor) << "\n";
	}
	ios << hate::Indentation("\t");
	ios << "reference: " << *m_reference << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::common
