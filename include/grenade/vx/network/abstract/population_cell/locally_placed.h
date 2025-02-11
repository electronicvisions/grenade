#pragma once
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/population.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/vx/network/receptor.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include "lola/vx/v3/neuron.h"
#include <unordered_set>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Base for neurons with user-given local placement of compartments.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) LocallyPlacedNeuron
    : public grenade::common::Population::Cell
{
	/** Compartment properties. */
	struct Compartment
	{
		/**
		 * The spike master is used for outgoing connections and recording of spikes.
		 * Not every compartment is required to feature a spike master, e.g. if it is passive.
		 */
		struct SpikeMaster
		{
			/** Index of atomic neuron circuit on compartment. */
			size_t atomic_neuron_on_compartment;

			SpikeMaster(size_t atomic_neuron_on_compartment) SYMBOL_VISIBLE;

			bool operator==(SpikeMaster const& other) const = default;
			bool operator!=(SpikeMaster const& other) const = default;

			GENPYBIND(stringstream)
			friend std::ostream& operator<<(std::ostream& os, SpikeMaster const& config)
			    SYMBOL_VISIBLE;
		};
		std::optional<SpikeMaster> spike_master;

		/** Receptors per atomic neuron on compartment. */
		typedef Receptor::Type ReceptorType GENPYBIND(visible);
		typedef std::vector<std::map<grenade::common::ReceptorOnCompartment, ReceptorType>>
		    Receptors;
		Receptors receptors;

		Compartment(std::optional<SpikeMaster> const& spike_master, Receptors const& receptors)
		    SYMBOL_VISIBLE;

		bool operator==(Compartment const& other) const = default;
		bool operator!=(Compartment const& other) const = default;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Compartment const& config) SYMBOL_VISIBLE;
	};

	/* Map of compartment properties. */
	typedef std::map<grenade::common::CompartmentOnNeuron, Compartment> Compartments;
	Compartments compartments;

	halco::hicann_dls::vx::v3::LogicalNeuronCompartments shape;

	LocallyPlacedNeuron(
	    Compartments compartments, halco::hicann_dls::vx::v3::LogicalNeuronCompartments shape);

	/**
	 * LocallyPlacedNeuron doesn't have externally-defined inputs, therefore no dynamics are valid.
	 */
	virtual bool valid(
	    size_t input_port_on_cell,
	    grenade::common::Population::Cell::Dynamics const& dynamics) const override;

	/**
	 * LocallyPlacedNeuron requires a time domain, since it is represented by physical circuits
	 * operating in time.
	 */
	virtual bool requires_time_domain() const override;

	/**
	 * LocallyPlacedNeuron is partitionable onto an execution instance on the executor.
	 */
	virtual bool is_partitionable() const override;

	/**
	 * Only ClockCycleTimeDomainRuntimes are valid.
	 */
	virtual bool valid(
	    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const override;

	/**
	 * Get input ports.
	 * The LocallyPlacedNeuron features one input port of synaptic input to the neuron (port index
	 * 0). Its channels are formed by the receptors (dimension 1) per compartment (dimension 0) on
	 * the neuron.
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;

	/**
	 * Get output ports.
	 * The LocallyPlacedNeuron features one output port of spikes from the neuron (port index
	 * 0), where the channels are the compartments, and one output port of an analog value to read
	 * out (port index 1), where the channels are the atomic neuron circuits (dimension 1) per
	 * compartment (dimension 0).
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_output_ports() const override;

protected:
	virtual bool is_equal_to(grenade::common::Population::Cell const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
