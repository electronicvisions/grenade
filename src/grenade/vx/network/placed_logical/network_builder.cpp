#include "grenade/vx/network/placed_logical/network_builder.h"

#include "hate/algorithm.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <set>
#include <log4cxx/logger.h>
#include <sys/socket.h>

namespace grenade::vx::network::placed_logical {

NetworkBuilder::NetworkBuilder() :
    m_populations(),
    m_projections(),
    m_duration(0),
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
	m_duration += std::chrono::microseconds(timer.get_us());
	return descriptor;
}

PopulationDescriptor NetworkBuilder::add(ExternalPopulation const& population)
{
	hate::Timer timer;
	PopulationDescriptor descriptor(m_populations.size());
	m_populations.insert({descriptor, population});
	LOG4CXX_TRACE(
	    m_logger,
	    "add(): Added external population(" << descriptor << ") in " << timer.print() << ".");
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
	    m_logger, "add(): Added background spike source population(" << descriptor << ") in "
	                                                                 << timer.print() << ".");
	m_duration += std::chrono::microseconds(timer.get_us());
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
	if (m_cadc_recording) {
		for (auto const& neuron : m_cadc_recording->neurons) {
			if (neuron.population == madc_recording.population &&
			    neuron.neuron_on_population == madc_recording.neuron_on_population &&
			    neuron.compartment_on_neuron == madc_recording.compartment_on_neuron &&
			    neuron.atomic_neuron_on_compartment ==
			        madc_recording.atomic_neuron_on_compartment &&
			    neuron.source != madc_recording.source) {
				throw std::runtime_error(
				    "MADC recording source conflicts with CADC recording's source.");
			}
		}
	}
	// check that plasticity rule target population readout settings don't contradict madc recording
	for (auto const& [_, plasticity_rule] : m_plasticity_rules) {
		for (auto const& target_population : plasticity_rule.populations) {
			auto const& population_neurons =
			    std::get<Population>(m_populations.at(target_population.descriptor)).neurons;
			if (target_population.descriptor != madc_recording.population) {
				continue;
			}
			for (size_t j = 0; j < population_neurons.size(); ++j) {
				if (j != madc_recording.neuron_on_population) {
					continue;
				}
				for (auto const& [compartment, denmems] :
				     target_population.neuron_readout_sources.at(j)) {
					if (compartment != madc_recording.compartment_on_neuron) {
						continue;
					}
					for (size_t k = 0; k < denmems.size(); ++k) {
						if (k == madc_recording.atomic_neuron_on_compartment && denmems.at(k) &&
						    *denmems.at(k) != madc_recording.source) {
							throw std::runtime_error(
							    "PlasticityRule neuron readout source doesn't match "
							    "MADC recording source for same neuron.");
						}
					}
				}
			}
		}
	}
	m_madc_recording = madc_recording;
	LOG4CXX_TRACE(m_logger, "add(): Added MADC recording in " << timer.print() << ".");
	m_duration += std::chrono::microseconds(timer.get_us());
}

void NetworkBuilder::add(CADCRecording const& cadc_recording)
{
	hate::Timer timer;
	if (m_cadc_recording) {
		throw std::runtime_error("Only one CADC recording per network possible.");
	}
	std::set<std::tuple<
	    PopulationDescriptor, size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron,
	    size_t>>
	    unique;
	for (auto const& neuron : cadc_recording.neurons) {
		if (!m_populations.contains(neuron.population)) {
			throw std::runtime_error("CADC recording references unknown population.");
		}
		if (!std::holds_alternative<Population>(m_populations.at(neuron.population))) {
			throw std::runtime_error("CADC recording does not reference internal population.");
		}
		if (neuron.neuron_on_population >=
		    std::get<Population>(m_populations.at(neuron.population)).neurons.size()) {
			throw std::runtime_error(
			    "CADC recording references neuron index out of range of population.");
		}
		if (m_madc_recording && neuron.population == m_madc_recording->population &&
		    neuron.neuron_on_population == m_madc_recording->neuron_on_population &&
		    neuron.compartment_on_neuron == m_madc_recording->compartment_on_neuron &&
		    neuron.atomic_neuron_on_compartment == m_madc_recording->atomic_neuron_on_compartment &&
		    neuron.source != m_madc_recording->source) {
			throw std::runtime_error(
			    "CADC recording source conflicts with MADC recording's source.");
		}
		unique.insert(
		    {neuron.population, neuron.neuron_on_population, neuron.compartment_on_neuron,
		     neuron.atomic_neuron_on_compartment});
	}
	if (unique.size() != cadc_recording.neurons.size()) {
		throw std::runtime_error("CADC recording neurons are not unique.");
	}
	m_cadc_recording = cadc_recording;
	LOG4CXX_TRACE(m_logger, "add(): Added CADC recording in " << timer.print() << ".");
	m_duration += std::chrono::microseconds(timer.get_us());
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

	// check that target projections fulfil source requirement
	if (plasticity_rule.enable_requires_one_source_per_row_in_order) {
		for (auto const& d : plasticity_rule.projections) {
			std::set<Projection::Connection::Index> rows;
			std::set<Projection::Connection::Index> columns;
			std::vector<std::pair<Projection::Connection::Index, Projection::Connection::Index>>
			    indices;
			auto const& projection = m_projections.at(d);
			for (auto const& connection : projection.connections) {
				rows.insert(connection.index_pre);
				columns.insert(connection.index_post);
				indices.push_back({connection.index_pre, connection.index_post});
			}
			if (rows.size() * columns.size() != indices.size()) {
				throw std::runtime_error("Not only one source per row in projection.");
			}
			if (!std::is_sorted(indices.begin(), indices.end())) {
				throw std::runtime_error("Sources of rows are not in order in projection.");
			}
			// If the source population is internal, the requirement can only be fulfilled if the
			// source population neuron event outputs on neuron event output block (or equivalently
			// the connected PADI-busses) are in order.
			if (std::holds_alternative<Population>(m_populations.at(projection.population_pre))) {
				auto const& population_pre =
				    std::get<Population>(m_populations.at(projection.population_pre));
				std::vector<halco::hicann_dls::vx::v3::AtomicNeuronOnDLS> atomic_neurons;
				for (auto const& neuron : population_pre.neurons) {
					auto const local_atomic_neurons = neuron.coordinate.get_atomic_neurons();
					atomic_neurons.insert(
					    atomic_neurons.end(), local_atomic_neurons.begin(),
					    local_atomic_neurons.end());
				}
				if (!std::is_sorted(
				        atomic_neurons.begin(), atomic_neurons.end(),
				        [](auto const& a, auto const& b) {
					        return a.toNeuronColumnOnDLS()
					                   .toNeuronEventOutputOnDLS()
					                   .toNeuronEventOutputOnNeuronBackendBlock() <
					               b.toNeuronColumnOnDLS()
					                   .toNeuronEventOutputOnDLS()
					                   .toNeuronEventOutputOnNeuronBackendBlock();
				        })) {
					throw std::runtime_error(
					    "Projection being in order can't be fulfilled since the source population "
					    "neurons don't project in order onto PADI-busses, which implies that the "
					    "synapse rows can't be allocated in order.");
				}
			}
		}
	}

	// check that target populations exist and are on-chip
	for (auto const& d : plasticity_rule.populations) {
		if (!m_populations.contains(d.descriptor)) {
			throw std::runtime_error(
			    "PlasticityRule population descriptor not present in network.");
		}
		if (!std::holds_alternative<Population>(m_populations.at(d.descriptor))) {
			throw std::runtime_error("PlasticityRule population not on chip.");
		}
	}

	// check that target population readout options match population shape
	for (auto const& d : plasticity_rule.populations) {
		auto const& population = std::get<Population>(m_populations.at(d.descriptor));
		auto const& readout_sources = d.neuron_readout_sources;
		if (population.neurons.size() != readout_sources.size()) {
			throw std::runtime_error("PlasticityRule population neuron readout source count "
			                         "doesn't match population neuron count.");
		}
		for (size_t i = 0; i < population.neurons.size(); ++i) {
			auto const& neuron = population.neurons.at(i).coordinate.get_placed_compartments();
			if (readout_sources.at(i).size() != neuron.size()) {
				throw std::runtime_error(
				    "PlasticityRule population neuron readout source compartment count "
				    "doesn't match population neuron compartment count.");
			}
			for (auto const& [compartment, denmem_coords] : neuron) {
				if (!readout_sources.at(i).contains(compartment) ||
				    readout_sources.at(i).at(compartment).size() != denmem_coords.size()) {
					throw std::runtime_error(
					    "PlasticityRule population neuron readout source compartments "
					    "don't match population neuron compartments.");
				}
			}
		}
	}

	// check that target population readout settings don't contradict madc recording
	if (m_madc_recording) {
		for (auto const& target_population : plasticity_rule.populations) {
			auto const& population_neurons =
			    std::get<Population>(m_populations.at(target_population.descriptor)).neurons;
			if (target_population.descriptor != m_madc_recording->population) {
				continue;
			}
			for (size_t j = 0; j < population_neurons.size(); ++j) {
				if (j != m_madc_recording->neuron_on_population) {
					continue;
				}
				for (auto const& [compartment, denmems] :
				     target_population.neuron_readout_sources.at(j)) {
					if (compartment != m_madc_recording->compartment_on_neuron) {
						continue;
					}
					for (size_t k = 0; k < denmems.size(); ++k) {
						if (k == m_madc_recording->atomic_neuron_on_compartment && denmems.at(k) &&
						    *denmems.at(k) != m_madc_recording->source) {
							throw std::runtime_error(
							    "PlasticityRule neuron readout source doesn't match "
							    "MADC recording source for same neuron.");
						}
					}
				}
			}
		}
	}

	PlasticityRuleDescriptor descriptor(m_plasticity_rules.size());
	m_plasticity_rules.insert({descriptor, plasticity_rule});
	LOG4CXX_TRACE(
	    m_logger, "add(): Added plasticity_rule(" << descriptor << ") in " << timer.print() << ".");
	m_duration += std::chrono::microseconds(timer.get_us());
	return descriptor;
}

std::shared_ptr<Network> NetworkBuilder::done()
{
	LOG4CXX_TRACE(m_logger, "done(): Finished building network.");
	auto const ret = std::make_shared<Network>(
	    std::move(m_populations), std::move(m_projections), std::move(m_madc_recording),
	    std::move(m_cadc_recording), std::move(m_plasticity_rules), m_duration);
	m_madc_recording.reset();
	m_cadc_recording.reset();
	m_duration = std::chrono::microseconds(0);
	assert(ret);
	LOG4CXX_DEBUG(m_logger, "done(): " << *ret);
	return ret;
}

} // namespace grenade::vx::network::placed_logical
