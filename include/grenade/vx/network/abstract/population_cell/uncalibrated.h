#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/vx/network/abstract/population_cell/locally_placed.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include "lola/vx/v3/neuron.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Locally-placed neuron with uncalibrated parameterization.
 * The parameterization contains the hardware configuration directly.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) UncalibratedNeuron : public LocallyPlacedNeuron
{
	struct ParameterSpace : public grenade::common::Population::Cell::ParameterSpace
	{
		/**
		 * Parameterization for a population of uncalibrated neurons.
		 */
		struct Parameterization
		    : public grenade::common::Population::Cell::ParameterSpace::Parameterization
		{
			/**
			 * Neuron circuit configuration per neuron circuit per compartment on neuron per neuron.
			 */
			std::vector<std::map<
			    grenade::common::CompartmentOnNeuron,
			    std::vector<lola::vx::v3::AtomicNeuron>>>
			    configs;

			/**
			 * Base configuration required to be present on chips on which the neuron instances are
			 * placed.
			 * The outer dimension is a collection dimension, per collection the neuron
			 * indices on the population as well as the corresponding chip configuration is stored.
			 */
			std::vector<std::pair<std::vector<size_t>, lola::vx::v3::Chip>> base_configs;

			/**
			 * Number of parameterizations matching the number of neurons for which parameterization
			 * is given.
			 * Matches the outer dimension of configs.
			 */
			virtual size_t size() const override;

			virtual std::unique_ptr<grenade::common::PortData> copy() const override;
			virtual std::unique_ptr<grenade::common::PortData> move() override;

			virtual std::unique_ptr<
			    grenade::common::Population::Cell::ParameterSpace::Parameterization>
			get_section(grenade::common::MultiIndexSequence const& sequence) const override;

		protected:
			virtual bool is_equal_to(grenade::common::PortData const& other) const override;
			virtual std::ostream& print(std::ostream& os) const override;
		};

		typedef std::map<grenade::common::CompartmentOnNeuron, size_t> NumNeuronCircuits;

		/**
		 * Number of neuron circuits per compartment.
		 * This has to match the neuron's shape.
		 */
		NumNeuronCircuits num_neuron_circuits;

		/**
		 * Construct parameter space.
		 * @param size Number of neurons in population
		 * @param num_neuron_circuits Number of neuron circuits per compartment
		 */
		ParameterSpace(size_t size, NumNeuronCircuits num_neuron_circuits);

		virtual size_t size() const override;

		virtual bool valid(
		    size_t input_port_on_cell,
		    grenade::common::Population::Cell::ParameterSpace::Parameterization const&
		        parameterization) const override;

		virtual std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> copy()
		    const override;
		virtual std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> move() override;

		virtual std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const override;

	protected:
		virtual bool is_equal_to(
		    grenade::common::Population::Cell::ParameterSpace const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;

		size_t m_size;
	};

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	UncalibratedNeuron(
	    Compartments compartments, halco::hicann_dls::vx::v3::LogicalNeuronCompartments shape);

	/**
	 * Only UncalibratedNeuron parameter space is valid.
	 */
	virtual bool valid(
	    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const override;

	/**
	 * Get input ports.
	 * The UncalibratedNeuron features the same input ports as the LocallyPlacedNeuron.
	 * Additionally, it features a port for parameterization (port index 1).
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;

	virtual std::unique_ptr<grenade::common::Population::Cell> copy() const override;
	virtual std::unique_ptr<grenade::common::Population::Cell> move() override;

protected:
	virtual bool is_equal_to(grenade::common::Population::Cell const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
