#include "grenade/vx/network/placed_atomic/network_builder.h"

#include "hate/algorithm.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <set>
#include <log4cxx/logger.h>

namespace grenade::vx::network::placed_atomic {

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
	auto const& population_post = m_populations.at(projection.population_post);

	// check that target population is not external
	if (std::holds_alternative<ExternalPopulation>(population_post)) {
		throw std::runtime_error(
		    "Projections with external post-synaptic population not supported.");
	}

	// check that no single connection index is out of range of its population
	auto const get_size = hate::overloaded(
	    [](Population const& p) { return p.neurons.size(); }, [](auto const& p) { return p.size; });
	auto const size_pre = std::visit(get_size, population_pre);
	auto const size_post = std::visit(get_size, population_post);

	for (auto const& connection : projection.connections) {
		if ((connection.index_pre > size_pre) || (connection.index_post > size_post)) {
			throw std::runtime_error("Connection index out of range of population(s).");
		}
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
	if (m_cadc_recording) {
		for (auto const& neuron : m_cadc_recording->neurons) {
			if (neuron.population == madc_recording.population &&
			    neuron.index == madc_recording.index && neuron.source != madc_recording.source) {
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
			if (target_population.descriptor == madc_recording.population) {
				for (size_t j = 0; j < population_neurons.size(); ++j) {
					if (j == madc_recording.index &&
					    target_population.neuron_readout_sources.at(j) &&
					    *target_population.neuron_readout_sources.at(j) != madc_recording.source) {
						throw std::runtime_error(
						    "PlasticityRule neuron readout source doesn't match "
						    "MADC recording source for same neuron.");
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
	std::set<std::pair<PopulationDescriptor, size_t>> unique;
	for (auto const& neuron : cadc_recording.neurons) {
		if (!m_populations.contains(neuron.population)) {
			throw std::runtime_error("CADC recording references unknown population.");
		}
		if (!std::holds_alternative<Population>(m_populations.at(neuron.population))) {
			throw std::runtime_error("CADC recording does not reference internal population.");
		}
		if (neuron.index >=
		    std::get<Population>(m_populations.at(neuron.population)).neurons.size()) {
			throw std::runtime_error(
			    "CADC recording references neuron index out of range of population.");
		}
		if (m_madc_recording && neuron.population == m_madc_recording->population &&
		    neuron.index == m_madc_recording->index && neuron.source != m_madc_recording->source) {
			throw std::runtime_error(
			    "CADC recording source conflicts with MADC recording's source.");
		}
		unique.insert({neuron.population, neuron.index});
	}
	if (unique.size() != cadc_recording.neurons.size()) {
		throw std::runtime_error("CADC recording neurons are not unique.");
	}
	m_cadc_recording = cadc_recording;
	LOG4CXX_TRACE(m_logger, "add(): Added CADC recording in " << timer.print() << ".");
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
			std::set<size_t> rows;
			std::set<size_t> columns;
			std::vector<std::pair<size_t, size_t>> indices;
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
				if (!std::is_sorted(
				        population_pre.neurons.begin(), population_pre.neurons.end(),
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
		if (std::get<Population>(m_populations.at(d.descriptor)).neurons.size() !=
		    d.neuron_readout_sources.size()) {
			throw std::runtime_error("PlasticityRule population neuron readout source count "
			                         "doesn't match population neuron count.");
		}
	}

	// check that target population readout settings don't contradict madc recording
	if (m_madc_recording) {
		for (auto const& target_population : plasticity_rule.populations) {
			auto const& population_neurons =
			    std::get<Population>(m_populations.at(target_population.descriptor)).neurons;
			if (target_population.descriptor == m_madc_recording->population) {
				for (size_t j = 0; j < population_neurons.size(); ++j) {
					if (j == m_madc_recording->index &&
					    target_population.neuron_readout_sources.at(j) &&
					    *target_population.neuron_readout_sources.at(j) !=
					        m_madc_recording->source) {
						throw std::runtime_error(
						    "PlasticityRule neuron readout source doesn't match "
						    "MADC recording source for same neuron.");
					}
				}
			}
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
	    std::move(m_cadc_recording), std::move(m_plasticity_rules), m_duration);
	m_madc_recording.reset();
	m_cadc_recording.reset();
	m_duration = std::chrono::microseconds(0);
	assert(ret);
	LOG4CXX_DEBUG(m_logger, "done(): " << *ret);
	return ret;
}

} // namespace grenade::vx::network::placed_atomic
