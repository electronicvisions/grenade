#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/compartment_on_neuron.h"
#include "grenade/common/population.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <iosfwd>
#include <memory>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Cell implementing a delay of incoming spikes.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) DelayCell : public grenade::common::Population::Cell
{
	struct ParameterSpace : public grenade::common::Population::Cell::ParameterSpace
	{
		struct Parameterization
		    : public grenade::common::Population::Cell::ParameterSpace::Parameterization
		{
			std::vector<common::Time> delays;

			virtual size_t size() const override;

			Parameterization(std::vector<common::Time> delays);

			virtual std::unique_ptr<grenade::common::PortData> copy() const override;
			virtual std::unique_ptr<grenade::common::PortData> move() override;

			virtual std::unique_ptr<
			    grenade::common::Population::Cell::ParameterSpace::Parameterization>
			get_section(grenade::common::MultiIndexSequence const& sequence) const override;

		protected:
			virtual bool is_equal_to(grenade::common::PortData const& other) const override;
			virtual std::ostream& print(std::ostream& os) const override;
		};

		ParameterSpace(size_t size);

		virtual size_t size() const override;

		/**
		 * Only DelayCell parameterization is valid for input port index 1.
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
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	/**
	 * Construct DelayCell.
	 * @param requires_time_domain Whether the delay cell requires a time domain
	 */
	DelayCell(bool requires_time_domain = true);

	/**
	 * DelayCell can require a time domain in the model topology.
	 * Depending on the implementation it might switch to not supporting a time domain if it is
	 * implemented on the host computer.
	 */
	virtual bool requires_time_domain() const override;

	/**
	 * DelayCell is partitionable onto an execution instance on the executor.
	 */
	virtual bool is_partitionable() const override;

	/**
	 * Only DelayCell parameter space is valid.
	 */
	virtual bool valid(
	    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const override;

	/**
	 * DelayCell doesn't have externally-defined inputs, therefore no dynamics are valid.
	 */
	virtual bool valid(
	    size_t input_port_on_neuron,
	    grenade::common::Population::Cell::Dynamics const& dynamics) const override;

	/**
	 * Only ClockCycleTimeDomainRuntimes are valid.
	 */
	virtual bool valid(
	    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const override;

	/**
	 * Get input ports.
	 * The DelayCell features one input port of spikes to the neuron (port
	 * index 0). Its channel is the compartment (0). Additionally, it features one input port of
	 * parameterization to the neuron (port index 1), with one channel (0).
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;

	/**
	 * Get output ports.
	 * The DelayCell features one output port of spikes from the neuron (port index
	 * 0), where the channel is the compartment (0).
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_output_ports() const override;

	virtual std::unique_ptr<grenade::common::Population::Cell> copy() const override;
	virtual std::unique_ptr<grenade::common::Population::Cell> move() override;

protected:
	virtual bool is_equal_to(grenade::common::Population::Cell const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

	bool m_requires_time_domain;
};

} // namespace abstract
} // namespace grenade::vx::network
