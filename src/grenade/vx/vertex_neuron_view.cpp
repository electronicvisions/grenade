#include "grenade/vx/vertex/neuron_view.h"

#include <stdexcept>
#include "halco/hicann-dls/vx/neuron.h"

namespace grenade::vx::vertex {

NeuronView::NeuronView(neurons_type const& neurons) : m_neurons()
{
	std::set<neurons_type::value_type> unique(neurons.begin(), neurons.end());
	if (unique.size() != neurons.size()) {
		throw std::runtime_error("Neuron locations provided to NeuronView are not unique.");
	}
	m_neurons = neurons;
}

NeuronView::neurons_type::const_iterator NeuronView::begin() const
{
	return m_neurons.begin();
}

NeuronView::neurons_type::const_iterator NeuronView::end() const
{
	return m_neurons.end();
}

std::array<Port, 1> NeuronView::inputs() const
{
	return {Port(m_neurons.size(), ConnectionType::SynapticInput)};
}

Port NeuronView::output() const
{
	return Port(m_neurons.size(), ConnectionType::MembraneVoltage);
}

} // namespace grenade::vx::vertex
