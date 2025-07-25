#pragma once
#include "grenade/common/projection.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <memory>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Uncalibrated signed synapse type.
 * This synapse type is potentially represented by multiple synapse circuits for each polarity.
 * The weight is given in units of the hardware property of a single synapse circuit.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) UncalibratedSignedSynapse
    : public grenade::common::Projection::Synapse
{
	/**
	 * Weight value in units of the hardware synapse circuit property.
	 * Absolute values larger than configurable using a single synapse are spread using multiple
	 * synapse circuits.
	 * Positive values are configured using synapse circuits acting excitatory, negative values are
	 * configured using synapse circuits acting inhibitory.
	 */
	struct GENPYBIND(inline_base("*")) Weight
	    : public halco::common::detail::BaseType<Weight, intmax_t>
	{
		constexpr explicit Weight(value_type const value = 0) : base_t(value) {}
	};

	struct ParameterSpace : public grenade::common::Projection::Synapse::ParameterSpace
	{
		struct Parameterization
		    : public grenade::common::Projection::Synapse::ParameterSpace::Parameterization
		{
			/**
			 * Weights per synapse parameterization.
			 * The association of synapse parameterization and synapse instances is defined by the
			 * projection's connector, multiple synapses might have a shared parameterization.
			 */
			std::vector<Weight> weights;

			virtual size_t size() const override;

			Parameterization(std::vector<Weight> weights);

			/**
			 * Get section of parameterization.
			 * The sequence is required to be included in the interval [0, size()).
			 */
			virtual std::unique_ptr<
			    grenade::common::Projection::Synapse::ParameterSpace::Parameterization>
			get_section(grenade::common::MultiIndexSequence const& sequence) const override;

			virtual std::unique_ptr<grenade::common::PortData> copy() const override;
			virtual std::unique_ptr<grenade::common::PortData> move() override;

		protected:
			virtual bool is_equal_to(grenade::common::PortData const& other) const override;
			virtual std::ostream& print(std::ostream& os) const override;
		};

		/**
		 * Maximal weights per synapse parameterization.
		 * This is used during mapping to calculate the number of synapse circuits to allocate.
		 * It is used for both excitatory and inhibitory synapse circuit requirement calculations.
		 */
		std::vector<Weight> max_weights;

		ParameterSpace(std::vector<Weight> max_weights);

		virtual size_t size() const override;

		virtual bool valid(
		    size_t input_port_on_synapse,
		    grenade::common::Projection::Synapse::ParameterSpace::Parameterization const&
		        parameterization) const override;

		/**
		 * Get section of parameter space.
		 * The sequence is required to be included in the interval [0, size()).
		 */
		virtual std::unique_ptr<grenade::common::Projection::Synapse::ParameterSpace> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const override;

		virtual std::unique_ptr<grenade::common::Projection::Synapse::ParameterSpace> copy()
		    const override;
		virtual std::unique_ptr<grenade::common::Projection::Synapse::ParameterSpace> move()
		    override;

	protected:
		virtual bool is_equal_to(
		    grenade::common::Projection::Synapse::ParameterSpace const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;
	};

	/**
	 * Construct uncalibrated signed synapse.
	 * @param excitatory_receptor ReceptorOnCompartment value to use to represent the excitatory
	 * output portion
	 * @param inhibitory_receptor ReceptorOnCompartment value to use to represent the inhibitory
	 * output portion
	 */
	UncalibratedSignedSynapse(
	    grenade::common::ReceptorOnCompartment excitatory_receptor,
	    grenade::common::ReceptorOnCompartment inhibitory_receptor);

	/**
	 * ReceptorOnCompartment value to use to represent the excitatory output portion.
	 */
	grenade::common::ReceptorOnCompartment excitatory_receptor;

	/**
	 * ReceptorOnCompartment value to use to represent the inhibitory output portion.
	 */
	grenade::common::ReceptorOnCompartment inhibitory_receptor;

	virtual bool valid(
	    grenade::common::Projection::Synapse::ParameterSpace const& parameter_space) const override;
	virtual bool valid(
	    size_t input_port_on_synapse,
	    grenade::common::Projection::Synapse::Dynamics const& dynamics) const override;

	/**
	 * Only ClockCycleTimeDomainRuntimes are valid.
	 */
	virtual bool valid(
	    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const override;

	/**
	 * UncalibratedSignedSynapse requires a time domain, since it is represented by physical
	 * circuits operating in time.
	 */
	virtual bool requires_time_domain() const override;

	/**
	 * UncalibratedSignedSynapse is partitionable onto an execution instance on the executor.
	 */
	virtual bool is_partitionable() const override;

	/**
	 * Get input ports.
	 * The UncalibratedSignedSynapse features one input port of spikes to the synapses (port index
	 * 0) and one for the weight parameterization (port index 1).
	 */
	virtual Ports get_input_ports() const override;

	/**
	 * Get output ports.
	 * The UncalibratedSignedSynapse features one output port of synaptic input from the synapses
	 * (port index 0) and one for the weight parameterization (port index 1).
	 * The output port of synaptic input features two channels, one for the excitatory receptor, one
	 * for the inhibitory receptor.
	 */
	virtual Ports get_output_ports() const override;

	virtual std::unique_ptr<grenade::common::Projection::Synapse> copy() const override;
	virtual std::unique_ptr<grenade::common::Projection::Synapse> move() override;

protected:
	virtual bool is_equal_to(grenade::common::Projection::Synapse const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
