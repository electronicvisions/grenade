#include "grenade/vx/network/network_builder.h"

#include "hate/algorithm.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <set>
#include <log4cxx/logger.h>

namespace grenade::vx::network {

NetworkBuilder::NetworkBuilder() :
    m_populations(),
    m_projections(),
    m_duration(0),
    m_logger(log4cxx::Logger::getLogger("grenade.NetworkBuilder"))
{}

PopulationDescriptor NetworkBuilder::add(Population const& population)
{
	hate::Timer timer;
	// check that neuron and recording enable information count are equal
	if (population.neurons.size() != population.enable_record_spikes.size()) {
		throw std::runtime_error("Spike recorder enable mask not same size as neuron count.");
	}

	// check that supplied neurons are unique
	std::set<Population::Neurons::value_type> unique(
	    population.neurons.begin(), population.neurons.end());
	if (unique.size() != population.neurons.size()) {
		throw std::runtime_error("Neuron locations provided by Population are not unique.");
	}

	// check that supplied neurons don't overlap with already added populations
	for (auto const& [descriptor, other] : m_populations) {
		if (!std::holds_alternative<Population>(other)) {
			continue;
		}
		auto const& o = std::get<Population>(other);
		for (auto const& n : population.neurons) {
			if (std::find(o.neurons.begin(), o.neurons.end(), n) != o.neurons.end()) {
				std::stringstream ss;
				ss << "Neuron location(" << n
				   << ") provided by Population already present in other population(" << descriptor
				   << ").";
				throw std::runtime_error(ss.str());
			}
		}
	}

	PopulationDescriptor descriptor(m_populations.size());
	m_populations.insert({descriptor, population});
	LOG4CXX_TRACE(
	    m_logger, "add(): Added population(" << descriptor << ") in " << timer.print() << ".");
	m_duration += std::chrono::microseconds(timer.get_us());
	return descriptor;
}

PopulationDescriptor NetworkBuilder::add(ExternalPopulation const& population)
{
	hate::Timer timer;
	PopulationDescriptor descriptor(m_populations.size());
	m_populations.insert({descriptor, population});
	LOG4CXX_TRACE(m_logger, "add(): Added external population(" << descriptor << ").");
	m_duration += std::chrono::microseconds(timer.get_us());
	return descriptor;
}

PopulationDescriptor NetworkBuilder::add(BackgroundSpikeSourcePopulation const& population)
{
	hate::Timer timer;
	// check that supplied coordinate doesn't overlap with already added populations
	for (auto const& [descriptor, other] : m_populations) {
		if (!std::holds_alternative<BackgroundSpikeSourcePopulation>(other)) {
			continue;
		}
		auto const& o = std::get<BackgroundSpikeSourcePopulation>(other);
		for (auto const& [hemisphere, bus] : population.coordinate) {
			if (o.coordinate.contains(hemisphere) && o.coordinate.at(hemisphere) == bus) {
				std::stringstream ss;
				ss << "Background spike source location(" << hemisphere << ", " << bus
				   << ") provided by Population already present in other population(" << descriptor
				   << ").";
				throw std::runtime_error(ss.str());
			}
		}
	}
	if (population.config.enable_random) {
		// only power of two size supported
		if (population.size && (population.size & (population.size - 1))) {
			throw std::runtime_error(
			    "Population size " + std::to_string(population.size) +
			    " of background spike source needs to be power of two.");
		}
	} else {
		// only size one supported
		if (population.size != 1) {
			throw std::runtime_error(
			    "Population size " + std::to_string(population.size) +
			    " of background spike source needs to be one for regular spike train.");
		}
	}

	PopulationDescriptor descriptor(m_populations.size());
	m_populations.insert({descriptor, population});
	LOG4CXX_TRACE(
	    m_logger, "add(): Added background spike source population(" << descriptor << ").");
	m_duration += std::chrono::microseconds(timer.get_us());
	return descriptor;
}

ProjectionDescriptor NetworkBuilder::add(Projection const& projection)
{
	hate::Timer timer;
	auto const& population_pre = m_populations.at(projection.population_pre);
	auto const& population_post = m_populations.at(projection.population_post);

	// check that target population is internal population
	if (!std::holds_alternative<Population>(population_post)) {
		throw std::runtime_error(
		    "Projections with external post-synaptic population not supported.");
	}

	// check that no single connection index is out of range of its population
	auto const get_size = hate::overloaded(
	    [](Population const& p) { return p.neurons.size(); },
	    [](ExternalPopulation const& p) { return p.size; },
	    [](BackgroundSpikeSourcePopulation const& p) { return p.size; });
	auto const size_pre = std::visit(get_size, population_pre);
	auto const size_post = std::visit(get_size, population_post);

	for (auto const& connection : projection.connections) {
		if ((connection.index_pre > size_pre) || (connection.index_post > size_post)) {
			throw std::runtime_error("Connection index out of range of population(s).");
		}
	}

	// check that single connections connect unique pairs
	auto const get_unique_connections = [](auto const& connections) {
		std::set<std::pair<size_t, size_t>> unique_connections;
		for (auto const& c : connections) {
			unique_connections.insert({c.index_pre, c.index_post});
		}
		return unique_connections;
	};
	auto const unique_connections = get_unique_connections(projection.connections);
	if (unique_connections.size() != projection.connections.size()) {
		throw std::runtime_error(
		    "Projection to be added features same single connection multiple times.");
	}

	// check that if source is background the hemisphere matches
	if (std::holds_alternative<BackgroundSpikeSourcePopulation>(
	        m_populations.at(projection.population_pre))) {
		auto const& pre = std::get<BackgroundSpikeSourcePopulation>(population_pre);
		auto const& post = std::get<Population>(population_post);
		for (auto const& connection : projection.connections) {
			if (!pre.coordinate.contains(post.neurons.at(connection.index_post)
			                                 .toNeuronRowOnDLS()
			                                 .toHemisphereOnDLS())) {
				throw std::runtime_error(
				    "Source population of projection to be added can't reach target neuron(s).");
			}
		}
	}

	ProjectionDescriptor descriptor(m_projections.size());
	m_projections.insert({descriptor, projection});
	LOG4CXX_TRACE(
	    m_logger, "add(): Added projection(" << descriptor << ", " << projection.population_pre
	                                         << " -> " << projection.population_post << ") in "
	                                         << timer.print() << ".");
	m_duration += std::chrono::microseconds(timer.get_us());
	return descriptor;
}

void NetworkBuilder::add(MADCRecording const& madc_recording)
{
	hate::Timer timer;
	if (m_madc_recording) {
		throw std::runtime_error("Only one MADC recording per network possible.");
	}
	if (!m_populations.contains(madc_recording.population)) {
		throw std::runtime_error("MADC recording references unknown population.");
	}
	if (!std::holds_alternative<Population>(m_populations.at(madc_recording.population))) {
		throw std::runtime_error("MADC recording does not reference internal population.");
	}
	if (madc_recording.index >=
	    std::get<Population>(m_populations.at(madc_recording.population)).neurons.size()) {
		throw std::runtime_error(
		    "MADC recording references neuron index out of range of population.");
	}
	m_madc_recording = madc_recording;
	LOG4CXX_TRACE(m_logger, "add(): Added MADC recording in " << timer.print() << ".");
	m_duration += std::chrono::microseconds(timer.get_us());
}

std::shared_ptr<Network> NetworkBuilder::done()
{
	LOG4CXX_TRACE(m_logger, "done(): Finished building network.");
	auto const ret = std::make_shared<Network>(
	    std::move(m_populations), std::move(m_projections), std::move(m_madc_recording),
	    m_duration);
	m_madc_recording.reset();
	m_duration = std::chrono::microseconds(0);
	assert(ret);
	LOG4CXX_DEBUG(m_logger, "done(): " << *ret);
	return ret;
}

} // namespace grenade::vx::network
