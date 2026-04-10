#include "grenade/vx/network/routing/csp/vertex/source.h"
#include "grenade/vx/network/routing/csp/helper/bitset_beautifier.h"
#include "hate/indent.h"

namespace grenade::vx::network::routing::csp {

Source::Source(size_t number_label_bits, size_t num_labels, Gecode::Space& space) :
    m_number_label_bits(number_label_bits)
{
	m_labels.resize(num_labels);
	for (size_t i = 0; i < num_labels; i++) {
		m_labels.at(i) = Gecode::IntVar(space, 0, (1 << number_label_bits) - 1);
	}
}

Source::~Source() {}

void Source::update(Gecode::Space& space, RoutingVertex& other)
{
	auto& other_casted = dynamic_cast<Source&>(other);
	for (size_t i = 0; i < m_labels.size(); i++) {
		m_labels.at(i).update(space, other_casted.m_labels.at(i));
	}
}

std::string Source::get_name() const
{
	return "Source";
}

Gecode::IntVarArgs Source::get_parameters() const
{
	Gecode::IntVarArgs ret;
	for (auto const& label : m_labels) {
		ret << label;
	}
	return ret;
}

std::vector<std::string> Source::get_parameter_names() const
{
	std::vector<std::string> ret;
	for (size_t i = 0; i < size(); i++) {
		ret.push_back("Label(" + std::to_string(i) + ")");
	}
	return ret;
}

size_t Source::size() const
{
	return m_labels.size();
}

Gecode::IntVar const& Source::get_label(size_t index) const
{
	if (index > m_labels.size()) {
		throw std::out_of_range("Index out of bounds of labels.");
	}
	return m_labels.at(index);
}

halco::hicann_dls::vx::v3::SpikeLabel Source::get_assigned_label(size_t index) const
{
	auto const& label = get_label(index);
	if (!label.assigned()) {
		throw std::runtime_error("Source label not uniquely assigned.");
	}
	return halco::hicann_dls::vx::v3::SpikeLabel(label.val());
}

std::unique_ptr<RoutingVertex> Source::copy() const
{
	return std::make_unique<Source>(*this);
}

std::unique_ptr<RoutingVertex> Source::move()
{
	return std::make_unique<Source>(std::move(*this));
}

std::vector<size_t> Source::get_parameter_bitcounts() const
{
	return std::vector<size_t>(m_labels.size(), m_number_label_bits);
}

bool Source::is_equal_to(RoutingVertex const&) const
{
	return false;
}

} // namespace grenade::vx::network::routing::csp