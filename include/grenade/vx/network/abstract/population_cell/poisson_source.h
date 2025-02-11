#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/input_data.h"
#include "grenade/common/population.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "haldls/vx/v3/background.h"
#include "hate/visibility.h"
#include <vector>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Neuron representing a Poisson source using the on-chip background generators.
 * This neuron can serve as source only.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) PoissonSourceNeuron
    : public grenade::common::Population::Cell
{
	struct ParameterSpace : public grenade::common::Population::Cell::ParameterSpace
	{
		ParameterSpace(size_t size);

		struct Parameterization
		    : public grenade::common::Population::Cell::ParameterSpace::Parameterization
		{
			haldls::vx::v3::BackgroundSpikeSource::Period period;
			haldls::vx::v3::BackgroundSpikeSource::Rate rate;
			haldls::vx::v3::BackgroundSpikeSource::Seed seed;

			Parameterization(size_t size);

			virtual size_t size() const override;

			virtual std::unique_ptr<grenade::common::PortData> copy() const override;
			virtual std::unique_ptr<grenade::common::PortData> move() override;

			virtual std::unique_ptr<
			    grenade::common::Population::Cell::ParameterSpace::Parameterization>
			get_section(grenade::common::MultiIndexSequence const& sequence) const override;

		protected:
			virtual bool is_equal_to(grenade::common::PortData const& other) const override;
			virtual std::ostream& print(std::ostream& os) const override;

		private:
			size_t m_size;
		};

		virtual size_t size() const override;

		/**
		 * Only PoissonSourceNeuron parameterization is valid for input port 0.
		 */
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

	private:
		size_t m_size;
	};

	/**
	 * Only PoissonSourceNeuron parameter space is valid.
	 */
	virtual bool valid(
	    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const override;

	/**
	 * PoissonSourceNeuron requires a time domain, since it is represented by physical circuits
	 * operating in time.
	 */
	virtual bool requires_time_domain() const override;

	/**
	 * PoissonSourceNeuron is partitionable onto an execution instance on the executor.
	 */
	virtual bool is_partitionable() const override;

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	/**
	 * PoissonSourceNeuron doesn't have externally-defined inputs, therefore no dynamics are valid.
	 */
	virtual bool valid(
	    size_t input_port_on_cell,
	    grenade::common::Population::Cell::Dynamics const& dynamics) const override;

	/**
	 * Only ClockCycleTimeDomainRuntimes are valid.
	 */
	virtual bool valid(
	    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const override;

	/**
	 * Get input ports.
	 * The PoissonSourceNeuron features one input port of parameterization to the neuron (port
	 * index 0). Its channel is formed by the compartment (0).
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;

	/**
	 * Get output ports.
	 * The PoissonSourceNeuron features one output port of spikes from the neuron (port index
	 * 0), where the channel is the compartment (0).
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_output_ports() const override;

	virtual std::unique_ptr<grenade::common::Population::Cell> copy() const override;
	virtual std::unique_ptr<grenade::common::Population::Cell> move() override;

protected:
	virtual bool is_equal_to(grenade::common::Population::Cell const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
