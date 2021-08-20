#include "grenade/vx/logical_network/network_builder.h"

#include "hate/algorithm.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <set>
#include <log4cxx/logger.h>
#include <sys/socket.h>

namespace grenade::vx::logical_network {

NetworkBuilder::NetworkBuilder() :
    m_populations(),
    m_projections(),
    m_logger(log4cxx::Logger::getLogger("grenade.logical_network.NetworkBuilder"))
{}

PopulationDescriptor NetworkBuilder::add(Population const& population)
{
	hate::Timer timer;
	// check population is valid
	if (!population.valid()) {
		throw std::runtime_error("Population is not valid.");
	}

	// check that supplied neurons don't overlap with already added populations
	auto const atomic_neurons = population.get_atomic_neurons();
	for (auto const& [descriptor, other] : m_populations) {
		if (!std::holds_alternative<Population>(other)) {
			continue;
		}
		auto const& o = std::get<Population>(other);
		for (auto const& n : atomic_neurons) {
			auto const o_neurons = o.get_atomic_neurons();
			if (std::find(o_neurons.begin(), o_neurons.end(), n) != o_neurons.end()) {
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
	return descriptor;
}

PopulationDescriptor NetworkBuilder::add(ExternalPopulation const& population)
{
	PopulationDescriptor descriptor(m_populations.size());
	m_populations.insert({descriptor, population});
	LOG4CXX_TRACE(m_logger, "add(): Added external population(" << descriptor << ").");
	return descriptor;
}

PopulationDescriptor NetworkBuilder::add(BackgroundSpikeSourcePopulation const& population)
{
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
	return descriptor;
}

ProjectionDescriptor NetworkBuilder::add(Projection const& projection)
{
	hate::Timer timer;
	auto const& population_pre = m_populations.at(projection.population_pre);

	// check that target population is not external
	if (!std::holds_alternative<Population>(m_populations.at(projection.population_post))) {
		throw std::runtime_error("Only projections with on-chip neuron population are supported.");
	}

	auto const& population_post =
	    std::get<Population>(m_populations.at(projection.population_post));

	// check that no single connection index is out of range of its population
	auto const get_size = hate::overloaded(
	    [](Population const& p) { return p.neurons.size(); }, [](auto const& p) { return p.size; });
	auto const size_pre = std::visit(get_size, population_pre);
	auto const size_post = get_size(population_post);

	auto const contains =
	    [](Population const& population,
	       std::pair<size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron> index) {
		    return population.neurons.at(index.first)
		        .coordinate.get_placed_compartments()
		        .contains(index.second);
	    };

	for (auto const& connection : projection.connections) {
		if ((connection.index_pre.first > size_pre) || (connection.index_post.first > size_post)) {
			throw std::runtime_error("Connection index out of range of population(s).");
		}
		if (std::holds_alternative<Population>(population_pre)) {
			auto const& population = std::get<Population>(population_pre);
			if (!contains(population, connection.index_pre)) {
				throw std::runtime_error("Connection index compartment out of range of neuron.");
			}
		} else {
			if (connection.index_pre.second !=
			    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron(0)) {
				throw std::runtime_error(
				    "Only on-chip population supports multiple compartments per neuron.");
			}
		}
		if (!contains(population_post, connection.index_post)) {
			throw std::runtime_error("Connection index compartment out of range of neuron.");
		}
	}

	// check that receptor is available for all post neurons
	for (auto const& connection : projection.connections) {
		auto const& receptors = population_post.neurons.at(connection.index_post.first)
		                            .compartments.at(connection.index_post.second)
		                            .receptors;
		if (!std::any_of(
		        receptors.begin(), receptors.end(),
		        [receptor = projection.receptor](auto const& r) { return r.contains(receptor); })) {
			throw std::runtime_error("Neuron does not feature receptor requested by projection.");
		}
	}

	// check that single connections connect unique pairs
	auto const get_unique_connections = [](auto const& connections) {
		std::set<std::pair<Projection::Connection::Index, Projection::Connection::Index>>
		    unique_connections;
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
		for (auto const& connection : projection.connections) {
			auto const& receptors = population_post.neurons.at(connection.index_post.first)
			                            .compartments.at(connection.index_post.second)
			                            .receptors;
			auto const& placed_compartments =
			    population_post.neurons.at(connection.index_post.first)
			        .coordinate.get_placed_compartments();
			auto const& placed_compartment_neurons =
			    placed_compartments.at(connection.index_post.second);
			bool found_target_neuron = false;
			for (size_t i = 0; i < placed_compartment_neurons.size(); ++i) {
				if (pre.coordinate.contains(
				        placed_compartment_neurons.at(i).toNeuronRowOnDLS().toHemisphereOnDLS()) &&
				    receptors.at(i).contains(projection.receptor)) {
					found_target_neuron = true;
					break;
				}
			}
			if (!found_target_neuron) {
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
	if (madc_recording.neuron_on_population >=
	    std::get<Population>(m_populations.at(madc_recording.population)).neurons.size()) {
		throw std::runtime_error(
		    "MADC recording references neuron index out of range of population.");
	}
	if (!std::get<Population>(m_populations.at(madc_recording.population))
	         .neurons.at(madc_recording.neuron_on_population)
	         .coordinate.get_placed_compartments()
	         .contains(madc_recording.compartment_on_neuron)) {
		throw std::runtime_error("MADC recording references non-existent neuron compartment.");
	}
	if (madc_recording.atomic_neuron_on_compartment >=
	    std::get<Population>(m_populations.at(madc_recording.population))
	        .neurons.at(madc_recording.neuron_on_population)
	        .coordinate.get_placed_compartments()
	        .at(madc_recording.compartment_on_neuron)
	        .size()) {
		throw std::runtime_error(
		    "MADC recording references atomic neuron index out of range of neuron.");
	}
	m_madc_recording = madc_recording;
	LOG4CXX_TRACE(m_logger, "add(): Added MADC recording in " << timer.print() << ".");
}

PlasticityRuleDescriptor NetworkBuilder::add(PlasticityRule const& plasticity_rule)
{
	hate::Timer timer;

	// check that target projections exist
	for (auto const& d : plasticity_rule.projections) {
		if (!m_projections.contains(d)) {
			throw std::runtime_error(
			    "PlasticityRule projection descriptor not present in network.");
		}
	}

	PlasticityRuleDescriptor descriptor(m_plasticity_rules.size());
	m_plasticity_rules.insert({descriptor, plasticity_rule});
	LOG4CXX_TRACE(
	    m_logger, "add(): Added plasticity_rule(" << descriptor << ") in " << timer.print() << ".");
	return descriptor;
}

std::shared_ptr<Network> NetworkBuilder::done()
{
	LOG4CXX_TRACE(m_logger, "done(): Finished building network.");
	auto const ret = std::make_shared<Network>(
	    std::move(m_populations), std::move(m_projections), std::move(m_madc_recording),
	    std::move(m_plasticity_rules));
	m_madc_recording.reset();
	assert(ret);
	LOG4CXX_DEBUG(m_logger, "done(): " << *ret);
	return ret;
}

} // namespace grenade::vx::logical_network
