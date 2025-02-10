#include "grenade/common/inter_topology_hyper_edge/projection.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/projection.h"

namespace grenade::common {

bool ProjectionInterTopologyHyperEdge::valid(
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& link_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
    LinkedTopology const& linked_topology) const
{
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const reference_projection = dynamic_cast<Projection const*>(
	        &linked_topology.get_reference().get(reference_vertex_descriptors.at(0)));
	    reference_projection) {
		auto const reference_input_sequence =
		    reference_projection->get_connector().get_input_sequence();
		auto const reference_output_sequence =
		    reference_projection->get_connector().get_output_sequence();
		for (auto const& link_vertex_descriptor : link_vertex_descriptors) {
			if (auto const link_projection =
			        dynamic_cast<Projection const*>(&linked_topology.get(link_vertex_descriptor));
			    link_projection) {
				if (!reference_input_sequence->includes(
				        *link_projection->get_connector().get_input_sequence())) {
					return false;
				}
				if (!reference_output_sequence->includes(
				        *link_projection->get_connector().get_output_sequence())) {
					return false;
				}
			} else {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::vector<std::vector<std::unique_ptr<PortData>>>
ProjectionInterTopologyHyperEdge::map_input_data(
    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
        reference_input_data,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
    LinkedTopology const& linked_topology) const
{
	auto const& reference_projection = dynamic_cast<Projection const&>(
	    linked_topology.get_reference().get(reference_vertex_descriptors.at(0)));

	std::vector<std::vector<std::unique_ptr<PortData>>> ret(linked_vertex_descriptors.size());
	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		auto const& link_projection =
		    dynamic_cast<Projection const&>(linked_topology.get(linked_vertex_descriptors.at(i)));
		ret.at(i).resize(reference_input_data.at(0).size());
		for (size_t j = 0; j < reference_input_data.at(0).size(); ++j) {
			if (reference_input_data.at(0).at(j)) {
				auto reference_shape =
				    reference_projection.get_connector().get_input_sequence()->cartesian_product(
				        *reference_projection.get_connector().get_output_sequence());

				// FIXME: also only works if link slice is reference sliced along link_shape.
				// Otherwise the real subset needs to be taken of the link connector
				auto link_shape =
				    link_projection.get_connector().get_input_sequence()->cartesian_product(
				        *link_projection.get_connector().get_output_sequence());

				if (*reference_projection.get_connector().get_section(*link_shape) !=
				    link_projection.get_connector()) {
					throw std::invalid_argument(
					    "Projection inter-topology hyper edge not implemented for non-rectangular "
					    "subsections of the connector.");
				}

				if (auto const reference_synapse_input_data =
				        dynamic_cast<Projection::Synapse::Dynamics const*>(
				            &reference_input_data.at(0).at(j).value().get());
				    reference_synapse_input_data) {
					ret.at(i).at(j) = reference_synapse_input_data->get_section(
					    *reference_projection.get_connector().get_synapse_parameterizations(
					        *link_shape));
				} else if (auto const reference_connector_input_data =
				               dynamic_cast<Projection::Connector::Dynamics const*>(
				                   &reference_input_data.at(0).at(j).value().get());
				           reference_connector_input_data) {
					ret.at(i).at(j) = reference_connector_input_data->get_section(*link_shape);
				} else if (auto const reference_synapse_input_data = dynamic_cast<
				               Projection::Synapse::ParameterSpace::Parameterization const*>(
				               &reference_input_data.at(0).at(j).value().get());
				           reference_synapse_input_data) {
					ret.at(i).at(j) = reference_synapse_input_data->get_section(
					    *reference_projection.get_connector().get_synapse_parameterizations(
					        *link_shape));
				} else if (auto const reference_connector_input_data =
				               dynamic_cast<Projection::Connector::Parameterization const*>(
				                   &reference_input_data.at(0).at(j).value().get());
				           reference_connector_input_data) {
					ret.at(i).at(j) = reference_connector_input_data->get_section(*link_shape);
				} else {
					throw std::invalid_argument("Input data type not implemented.");
				}
			}
		}
	}

	return ret;
}

std::unique_ptr<InterTopologyHyperEdge> ProjectionInterTopologyHyperEdge::copy() const
{
	return std::make_unique<ProjectionInterTopologyHyperEdge>(*this);
}

std::unique_ptr<InterTopologyHyperEdge> ProjectionInterTopologyHyperEdge::move()
{
	return std::make_unique<ProjectionInterTopologyHyperEdge>(std::move(*this));
}

bool ProjectionInterTopologyHyperEdge::is_equal_to(InterTopologyHyperEdge const&) const
{
	return true;
}

std::ostream& ProjectionInterTopologyHyperEdge::print(std::ostream& os) const
{
	return os << "ProjectionInterTopologyHyperEdge()";
}

} // namespace grenade::common
