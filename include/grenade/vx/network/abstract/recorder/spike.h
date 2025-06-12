#pragma once
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/recorder.h"
#include "hate/visibility.h"
#include <optional>
#include <set>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * SpikeRecorder in topology.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    visible,
    holder_type("std::shared_ptr<grenade::vx::network::abstract::SpikeRecorder>")) SpikeRecorder
    : public Recorder
{
	/**
	 * Construct spike recorder with specified number of channels and time-domain.
	 * @param shape Shape of spike sources to record
	 * @param time_domain Time domain onto which to place recorder
	 */
	SpikeRecorder(
	    grenade::common::MultiIndexSequence const& shape,
	    grenade::common::TimeDomainOnTopology const& time_domain);

	/**
	 * Recording containing spike times for each batch entry for each recorded channel.
	 */
	struct Results : public Recorder::Results
	{
		/**
		 * Spike times with shape (batch_size, size, #spikes).
		 */
		typedef std::vector<std::vector<std::vector<grenade::vx::common::Time>>> Spikes;
		Spikes spikes;

		Results(size_t batch_size, size_t size);

		Results(Spikes spikes);

		virtual size_t batch_size() const override;

		virtual void set_section(
		    Recorder::Results const& results,
		    grenade::common::MultiIndexSequence const& sequence) override;

		virtual size_t size() const override;

		std::unique_ptr<PortData> copy() const override;
		std::unique_ptr<PortData> move() override;

	protected:
		virtual bool is_equal_to(PortData const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;
	};

	virtual std::unique_ptr<Recorder::Results> create_empty_results(
	    size_t batch_size) const override;

	/**
	 * Only SpikeRecorder results are valid at output port 0.
	 */
	virtual bool valid_output_port_data(
	    size_t output_port_on_vertex, grenade::common::PortData const& results) const override;

	/**
	 * Get input ports of spike recorder.
	 * The spike recorder has one input port of to be recorded spikes from neurons (port index 0).
	 */
	virtual std::vector<Port> get_input_ports() const override;

	/**
	 * Get output ports of spike recorder.
	 * The spike recorder has one output port of the recorded spikes (port index 0).
	 */
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<grenade::common::Vertex> copy() const override;
	virtual std::unique_ptr<grenade::common::Vertex> move() override;

protected:
	std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
