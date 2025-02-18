#include "grenade/vx/network/abstract/population_cell/uncalibrated.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "hate/indent.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

size_t UncalibratedNeuron::ParameterSpace::Parameterization::size() const
{
	return configs.size();
}

std::unique_ptr<grenade::common::PortData>
UncalibratedNeuron::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<UncalibratedNeuron::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData>
UncalibratedNeuron::ParameterSpace::Parameterization::move()
{
	return std::make_unique<UncalibratedNeuron::ParameterSpace::Parameterization>(std::move(*this));
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace::Parameterization>
UncalibratedNeuron::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	Parameterization ret;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	ret.configs.reserve(sequence.size());
	std::map<size_t, size_t> section_index_lut;
	for (size_t i = 0; auto const& element : sequence.get_elements()) {
		ret.configs.push_back(configs.at(element.value.at(0)));
		section_index_lut.emplace(element.value.at(0), i);
		i++;
	}
	for (auto const& [indices, chip] : base_configs) {
		std::vector<size_t> section_indices;
		section_indices.reserve(indices.size());
		for (auto const& index : indices) {
			if (section_index_lut.contains(index)) {
				section_indices.push_back(section_index_lut.at(index));
			}
		}
		if (!section_indices.empty()) {
			ret.base_configs.emplace_back(std::move(section_indices), chip);
		}
	}
	return std::make_unique<UncalibratedNeuron::ParameterSpace::Parameterization>(std::move(ret));
}

bool UncalibratedNeuron::ParameterSpace::Parameterization::is_equal_to(
    grenade::common::PortData const& other) const
{
	auto const& other_parameterization =
	    static_cast<UncalibratedNeuron::ParameterSpace::Parameterization const&>(other);
	return configs == other_parameterization.configs &&
	       base_configs == other_parameterization.base_configs;
}

std::ostream& UncalibratedNeuron::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	for (size_t j = 0; auto const& nrn : configs) {
		ios << hate::Indentation("\t");
		ios << "neuron_on_population: " << j << ":\n";
		for (auto const& [compartment_on_neuron, config] : nrn) {
			ios << hate::Indentation("\t\t");
			ios << compartment_on_neuron << ":\n";
			ios << hate::Indentation("\t\t\t");
			ios << hate::join(config.begin(), config.end(), "\n") << "\n";
		}
		j++;
	}
	for (auto const& [neurons_on_population, base_config] : base_configs) {
		ios << hate::Indentation("\t");
		ios << "base_config for neurons_on_population (" << hate::join(neurons_on_population, ", ")
		    << "): ";
		ios << base_config << "\n";
	}
	ios << hate::Indentation() << "\n)";
	return os;
}


UncalibratedNeuron::ParameterSpace::ParameterSpace(
    size_t size, NumNeuronCircuits num_neuron_circuits) :
    num_neuron_circuits(std::move(num_neuron_circuits)), m_size(size)
{
}

size_t UncalibratedNeuron::ParameterSpace::size() const
{
	return m_size;
}

bool UncalibratedNeuron::ParameterSpace::valid(
    size_t input_port_on_cell,
    grenade::common::Population::Cell::ParameterSpace::Parameterization const& parameterization)
    const
{
	if (input_port_on_cell != 1) {
		return false;
	}
	if (auto const ptr = dynamic_cast<Parameterization const*>(&parameterization); ptr) {
		if (m_size != ptr->configs.size()) {
			return false;
		}
		for (size_t i = 0; i < m_size; ++i) {
			if (ptr->configs.at(i).size() != num_neuron_circuits.size()) {
				return false;
			}
			for (auto const& [compartment_on_neuron, num] : num_neuron_circuits) {
				if (!ptr->configs.at(i).contains(compartment_on_neuron) ||
				    ptr->configs.at(i).at(compartment_on_neuron).size() != num) {
					return false;
				}
			}
		}
		// check that for all neurons there's exactly one base config entry
		std::unordered_set<size_t> unique_base_config_neurons;
		for (auto const& [neurons_on_population, _] : ptr->base_configs) {
			for (auto const& neuron_on_population : neurons_on_population) {
				// check that no index is out of range of the parameter space
				if (neuron_on_population >= m_size) {
					return false;
				}
				unique_base_config_neurons.insert(neuron_on_population);
			}
		}
		return unique_base_config_neurons.size() == m_size;
	}
	return false;
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
UncalibratedNeuron::ParameterSpace::copy() const
{
	return std::make_unique<UncalibratedNeuron::ParameterSpace>(*this);
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
UncalibratedNeuron::ParameterSpace::move()
{
	return std::make_unique<UncalibratedNeuron::ParameterSpace>(std::move(*this));
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
UncalibratedNeuron::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	return std::make_unique<UncalibratedNeuron::ParameterSpace>(
	    sequence.size(), num_neuron_circuits);
}

bool UncalibratedNeuron::ParameterSpace::is_equal_to(
    grenade::common::Population::Cell::ParameterSpace const& other) const
{
	return num_neuron_circuits ==
	       static_cast<UncalibratedNeuron::ParameterSpace const&>(other).num_neuron_circuits;
}

std::ostream& UncalibratedNeuron::ParameterSpace::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "ParameterSpace(\n";
	ios << hate::Indentation("\t");
	ios << "size: " << m_size << "\n";
	for (auto const& [compartment_on_neuron, num] : num_neuron_circuits) {
		ios << compartment_on_neuron << ": " << num << "\n";
	}
	ios << hate::Indentation() << "\n)";
	return os;
}


UncalibratedNeuron::UncalibratedNeuron(
    Compartments compartments, halco::hicann_dls::vx::v3::LogicalNeuronCompartments shape) :
    LocallyPlacedNeuron(std::move(compartments), std::move(shape))
{
}

bool UncalibratedNeuron::valid(
    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const
{
	if (auto const ptr = dynamic_cast<ParameterSpace const*>(&parameter_space); ptr != nullptr) {
		if (shape.get_compartments().size() != ptr->num_neuron_circuits.size()) {
			return false;
		}
		for (auto const& [compartment, atomic_neurons] : shape.get_compartments()) {
			if (!ptr->num_neuron_circuits.contains(
			        grenade::common::CompartmentOnNeuron(compartment.value())) ||
			    ptr->num_neuron_circuits.at(grenade::common::CompartmentOnNeuron(
			        compartment.value())) != atomic_neurons.size()) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::vector<grenade::common::Vertex::Port> UncalibratedNeuron::get_input_ports() const
{
	auto ret = LocallyPlacedNeuron::get_input_ports();
	ret.push_back(grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})})));
	return ret;
}

std::unique_ptr<grenade::common::Population::Cell> UncalibratedNeuron::copy() const
{
	return std::make_unique<UncalibratedNeuron>(*this);
}

std::unique_ptr<grenade::common::Population::Cell> UncalibratedNeuron::move()
{
	return std::make_unique<UncalibratedNeuron>(std::move(*this));
}

bool UncalibratedNeuron::is_equal_to(grenade::common::Population::Cell const& other) const
{
	return LocallyPlacedNeuron::is_equal_to(other);
}

std::ostream& UncalibratedNeuron::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "UncalibratedNeuron(\n";
	ios << hate::Indentation("\t");
	LocallyPlacedNeuron::print(ios);
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
