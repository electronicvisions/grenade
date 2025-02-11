#include "grenade/vx/network/abstract/population_cell/locally_placed.h"

#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/multi_index_sequence_dimension_unit/compartment_on_neuron.h"
#include "grenade/common/multi_index_sequence_dimension_unit/receptor_on_compartment.h"
#include "grenade/common/population.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/multi_index_sequence_dimension_unit/atomic_neuron_on_compartment.h"
#include "grenade/vx/network/abstract/vertex_port_type/analog_observable.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "grenade/vx/network/abstract/vertex_port_type/synaptic_input.h"
#include "hate/indent.h"
#include "hate/join.h"

namespace grenade::vx::network::abstract {

LocallyPlacedNeuron::Compartment::SpikeMaster::SpikeMaster(size_t atomic_neuron_on_compartment) :
    atomic_neuron_on_compartment(atomic_neuron_on_compartment)
{
}

std::ostream& operator<<(
    std::ostream& os, LocallyPlacedNeuron::Compartment::SpikeMaster const& config)
{
	os << "SpikeMaster(atomic_neuron_on_compartment: " << config.atomic_neuron_on_compartment
	   << ")";
	return os;
}


LocallyPlacedNeuron::Compartment::Compartment(
    std::optional<SpikeMaster> const& spike_master, Receptors const& receptors) :
    spike_master(spike_master), receptors(receptors)
{
}

std::ostream& operator<<(std::ostream& os, LocallyPlacedNeuron::Compartment const& config)
{
	hate::IndentingOstream ios(os);
	ios << "Compartment(\n";
	ios << hate::Indentation("\t");
	ios << "spike_master: ";
	if (config.spike_master) {
		ios << *(config.spike_master);
	} else {
		ios << "None";
	}
	ios << "\n";
	ios << "receptors:\n";
	for (size_t an = 0; auto const& receptor : config.receptors) {
		ios << hate::Indentation("\t\t");
		ios << "atomic_neuron_on_compartment " << an << ":\n";
		ios << hate::Indentation("\t\t\t");
		for (auto const& [roc, t] : receptor) {
			ios << roc << ": " << t << "\n";
		}
		an++;
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}


LocallyPlacedNeuron::LocallyPlacedNeuron(
    Compartments compartments, halco::hicann_dls::vx::v3::LogicalNeuronCompartments shape) :
    compartments(std::move(compartments)), shape(std::move(shape))
{
}

bool LocallyPlacedNeuron::valid(size_t, grenade::common::Population::Cell::Dynamics const&) const
{
	return false;
}

bool LocallyPlacedNeuron::requires_time_domain() const
{
	return true;
}

bool LocallyPlacedNeuron::is_partitionable() const
{
	return true;
}

bool LocallyPlacedNeuron::valid(
    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const
{
	return dynamic_cast<ClockCycleTimeDomainRuntimes const*>(&time_domain_runtimes) != nullptr;
}


std::vector<grenade::common::Vertex::Port> LocallyPlacedNeuron::get_input_ports() const
{
	std::vector<grenade::common::MultiIndex> compartment_elements;
	for (auto const& [compartment_on_neuron, compartment] : compartments) {
		std::set<grenade::common::ReceptorOnCompartment> receptors;
		for (auto const& atomic_cell : compartment.receptors) {
			for (auto const& [receptor, _] : atomic_cell) {
				receptors.insert(receptor);
			}
		}
		for (auto const& receptor : receptors) {
			compartment_elements.push_back(
			    grenade::common::MultiIndex({compartment_on_neuron.value(), receptor.value()}));
		}
	}
	// try to simplify to cuboid sequence
	std::unique_ptr<grenade::common::MultiIndexSequence> compartment_sequence;
	if (!compartment_elements.empty()) {
		auto shape = compartment_elements.back().value;
		for (size_t i = 0; i < shape.size(); ++i) {
			shape.at(i) -= compartment_elements.at(0).value.at(i); // max - min
			shape.at(i) += 1;                                      // size = max - min + 1
		}
		grenade::common::CuboidMultiIndexSequence cuboid_compartment_sequence(
		    shape, compartment_elements.at(0),
		    {grenade::common::CompartmentOnNeuronDimensionUnit(),
		     grenade::common::ReceptorOnCompartmentDimensionUnit()});

		grenade::common::ListMultiIndexSequence list_compartment_sequence(
		    std::move(compartment_elements),
		    {grenade::common::CompartmentOnNeuronDimensionUnit(),
		     grenade::common::ReceptorOnCompartmentDimensionUnit()});

		if (cuboid_compartment_sequence.size() == list_compartment_sequence.size() &&
		    cuboid_compartment_sequence.includes(list_compartment_sequence)) {
			compartment_sequence = cuboid_compartment_sequence.move();
		} else {
			compartment_sequence = list_compartment_sequence.move();
		}
	} else {
		compartment_sequence = grenade::common::ListMultiIndexSequence(
		                           std::move(compartment_elements),
		                           {grenade::common::CompartmentOnNeuronDimensionUnit(),
		                            grenade::common::ReceptorOnCompartmentDimensionUnit()})
		                           .move();
	}

	assert(compartment_sequence);
	return {grenade::common::Vertex::Port(
	    SynapticInput(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	    std::move(*compartment_sequence))};
}

std::vector<grenade::common::Vertex::Port> LocallyPlacedNeuron::get_output_ports() const
{
	std::vector<grenade::common::MultiIndex> compartment_elements;
	for (auto const& [compartment_on_neuron, _] : compartments) {
		compartment_elements.push_back(
		    grenade::common::MultiIndex({compartment_on_neuron.value()}));
	}
	// try to simplify to cuboid sequence
	std::unique_ptr<grenade::common::MultiIndexSequence> compartment_sequence;
	if (!compartment_elements.empty()) {
		auto shape = compartment_elements.back().value;
		for (size_t i = 0; i < shape.size(); ++i) {
			shape.at(i) -= compartment_elements.at(0).value.at(i); // max - min
			shape.at(i) += 1;                                      // size = max - min + 1
		}
		grenade::common::CuboidMultiIndexSequence cuboid_compartment_sequence(
		    shape, compartment_elements.at(0),
		    {grenade::common::CompartmentOnNeuronDimensionUnit()});

		grenade::common::ListMultiIndexSequence list_compartment_sequence(
		    std::move(compartment_elements), {grenade::common::CompartmentOnNeuronDimensionUnit()});

		if (cuboid_compartment_sequence.size() == list_compartment_sequence.size() &&
		    cuboid_compartment_sequence.includes(list_compartment_sequence)) {
			compartment_sequence = cuboid_compartment_sequence.move();
		} else {
			compartment_sequence = list_compartment_sequence.move();
		}
	} else {
		compartment_sequence = grenade::common::ListMultiIndexSequence(
		                           std::move(compartment_elements),
		                           {grenade::common::CompartmentOnNeuronDimensionUnit()})
		                           .move();
	}

	assert(compartment_sequence);
	std::vector<grenade::common::MultiIndex> elements;
	for (auto const& [compartment, atomic_cells] : shape.get_compartments()) {
		for (size_t atomic_neuron_on_compartment = 0;
		     atomic_neuron_on_compartment < atomic_cells.size(); ++atomic_neuron_on_compartment) {
			elements.push_back(
			    grenade::common::MultiIndex({compartment.value(), atomic_neuron_on_compartment}));
		}
	}
	// try to simplify to cuboid sequence
	std::unique_ptr<grenade::common::MultiIndexSequence> sequence;
	if (!compartment_elements.empty()) {
		auto shape = compartment_elements.back().value;
		for (size_t i = 0; i < shape.size(); ++i) {
			shape.at(i) -= compartment_elements.at(0).value.at(i); // max - min
			shape.at(i) += 1;                                      // size = max - min + 1
		}
		grenade::common::CuboidMultiIndexSequence cuboid_compartment_sequence(
		    shape, compartment_elements.at(0),
		    {grenade::common::CompartmentOnNeuronDimensionUnit(),
		     AtomicNeuronOnCompartmentDimensionUnit()});

		grenade::common::ListMultiIndexSequence list_compartment_sequence(
		    std::move(compartment_elements), {grenade::common::CompartmentOnNeuronDimensionUnit(),
		                                      AtomicNeuronOnCompartmentDimensionUnit()});

		if (cuboid_compartment_sequence.size() == list_compartment_sequence.size() &&
		    cuboid_compartment_sequence.includes(list_compartment_sequence)) {
			sequence = cuboid_compartment_sequence.move();
		} else {
			sequence = list_compartment_sequence.move();
		}
	} else {
		sequence = grenade::common::ListMultiIndexSequence(
		               std::move(elements), {grenade::common::CompartmentOnNeuronDimensionUnit(),
		                                     AtomicNeuronOnCompartmentDimensionUnit()})
		               .move();
	}

	assert(sequence);
	return {
	    grenade::common::Vertex::Port(
	        Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
	        std::move(*compartment_sequence)),
	    grenade::common::Vertex::Port(
	        AnalogObservable(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	        grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
	        grenade::common::Vertex::Port::RequiresOrGeneratesData::no, std::move(*sequence))};
}

bool LocallyPlacedNeuron::is_equal_to(grenade::common::Population::Cell const& other) const
{
	auto const& other_cell = static_cast<LocallyPlacedNeuron const&>(other);
	return compartments == other_cell.compartments && shape == other_cell.shape;
}

std::ostream& LocallyPlacedNeuron::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "LocallyPlacedNeuron(\n";
	ios << hate::Indentation("\t");
	for (auto const& [compartment_on_neuron, compartment] : compartments) {
		ios << compartment_on_neuron << ": " << compartment << "\n";
	}
	ios << shape << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
