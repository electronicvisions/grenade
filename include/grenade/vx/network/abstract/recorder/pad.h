#pragma once
#include "grenade/common/multi_index_sequence.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/recorder.h"
#include "grenade/vx/signal_flow/vertex/pad_readout.h"
#include "halco/hicann-dls/vx/v3/readout.h"
#include "hate/visibility.h"

namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK_ABSTRACT {

/**
 * PadRecorder in topology.
 * While it fulfills the recorder interface, it doesn't yield recorded data, but merely connects the
 * sources to physical pads on the xBoard.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    visible,
    holder_type("std::shared_ptr<grenade::vx::network::abstract::PadRecorder>")) PadRecorder
    : public Recorder
{
	struct Parameterization : public grenade::common::PortData
	{
		std::vector<bool> enable_buffered{false};

		Parameterization();

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(PortData const& other) const override;
	};

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	/**
	 * Construct pad recorder with specified pad location, number of channels and time-domain.
	 * @param pads Pad locations to use
	 * @param shape Shape of sources to record
	 * @param time_domain Time domain onto which to place recorder
	 * @throws std::invalid_argument On number of pad locations not matching shape size of recorder.
	 */
	PadRecorder(
	    std::vector<halco::hicann_dls::vx::v3::PadOnDLS> pads,
	    grenade::common::MultiIndexSequence const& shape,
	    grenade::common::TimeDomainOnTopology const& time_domain);

	/**
	 * Which pad to target per channel.
	 */
	std::vector<halco::hicann_dls::vx::v3::PadOnDLS> const& get_pads() const;

	/**
	 * Set which pad to target per channel.
	 * @throws std::invalid_argument On number of pad locations not matching shape size of recorder.
	 */
	void set_pads(std::vector<halco::hicann_dls::vx::v3::PadOnDLS> value);

	/**
	 * Only Parameterization is valid at output port 1.
	 */
	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, grenade::common::PortData const& input_data) const override;

	/**
	 * Get input ports of pad recorder.
	 * The pad recorder has one input port of to be recorded sources (port index 0).
	 */
	virtual std::vector<Port> get_input_ports() const override;

	/**
	 * Get output ports of pad recorder.
	 * The pad recorder has no output ports.
	 */
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	bool is_equal_to(Vertex const& other) const override;
	std::ostream& print(std::ostream& os) const override;

	std::vector<halco::hicann_dls::vx::v3::PadOnDLS> m_pads;
};

} // namespace abstract
} // namespace grenade::vx::network
