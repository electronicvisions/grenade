#include "grenade/vx/network/abstract/mapping/chip.h"

#include "grenade/common/linked_topology.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"
#include "grenade/vx/signal_flow/vertex/chip.h"


namespace grenade::vx::network::abstract {

ChipMapping::ChipMapping(std::optional<lola::vx::v3::Chip> base) : base(std::move(base)) {}


bool ChipMapping::valid(
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        linked_vertex_descriptors,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const& topology) const
{
	for (auto const& reference_vertex_descriptor : reference_vertex_descriptors) {
		if (auto const population = dynamic_cast<grenade::common::Population const*>(
		        &topology.get_reference().get(reference_vertex_descriptor));
		    population) {
			if (auto const neuron =
			        dynamic_cast<grenade::vx::network::abstract::LocallyPlacedNeuron const*>(
			            &population->get_cell());
			    !neuron) {
				return false;
			}
		}
	}
	if (linked_vertex_descriptors.size() != 1) {
		return false;
	}
	if (auto const chip = dynamic_cast<signal_flow::vertex::Chip const*>(
	        &topology.get(linked_vertex_descriptors.at(0)));
	    !chip) {
		return false;
	}
	return true;
}

std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> ChipMapping::map_input_data(
    std::vector<
        std::vector<std::optional<std::reference_wrapper<grenade::common::PortData const>>>> const&
        reference_vertex_parameterization,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&,
    grenade::common::InterGraphHyperEdgeVertexDescriptors<grenade::common::VertexOnTopology> const&
        reference_vertex_descriptors,
    grenade::common::LinkedTopology const&) const
{
	std::optional<lola::vx::v3::Chip> ret = base;
	for (size_t i = 0; i < reference_vertex_descriptors.size(); ++i) {
		if (auto const reference_neuron_parameterization =
		        dynamic_cast<UncalibratedNeuron::ParameterSpace::Parameterization const*>(
		            &reference_vertex_parameterization.at(i).at(1).value().get());
		    reference_neuron_parameterization) {
			for (auto const& [_, b] : reference_neuron_parameterization->base_configs) {
				if (!ret) {
					ret = b;
				} else {
					if (*ret != b) {
						throw std::invalid_argument("Base chip configs are heterogeneous.");
					}
				}
			}
		}
	}

	if (!ret) {
		throw std::logic_error("No base chip config given.");
	}

	std::vector<std::vector<std::unique_ptr<grenade::common::PortData>>> mapped_parameterizations;

	mapped_parameterizations.push_back({});
	mapped_parameterizations.back().emplace_back(
	    std::make_unique<signal_flow::vertex::Chip::Parameterization>(std::move(*ret)));

	return mapped_parameterizations;
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> ChipMapping::copy() const
{
	return std::make_unique<ChipMapping>(*this);
}

std::unique_ptr<grenade::common::InterTopologyHyperEdge> ChipMapping::move()
{
	return std::make_unique<ChipMapping>(std::move(*this));
}

std::ostream& ChipMapping::print(std::ostream& os) const
{
	os << "ChipMapping(";
	if (base) {
		os << *base;
	} else {
		os << "nullopt";
	}
	os << ")";
	return os;
}

bool ChipMapping::is_equal_to(InterTopologyHyperEdge const& other) const
{
	return base == static_cast<ChipMapping const&>(other).base;
}

} // namespace grenade::vx::network::abstract
