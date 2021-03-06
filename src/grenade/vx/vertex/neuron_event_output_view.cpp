#include "grenade/vx/vertex/neuron_event_output_view.h"

#include "grenade/cerealization.h"
#include "grenade/vx/port_restriction.h"
#include "grenade/vx/vertex/neuron_view.h"
#include "halco/common/cerealization_geometry.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::vertex {

NeuronEventOutputView::NeuronEventOutputView(Neurons const& neurons) : m_neurons()
{
	for (auto const& [_, columns_of_inputs] : neurons) {
		for (auto const& columns : columns_of_inputs) {
			std::set<Columns::value_type> unique(columns.begin(), columns.end());
			if (unique.size() != columns.size()) {
				throw std::runtime_error(
				    "Neuron locations provided to NeuronEventOutputView are not unique.");
			}
		}
	}
	m_neurons = neurons;
}

NeuronEventOutputView::Neurons const& NeuronEventOutputView::get_neurons() const
{
	return m_neurons;
}

std::vector<Port> NeuronEventOutputView::inputs() const
{
	std::vector<Port> ret;
	for (auto const& [_, columns_of_inputs] : m_neurons) {
		for (auto const& columns_of_input : columns_of_inputs) {
			ret.push_back(Port(columns_of_input.size(), ConnectionType::MembraneVoltage));
		}
	}
	return ret;
}

Port NeuronEventOutputView::output() const
{
	std::set<halco::hicann_dls::vx::v2::NeuronEventOutputOnDLS> outputs;
	for (auto const& [_, columns_of_inputs] : m_neurons) {
		for (auto const& columns_of_input : columns_of_inputs) {
			for (auto const& column : columns_of_input) {
				outputs.insert(column.toNeuronEventOutputOnDLS());
			}
		}
	}
	return Port(outputs.size(), ConnectionType::CrossbarInputLabel);
}

std::ostream& operator<<(std::ostream& os, NeuronEventOutputView const& config)
{
	os << "NeuronEventOutputView([";
	for (auto const& [row, columns_of_inputs] : config.m_neurons) {
		os << "row: " << row << " num_columns: ";
		size_t num_columns = 0;
		for (auto const& columns_of_input : columns_of_inputs) {
			num_columns += columns_of_input.size();
		}
		os << num_columns << "]";
		if (row != (config.m_neurons.size() - 1)) {
			os << ", " << std::endl;
		}
	}
	os << "])";
	return os;
}

bool NeuronEventOutputView::supports_input_from(
    NeuronView const& input, std::optional<PortRestriction> const& restriction) const
{
	if (!m_neurons.contains(input.get_row())) {
		return false;
	}
	auto const input_columns = input.get_columns();
	auto const& local_columns = m_neurons.at(input.get_row());
	if (!restriction) {
		if (std::find(local_columns.begin(), local_columns.end(), input_columns) ==
		    local_columns.end()) {
			return false;
		}
	} else {
		if (!restriction->is_restriction_of(input.output())) {
			throw std::runtime_error(
			    "Given restriction is not a restriction of input vertex output port.");
		}
		NeuronView::Columns restricted_input_columns(
		    input_columns.begin() + restriction->min(),
		    input_columns.begin() + restriction->max() + 1);
		if (std::find(local_columns.begin(), local_columns.end(), restricted_input_columns) ==
		    local_columns.end()) {
			return false;
		}
	}
	return true;
}

bool NeuronEventOutputView::operator==(NeuronEventOutputView const& other) const
{
	return (m_neurons == other.m_neurons);
}

bool NeuronEventOutputView::operator!=(NeuronEventOutputView const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void NeuronEventOutputView::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_neurons);
}

} // namespace grenade::vx::vertex

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::vertex::NeuronEventOutputView)
CEREAL_CLASS_VERSION(grenade::vx::vertex::NeuronEventOutputView, 1)
