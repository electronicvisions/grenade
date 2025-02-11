#include "grenade/vx/network/abstract/recorder/pad.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/vertex_port_type/analog_observable.h"
#include "grenade/vx/network/abstract/vertex_port_type/madc_samples.h"
#include "halco/hicann-dls/vx/v3/readout.h"
#include "hate/indent.h"
#include "hate/join.h"
#include <memory>

namespace grenade::vx::network::abstract {

PadRecorder::PadRecorder(
    std::vector<halco::hicann_dls::vx::v3::PadOnDLS> pads,
    grenade::common::MultiIndexSequence const& shape,
    grenade::common::TimeDomainOnTopology const& time_domain) :
    Recorder(shape, time_domain), m_pads()
{
	if (pads.size() != shape.size()) {
		throw std::invalid_argument(
		    "Number of pad locations doesn't match shape of channels of recorder.");
	}
	m_pads = std::move(pads);
}

std::vector<halco::hicann_dls::vx::v3::PadOnDLS> const& PadRecorder::get_pads() const
{
	return m_pads;
}

void PadRecorder::set_pads(std::vector<halco::hicann_dls::vx::v3::PadOnDLS> value)
{
	if (value.size() != get_shape().size()) {
		throw std::invalid_argument(
		    "Number of pad locations doesn't match shape of channels of recorder.");
	}
	m_pads = std::move(value);
}

std::vector<grenade::common::Vertex::Port> PadRecorder::get_input_ports() const
{
	return {Port(
	    AnalogObservable(), Port::SumOrSplitSupport::no,
	    Port::ExecutionInstanceTransitionConstraint::not_supported,
	    Port::RequiresOrGeneratesData::no, get_shape())};
}

std::vector<grenade::common::Vertex::Port> PadRecorder::get_output_ports() const
{
	return {};
}

std::unique_ptr<grenade::common::Vertex> PadRecorder::copy() const
{
	return std::make_unique<PadRecorder>(*this);
}

std::unique_ptr<grenade::common::Vertex> PadRecorder::move()
{
	return std::make_unique<PadRecorder>(std::move(*this));
}

bool PadRecorder::is_equal_to(Vertex const& other) const
{
	return Recorder::is_equal_to(other) && m_pads == static_cast<PadRecorder const&>(other).m_pads;
}

std::ostream& PadRecorder::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "PadRecorder(\n";
	ios << hate::Indentation("\t");
	Recorder::print(ios);
	ios << "\n";
	ios << "pads: " << hate::join(m_pads, ", ") << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
