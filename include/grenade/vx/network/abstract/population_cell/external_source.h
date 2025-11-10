#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/input_data.h"
#include "grenade/common/population.h"
#include "grenade/common/receptor_on_compartment.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <vector>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Neuron representing an external source for which the emitted spike-train is pre-defined (via
 * dynamics input).
 * This neuron can serve as source only.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) ExternalSourceNeuron
    : public grenade::common::Population::Cell
{
	struct ParameterSpace : public grenade::common::Population::Cell::ParameterSpace
	{
		ParameterSpace(size_t size);

		virtual size_t size() const override;

		/**
		 * ExternalSourceNeuron doesn't feature parameterization, therefore no parameterization is
		 * valid.
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
	 * Only ExternalSourceNeuron parameter space is valid.
	 */
	virtual bool valid(
	    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const override;

	struct Dynamics : public grenade::common::Population::Cell::Dynamics
	{
		/**
		 * Externally-supplied spike times per neuron on population per batch entry.
		 * The outer dimension is the neuron on the population.
		 * For each neuron on the population, the outer dimension is the batch dimension, while the
		 * inner dimension is the collection of spikes for the neuron.
		 */
		typedef std::vector<std::vector<std::vector<grenade::vx::common::Time>>> SpikeTimes;
		SpikeTimes spike_times;

		Dynamics(SpikeTimes spike_times);

		virtual size_t size() const override;

		virtual size_t batch_size() const override;

		virtual std::unique_ptr<grenade::common::PortData> copy() const override;
		virtual std::unique_ptr<grenade::common::PortData> move() override;

		virtual std::unique_ptr<grenade::common::Population::Cell::Dynamics> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const override;

	protected:
		virtual bool is_equal_to(grenade::common::PortData const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;
	};

	/**
	 * Only ExternalSourceNeuron dynamics are valid.
	 */
	virtual bool valid(
	    size_t input_port_on_cell,
	    grenade::common::Population::Cell::Dynamics const& dynamics) const override;

	/**
	 * Dynamics port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) DynamicsPortType
	    : public dapr::EmptyProperty<DynamicsPortType, grenade::common::VertexPortType>
	{};

	/**
	 * ExternalSourceNeuron requires a time domain, since it is represented by physical circuits
	 * operating in time.
	 */
	virtual bool requires_time_domain() const override;

	/**
	 * ExternalSourceNeuron is partitionable onto an execution instance on the executor.
	 */
	virtual bool is_partitionable() const override;

	/**
	 * Only ClockCycleTimeDomainRuntimes are valid.
	 */
	virtual bool valid(
	    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const override;

	/**
	 * Get input ports.
	 * The ExternalSourceNeuron features one input port of dynamics to the neuron (port index
	 * 0), with one channel (0).
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;

	/**
	 * Get output ports.
	 * The ExternalSourceNeuron features one output port of spikes from the neuron (port index
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
