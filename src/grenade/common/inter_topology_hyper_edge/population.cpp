#include "grenade/common/inter_topology_hyper_edge/population.h"

#include "grenade/common/linked_topology.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/population.h"
#include <stdexcept>

namespace grenade::common {

bool PopulationInterTopologyHyperEdge::valid(
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& link_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
    LinkedTopology const& topology) const
{
	if (reference_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const reference_population = dynamic_cast<Population const*>(
	        &topology.get_reference().get(reference_vertex_descriptors.at(0)));
	    reference_population) {
		for (auto const& link_vertex_descriptor : link_vertex_descriptors) {
			if (auto const link_population =
			        dynamic_cast<Population const*>(&topology.get(link_vertex_descriptor));
			    link_population) {
				if (!reference_population->get_shape().includes(link_population->get_shape())) {
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
PopulationInterTopologyHyperEdge::map_input_data(
    std::vector<std::vector<std::optional<std::reference_wrapper<PortData const>>>> const&
        reference_vertex_input_data,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& linked_vertex_descriptors,
    InterGraphHyperEdgeVertexDescriptors<VertexOnTopology> const& reference_vertex_descriptors,
    LinkedTopology const& topology) const
{
	auto const& reference_population = dynamic_cast<Population const&>(
	    topology.get_reference().get(reference_vertex_descriptors.at(0)));

	std::vector<std::vector<std::unique_ptr<PortData>>> ret(linked_vertex_descriptors.size());
	for (size_t i = 0; i < linked_vertex_descriptors.size(); ++i) {
		auto const& link_population =
		    dynamic_cast<Population const&>(topology.get(linked_vertex_descriptors.at(i)));
		ret.at(i).resize(reference_vertex_input_data.at(0).size());
		for (size_t j = 0; j < reference_vertex_input_data.at(0).size(); ++j) {
			if (reference_vertex_input_data.at(0).at(j)) {
				if (auto const dynamics = dynamic_cast<Population::Cell::Dynamics const*>(
				        &reference_vertex_input_data.at(0).at(j).value().get());
				    dynamics) {
					ret.at(i).at(j) = dynamics->get_section(
					    *CuboidMultiIndexSequence({reference_population.get_shape().size()})
					         .related_sequence_subset_restriction(
					             reference_population.get_shape(), link_population.get_shape()));
				} else if (auto const parameterization = dynamic_cast<
				               Population::Cell::ParameterSpace::Parameterization const*>(
				               &reference_vertex_input_data.at(0).at(j).value().get());
				           parameterization) {
					ret.at(i).at(j) = parameterization->get_section(
					    *CuboidMultiIndexSequence({reference_population.get_shape().size()})
					         .related_sequence_subset_restriction(
					             reference_population.get_shape(), link_population.get_shape()));
				} else {
					throw std::invalid_argument("Got unknown type of input data for population.");
				}
			}
		}
	}

	return ret;
}

std::unique_ptr<InterTopologyHyperEdge> PopulationInterTopologyHyperEdge::copy() const
{
	return std::make_unique<PopulationInterTopologyHyperEdge>(*this);
}

std::unique_ptr<InterTopologyHyperEdge> PopulationInterTopologyHyperEdge::move()
{
	return std::make_unique<PopulationInterTopologyHyperEdge>(std::move(*this));
}

bool PopulationInterTopologyHyperEdge::is_equal_to(InterTopologyHyperEdge const&) const
{
	return true;
}

std::ostream& PopulationInterTopologyHyperEdge::print(std::ostream& os) const
{
	return os << "PopulationInterTopologyHyperEdge()";
}

} // namespace grenade::common
