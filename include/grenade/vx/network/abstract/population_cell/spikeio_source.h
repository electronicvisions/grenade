#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/population.h"
#include "grenade/common/port_data.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v3/fpga.h"
#include "haldls/vx/fpga.h"
#include "hate/visibility.h"
#include <vector>


namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * Neuron representing an external source via the FPGA-based SpikeIO interface.
 * This neuron can serve as source only.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) SpikeIOSourceNeuron
    : public grenade::common::Population::Cell
{
	struct ParameterSpace : public grenade::common::Population::Cell::ParameterSpace
	{
		struct Parameterization
		    : public grenade::common::Population::Cell::ParameterSpace::Parameterization
		{
			std::vector<halco::hicann_dls::vx::SpikeIOAddress> labels;
			haldls::vx::SpikeIOConfig::DataRateScaler data_rate_scaler{};

			Parameterization() = default;

			virtual size_t size() const override;

			virtual std::unique_ptr<
			    grenade::common::Population::Cell::ParameterSpace::Parameterization>
			get_section(grenade::common::MultiIndexSequence const& sequence) const override;

			virtual std::unique_ptr<PortData> copy() const override;
			virtual std::unique_ptr<PortData> move() override;

		protected:
			virtual bool is_equal_to(PortData const& other) const override;
			virtual std::ostream& print(std::ostream& os) const override;
		};

		explicit ParameterSpace(size_t size);

		virtual size_t size() const override;

		/**
		 * Only SpikeIOSourceNeuron parameterization is valid for input port 0.
		 */
		virtual bool valid(
		    size_t input_port_on_cell,
		    grenade::common::Population::Cell::ParameterSpace::Parameterization const&
		        parameterization) const override;

		virtual std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const override;

		virtual std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> copy()
		    const override;
		virtual std::unique_ptr<grenade::common::Population::Cell::ParameterSpace> move() override;

	protected:
		virtual bool is_equal_to(
		    grenade::common::Population::Cell::ParameterSpace const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;

	private:
		size_t m_size;
	};

	/**
	 * Only SpikeIOSourceNeuron parameter space is valid.
	 */
	virtual bool valid(
	    grenade::common::Population::Cell::ParameterSpace const& parameter_space) const override;

	/**
	 * SpikeIOSourceNeuron doesn't require dynamics, thererfore none are valid.
	 */
	virtual bool valid(
	    size_t input_port_on_cell,
	    grenade::common::Population::Cell::Dynamics const& dynamics) const override;

	/**
	 * SpikeIOSourceNeuron requires a time domain, since it is represented by physical circuits
	 * operating in time.
	 */
	virtual bool requires_time_domain() const override;

	/**
	 * SpikeIOSourceNeuron is partitionable onto an execution instance on the executor.
	 */
	virtual bool is_partitionable() const override;

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	/**
	 * Only ClockCycleTimeDomainRuntimes are valid.
	 */
	virtual bool valid(
	    grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const override;

	/**
	 * Get input ports.
	 * The SpikeIOSourceNeuron features one input port of parameterization to the neuron (port index
	 * 0), with one channel (0).
	 */
	virtual std::vector<grenade::common::Vertex::Port> get_input_ports() const override;

	/**
	 * Get output ports.
	 * The SpikeIOSourceNeuron features one output port of spikes from the neuron (port index
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