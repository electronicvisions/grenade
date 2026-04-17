#include "grenade/vx/network/abstract/resource_estimator/population.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/population.h"
#include "grenade/common/resource_estimator.h"
#include "grenade/common/topology.h"
#include "grenade/common/vertex_on_topology.h"
#include "grenade/vx/network/abstract/population_cell/delay.h"
#include "grenade/vx/network/abstract/population_cell/external_source.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "grenade/vx/network/abstract/population_cell/poisson_source.h"
#include "grenade/vx/network/abstract/recorder/madc.h"
#include "hate/join.h"
#include <sstream>
#include <stdexcept>

namespace grenade::vx::network::abstract {

PopulationResourceEstimator::Resource::Resource(
    size_t neuron_circuit_count,
    size_t background_circuit_count,
    std::vector<size_t> madc_channel_usage,
    dapr::PropertyHolder<grenade::common::MultiIndexSequence> sequence) :
    neuron_circuit_count(neuron_circuit_count),
    background_circuit_count(background_circuit_count),
    madc_channel_usage(std::move(madc_channel_usage)),
    m_sequence(std::move(sequence))
{
}

grenade::common::ResourceEstimator::Resource& PopulationResourceEstimator::Resource::operator+=(
    ResourceEstimator::Resource const& other)
{
	if (auto const ptr = dynamic_cast<Resource const*>(&other); ptr) {
		neuron_circuit_count += ptr->neuron_circuit_count;
		background_circuit_count += ptr->background_circuit_count;
		madc_channel_usage.insert(
		    madc_channel_usage.end(), ptr->madc_channel_usage.begin(),
		    ptr->madc_channel_usage.end());
		m_sequence = dapr::PropertyHolder<grenade::common::MultiIndexSequence>();
		return *this;
	}
	throw std::invalid_argument("Resource types don't match.");
}

grenade::common::ResourceEstimator::Resource& PopulationResourceEstimator::Resource::operator*=(
    size_t factor)
{
	neuron_circuit_count *= factor;
	background_circuit_count *= factor;
	// no MADC channel usage scaling
	return *this;
}

std::vector<size_t> PopulationResourceEstimator::Resource::scalar_values() const
{
	size_t num_madc_channels = 0;
	for (auto const& entry : madc_channel_usage) {
		num_madc_channels += entry;
	}
	return {neuron_circuit_count, background_circuit_count, num_madc_channels};
}

std::unique_ptr<grenade::common::ResourceEstimator::Resource>
PopulationResourceEstimator::Resource::subsequence(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!m_sequence) {
		throw std::runtime_error("Subsequence not possible without resource sequence.");
	}
	if (!m_sequence->includes(sequence)) {
		std::stringstream ss;
		ss << "MultiIndexSequence given to subsequence not included in resource sequence:\n";
		ss << sequence << "\n";
		ss << m_sequence;
		throw std::invalid_argument(ss.str());
	}
	auto madc_subsequence_elements = grenade::common::CuboidMultiIndexSequence({m_sequence->size()})
	                                     .related_sequence_subset_restriction(*m_sequence, sequence)
	                                     ->get_elements();
	std::vector<size_t> madc_channel_usage_subsequence;
	madc_channel_usage_subsequence.reserve(madc_subsequence_elements.size());
	for (auto const& element : madc_subsequence_elements) {
		madc_channel_usage_subsequence.push_back(madc_channel_usage.at(element.value.at(0)));
	}
	return std::make_unique<Resource>(
	    std::ceil(static_cast<double>(neuron_circuit_count) * sequence.size() / m_sequence->size()),
	    std::ceil(
	        static_cast<double>(background_circuit_count) * sequence.size() / m_sequence->size()),
	    std::move(madc_channel_usage_subsequence), sequence);
}

std::unique_ptr<grenade::common::ResourceEstimator::Resource>
PopulationResourceEstimator::Resource::copy() const
{
	return std::make_unique<Resource>(*this);
}

std::unique_ptr<grenade::common::ResourceEstimator::Resource>
PopulationResourceEstimator::Resource::move()
{
	return std::make_unique<Resource>(std::move(*this));
}

bool PopulationResourceEstimator::Resource::is_equal_to(
    ResourceEstimator::Resource const& other) const
{
	auto const& other_resource = static_cast<Resource const&>(other);
	return neuron_circuit_count == other_resource.neuron_circuit_count &&
	       background_circuit_count == other_resource.background_circuit_count &&
	       madc_channel_usage == other_resource.madc_channel_usage &&
	       m_sequence == other_resource.m_sequence;
}

std::ostream& PopulationResourceEstimator::Resource::print(std::ostream& os) const
{
	os << "Resource(neuron_circuit_count: " << neuron_circuit_count
	   << ", background_circuit_count: " << background_circuit_count << ", madc_channel_usage: ["
	   << hate::join(madc_channel_usage, ", ") << "], sequence: ";
	if (m_sequence) {
		os << m_sequence;
	} else {
		os << "none";
	}
	os << ")";
	return os;
}


PopulationResourceEstimator::PopulationResourceEstimator(
    grenade::common::Topology const& topology,
    std::map<grenade::common::VertexOnTopology, double> overestimation) :
    ResourceEstimator(topology), m_overestimation(std::move(overestimation))
{
	for (auto const& [_, o] : overestimation) {
		if (o < 1.0) {
			throw std::invalid_argument("Expected overestimation to be at least a factor of one.");
		}
	}
}

std::unique_ptr<grenade::common::ResourceEstimator::Resource>
PopulationResourceEstimator::operator()(
    grenade::common::VertexOnTopology const& vertex_descriptor) const
{
	if (auto const section_ptr =
	        dynamic_cast<grenade::common::Population const*>(&m_topology.get(vertex_descriptor));
	    section_ptr) {
		double overestimation = 1.0;
		if (m_overestimation.contains(vertex_descriptor)) {
			overestimation = m_overestimation.at(vertex_descriptor);
		}
		auto const& sequence = section_ptr->get_shape();
		size_t const sequence_neuron_count = sequence.size();
		std::vector<size_t> madc_channel_usage(sequence_neuron_count);
		for (auto const& out_edge : m_topology.out_edges(vertex_descriptor)) {
			auto const target_descriptor = m_topology.target(out_edge);
			if (auto const madc_ptr =
			        dynamic_cast<MADCRecorder const*>(&m_topology.get(target_descriptor));
			    madc_ptr) {
				auto const& edge = m_topology.get(out_edge);
				std::set<size_t> cell_on_population_dimensions;
				for (size_t d = 0; d < sequence.dimensionality(); ++d) {
					cell_on_population_dimensions.insert(d);
				}
				auto const channels_on_source_elements =
				    edge.get_channels_on_source().get_elements();
				auto const sequence_elements = sequence.get_elements();
				std::map<grenade::common::MultiIndex, size_t> neuron_indices;
				for (size_t neuron_index = 0; auto const& sequence_element : sequence_elements) {
					neuron_indices.emplace(sequence_element, neuron_index);
					neuron_index++;
				}
				for (auto const& recorded_source : channels_on_source_elements) {
					grenade::common::MultiIndex recorded_source_neuron_element;
					for (size_t d = 0; d < recorded_source.value.size(); ++d) {
						if (cell_on_population_dimensions.contains(d)) {
							recorded_source_neuron_element.value.push_back(
							    recorded_source.value.at(d));
						}
					}
					madc_channel_usage[neuron_indices.at(recorded_source_neuron_element)] += 1;
				}
			}
		}
		if (auto const neuron_ptr =
		        dynamic_cast<LocallyPlacedNeuron const*>(&section_ptr->get_cell());
		    neuron_ptr) {
			auto const& compartments = neuron_ptr->shape.get_compartments();
			size_t neuron_circuit_count = 0;
			for (auto const& [_, atomic_neurons] : compartments) {
				neuron_circuit_count += atomic_neurons.size();
			}
			return std::make_unique<Resource>(
			    std::ceil(sequence_neuron_count * neuron_circuit_count * overestimation), 0,
			    std::move(madc_channel_usage), sequence);
		} else if (auto const neuron_ptr =
		               dynamic_cast<ExternalSourceNeuron const*>(&section_ptr->get_cell());
		           neuron_ptr) {
			return std::make_unique<Resource>(0, 0, std::move(madc_channel_usage), sequence);
		} else if (auto const neuron_ptr = dynamic_cast<DelayCell const*>(&section_ptr->get_cell());
		           neuron_ptr) {
			return std::make_unique<Resource>(0, 0, std::move(madc_channel_usage), sequence);
		} else if (auto const neuron_ptr =
		               dynamic_cast<PoissonSourceNeuron const*>(&section_ptr->get_cell());
		           neuron_ptr) {
			return std::make_unique<Resource>(
			    0, std::ceil(static_cast<double>(section_ptr->size()) / 256 * overestimation),
			    std::move(madc_channel_usage), sequence);
		}
	}
	return nullptr;
}

std::unique_ptr<grenade::common::ResourceEstimator> PopulationResourceEstimator::copy() const
{
	return std::make_unique<PopulationResourceEstimator>(*this);
}

} // namespace grenade::vx::network::abstract
