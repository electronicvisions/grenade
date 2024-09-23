#pragma once
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/population.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/recorder.h"
#include "grenade/vx/signal_flow/types.h"
#include "hate/visibility.h"
#include <optional>
#include <set>

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * CADCRecorder in topology.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    visible,
    holder_type("std::shared_ptr<grenade::vx::network::abstract::CADCRecorder>")) CADCRecorder
    : public Recorder
{
	/**
	 * Construct CADC recorder with specified number of channels and time-domain.
	 * @param shape Shape of sources to record
	 * @param enable_record_to_dram Whether to enable storing recording on DRAM
	 * @param time_domain Time domain onto which to place recorder
	 */
	CADCRecorder(
	    grenade::common::MultiIndexSequence const& shape,
	    bool enable_record_to_dram,
	    grenade::common::TimeDomainOnTopology const& time_domain);

	/**
	 * Whether to store recording on DRAM or block memory on the FPGA.
	 */
	bool enable_record_to_dram;

	/**
	 * Results containing CADC samples for each batch entry for each recorded channel.
	 */
	struct Results : public Recorder::Results
	{
		/**
		 * Samples with shape (batch_size, size, #samples).
		 */
		typedef std::vector<
		    std::vector<std::vector<std::pair<grenade::vx::common::Time, signal_flow::Int8>>>>
		    Samples;
		Samples samples;

		Results(size_t batch_size, size_t size);

		Results(Samples samples);

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
	 * Only CADCRecorder results are valid at output port 0.
	 */
	virtual bool valid_output_port_data(
	    size_t output_port_on_vertex, grenade::common::PortData const& recording) const override;

	/**
	 * Get input ports of CADC recorder.
	 * The CADC recorder has one input port of samples to be recorded from recordable sources (port
	 * index 0).
	 */
	virtual std::vector<Port> get_input_ports() const override;

	/**
	 * Get output ports of CADC recorder.
	 * The CADC recorder has one output port of recorded samples (port index 0).
	 */
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	bool is_equal_to(Vertex const& other) const override;
	std::ostream& print(std::ostream& os) const override;
};

} // namespace abstract
} // namespace grenade::vx::network
