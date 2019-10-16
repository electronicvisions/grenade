#include "grenade/vx/vertex/cadc_readout.h"

#include <sstream>
#include <stdexcept>
#include "halco/hicann-dls/vx/neuron.h"

namespace grenade::vx::vertex {

CADCMembraneReadoutView::CADCMembraneReadoutView(size_t const size) : m_size()
{
	if (size > halco::hicann_dls::vx::NeuronColumnOnDLS::size) {
		std::stringstream ss;
		ss << "CADMembraneReadoutView size " << size << " too large.";
		throw std::out_of_range(ss.str());
	}
	m_size = size;
}

std::array<Port, 1> CADCMembraneReadoutView::inputs() const
{
	return {Port(m_size, ConnectionType::MembraneVoltage)};
}

Port CADCMembraneReadoutView::output() const
{
	return Port(m_size, ConnectionType::Int8);
}

} // namespace grenade::vx::vertex
