#include "grenade/vx/network/abstract/population_cell/calibrated.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "hate/indent.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

std::ostream& operator<<(
    std::ostream& os,
    CalibratedNeuron::ParameterSpace::CalibrationTarget::RefractoryPeriod const& value)
{
	hate::IndentingOstream ios(os);
	ios << "RefractoryPeriod(\n";
	ios << hate::Indentation("\t");
	ios << "refractory_time: " << value.refractory_time << "\n";
	ios << "holdoff_time: " << value.holdoff_time << "\n";
	ios << hate::Indentation() << ")";
	return os;
}

std::ostream& operator<<(
    std::ostream& os,
    CalibratedNeuron::ParameterSpace::CalibrationTarget::CubaSynapticInput const& value)
{
	hate::IndentingOstream ios(os);
	ios << "CubaSynapticInput(\n";
	ios << hate::Indentation("\t");
	ios << "tau_syn: " << value.tau_syn << "\n";
	ios << "i_synin_gm: " << value.i_synin_gm << "\n";
	ios << "synapse_dac_bias: " << value.synapse_dac_bias << "\n";
	ios << hate::Indentation() << ")";
	return os;
}

std::ostream& operator<<(
    std::ostream& os,
    CalibratedNeuron::ParameterSpace::CalibrationTarget::CobaSynapticInput const& value)
{
	hate::IndentingOstream ios(os);
	ios << "CobaSynapticInput(\n";
	ios << hate::Indentation("\t");
	ios << "tau_syn: " << value.tau_syn << "\n";
	ios << "e_reversal: " << value.e_reversal << "\n";
	ios << "e_reference: ";
	if (value.e_reference) {
		ios << *value.e_reference;
	} else {
		ios << "disabled";
	}
	ios << "\n";
	ios << "i_synin_gm: " << value.i_synin_gm << "\n";
	ios << "synapse_dac_bias: " << value.synapse_dac_bias << "\n";
	ios << hate::Indentation() << ")";
	return os;
}

std::ostream& operator<<(
    std::ostream& os,
    CalibratedNeuron::ParameterSpace::CalibrationTarget::DisabledSynapticInput const&)
{
	return os << "DisabledSynapticInput()";
}

std::ostream& operator<<(
    std::ostream& os,
    CalibratedNeuron::ParameterSpace::CalibrationTarget::InterAtomicNeuronConnectivity const& value)
{
	hate::IndentingOstream ios(os);
	ios << "InterAtomicNeuronConnectivity(\n";
	ios << hate::Indentation("\t");
	ios << "connect_soma: " << value.connect_soma << "\n";
	ios << "connect_soma_right: " << value.connect_soma_right << "\n";
	ios << "connect_right: " << value.connect_right << "\n";
	ios << "connect_vertical: " << value.connect_vertical << "\n";
	ios << "tau_icc: ";
	if (value.tau_icc) {
		ios << *value.tau_icc;
	} else {
		ios << "disabled";
	}
	ios << "\n";
	ios << hate::Indentation() << ")";
	return os;
}


std::ostream& operator<<(
    std::ostream& os, CalibratedNeuron::ParameterSpace::CalibrationTarget const& value)
{
	hate::IndentingOstream ios(os);
	ios << "CalibrationTarget(\n";
	ios << hate::Indentation("\t");

	ios << "membrane_capacitance: " << value.membrane_capacitance << "\n";

	ios << "membrane_capacitance_during_calibration: ";
	if (value.membrane_capacitance_during_calibration) {
		ios << *value.membrane_capacitance_during_calibration;
	} else {
		ios << "membrane_capacitance";
	}
	ios << "\n";

	ios << "v_leak: " << value.v_leak << "\n";
	ios << "tau_membrane: ";
	if (value.tau_membrane) {
		ios << *value.tau_membrane;
	} else {
		ios << "disabled";
	}
	ios << "\n";

	ios << "v_threshold: ";
	if (value.v_threshold) {
		ios << *value.v_threshold;
	} else {
		ios << "disabled";
	}
	ios << "\n";

	ios << "v_reset: " << value.v_reset << "\n";

	ios << "refractory_period: ";
	if (value.refractory_period) {
		ios << *value.refractory_period;
	} else {
		ios << "disabled";
	}
	ios << "\n";

	ios << "synaptic_input_excitatory: ";
	std::visit([&ios](auto const& synin) { ios << synin; }, value.synaptic_input_excitatory);
	ios << "\n";

	ios << "synaptic_input_inhibitory: ";
	std::visit([&ios](auto const& synin) { ios << synin; }, value.synaptic_input_inhibitory);
	ios << "\n";

	ios << "inter_atomic_neuron_connectivity: " << value.inter_atomic_neuron_connectivity << "\n";
	ios << hate::Indentation() << ")";
	return os;
}

CalibratedNeuron::ParameterSpace::Parameterization::Parameterization(
    CalibrationTargets calibration_targets, ReadoutSources readout_sources) :
    calibration_targets(std::move(calibration_targets)), readout_sources(std::move(readout_sources))
{
}

size_t CalibratedNeuron::ParameterSpace::Parameterization::size() const
{
	if (calibration_targets.size() != readout_sources.size()) {
		throw std::runtime_error("Parameterization has inconsistent size.");
	}
	return calibration_targets.size();
}

std::unique_ptr<grenade::common::PortData>
CalibratedNeuron::ParameterSpace::Parameterization::copy() const
{
	return std::make_unique<CalibratedNeuron::ParameterSpace::Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData>
CalibratedNeuron::ParameterSpace::Parameterization::move()
{
	return std::make_unique<CalibratedNeuron::ParameterSpace::Parameterization>(std::move(*this));
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace::Parameterization>
CalibratedNeuron::ParameterSpace::Parameterization::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	CalibrationTargets section_targets;
	ReadoutSources section_readout_sources;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameterization to get section from.");
	}
	section_targets.reserve(sequence.size());
	section_readout_sources.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		section_targets.push_back(calibration_targets.at(element.value.at(0)));
		section_readout_sources.push_back(readout_sources.at(element.value.at(0)));
	}
	return std::make_unique<CalibratedNeuron::ParameterSpace::Parameterization>(
	    std::move(section_targets), std::move(section_readout_sources));
}

bool CalibratedNeuron::ParameterSpace::Parameterization::is_equal_to(
    grenade::common::PortData const& other) const
{
	return calibration_targets ==
	           static_cast<CalibratedNeuron::ParameterSpace::Parameterization const&>(other)
	               .calibration_targets &&
	       readout_sources ==
	           static_cast<CalibratedNeuron::ParameterSpace::Parameterization const&>(other)
	               .readout_sources;
}

std::ostream& CalibratedNeuron::ParameterSpace::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	for (size_t j = 0; auto const& nrn : calibration_targets) {
		ios << hate::Indentation("\t");
		ios << "neuron_on_population " << j << ":\n";
		for (auto const& [compartment_on_neuron, config] : nrn) {
			ios << hate::Indentation("\t\t");
			ios << compartment_on_neuron << ":\n";
			ios << hate::Indentation("\t\t\t");
			ios << hate::join(config.begin(), config.end(), "\n");
		}
		ios << "\n";
		j++;
	}
	for (size_t j = 0; auto const& nrn : readout_sources) {
		ios << hate::Indentation("\t");
		ios << "neuron_on_population " << j << ":\n";
		for (auto const& [compartment_on_neuron, ans] : nrn) {
			ios << hate::Indentation("\t\t");
			ios << compartment_on_neuron << ":\n";
			ios << hate::Indentation("\t\t\t");
			ios << hate::join(ans.begin(), ans.end(), "\n");
		}
		ios << "\n";
		j++;
	}
	ios << hate::Indentation() << "\n)";
	return os;
}


CalibratedNeuron::ParameterSpace::ParameterSpace(CalibrationTargets calibration_targets) :
    calibration_targets(std::move(calibration_targets))
{
}

size_t CalibratedNeuron::ParameterSpace::size() const
{
	return calibration_targets.size();
}

bool CalibratedNeuron::ParameterSpace::valid(
    size_t input_port_on_cell,
    grenade::common::Population::Cell::ParameterSpace::Parameterization const& parameterization)
    const
{
	if (input_port_on_cell != 1) {
		return false;
	}
	if (auto const ptr = dynamic_cast<Parameterization const*>(&parameterization); ptr) {
		if (calibration_targets != ptr->calibration_targets) {
			return false;
		}
		return true;
	}
	return false;
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
CalibratedNeuron::ParameterSpace::copy() const
{
	return std::make_unique<CalibratedNeuron::ParameterSpace>(*this);
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
CalibratedNeuron::ParameterSpace::move()
{
	return std::make_unique<CalibratedNeuron::ParameterSpace>(std::move(*this));
}

std::unique_ptr<grenade::common::Population::Cell::ParameterSpace>
CalibratedNeuron::ParameterSpace::get_section(
    grenade::common::MultiIndexSequence const& sequence) const
{
	CalibrationTargets section_targets;

	if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in parameter space to get section from.");
	}
	section_targets.reserve(sequence.size());
	for (auto const& element : sequence.get_elements()) {
		section_targets.push_back(calibration_targets.at(element.value.at(0)));
	}
	return std::make_unique<CalibratedNeuron::ParameterSpace>(std::move(section_targets));
}

bool CalibratedNeuron::ParameterSpace::is_equal_to(
    grenade::common::Population::Cell::ParameterSpace const& other) const
{
	return calibration_targets ==
	       static_cast<CalibratedNeuron::ParameterSpace const&>(other).calibration_targets;
}

std::ostream& CalibratedNeuron::ParameterSpace::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "ParameterSpace(\n";
	for (size_t j = 0; auto const& nrn : calibration_targets) {
		ios << hate::Indentation("\t");
		ios << "neuron_on_population " << j << ":\n";
		for (auto const& [compartment_on_neuron, config] : nrn) {
			ios << hate::Indentation("\t\t");
			ios << compartment_on_neuron << ":\n";
			ios << hate::Indentation("\t\t\t");
			ios << hate::join(config.begin(), config.end(), "\n");
		}
		ios << "\n";
		j++;
	}
	ios << hate::Indentation() << "\n)";
	return os;
}


CalibratedNeuron::CalibratedNeuron(
    Compartments compartments, halco::hicann_dls::vx::v3::LogicalNeuronCompartments shape) :
    LocallyPlacedNeuron(std::move(compartments), std::move(shape))
{
}

bool CalibratedNeuron::valid(
    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const
{
	if (auto const ptr = dynamic_cast<ParameterSpace const*>(&parameter_space); ptr) {
		for (auto const& neuron : ptr->calibration_targets) {
			if (shape.get_compartments().size() != neuron.size()) {
				return false;
			}
			for (auto const& [compartment, atomic_neurons] : shape.get_compartments()) {
				if (!neuron.contains(grenade::common::CompartmentOnNeuron(compartment.value())) ||
				    neuron.at(grenade::common::CompartmentOnNeuron(compartment.value())).size() !=
				        atomic_neurons.size()) {
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

std::vector<grenade::common::Vertex::Port> CalibratedNeuron::get_input_ports() const
{
	auto ret = LocallyPlacedNeuron::get_input_ports();
	ret.push_back(grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})})));
	return ret;
}

std::unique_ptr<grenade::common::Population::Cell> CalibratedNeuron::copy() const
{
	return std::make_unique<CalibratedNeuron>(*this);
}

std::unique_ptr<grenade::common::Population::Cell> CalibratedNeuron::move()
{
	return std::make_unique<CalibratedNeuron>(std::move(*this));
}

bool CalibratedNeuron::is_equal_to(grenade::common::Population::Cell const& other) const
{
	return LocallyPlacedNeuron::is_equal_to(other);
}

std::ostream& CalibratedNeuron::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "CalibratedNeuron(\n";
	ios << hate::Indentation("\t");
	LocallyPlacedNeuron::print(ios);
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
