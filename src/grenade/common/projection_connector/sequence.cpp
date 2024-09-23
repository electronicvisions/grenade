#include "grenade/common/projection_connector/sequence.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include <stdexcept>

namespace grenade::common {

SequenceConnector::SequenceConnector(
    MultiIndexSequence const& input_sequence,
    MultiIndexSequence const& output_sequence,
    MultiIndexSequence const& connection_sequence) :
    m_input_sequence(), m_output_sequence()
{
	if (!input_sequence.cartesian_product(output_sequence)->includes(connection_sequence)) {
		throw std::invalid_argument("Connection sequence is required to be included in product of "
		                            "input and output sequence.");
	}
	m_input_sequence = input_sequence;
	m_output_sequence = output_sequence;
	m_connection_sequence = connection_sequence;
}

std::unique_ptr<MultiIndexSequence> SequenceConnector::get_input_sequence() const
{
	return m_input_sequence->copy();
}

std::unique_ptr<MultiIndexSequence> SequenceConnector::get_output_sequence() const
{
	return m_output_sequence->copy();
}

size_t SequenceConnector::get_num_synapse_parameterizations(
    MultiIndexSequence const& sequence) const
{
	return get_num_synapses(sequence);
}

std::unique_ptr<MultiIndexSequence> SequenceConnector::get_synapse_parameterization_indices(
    MultiIndexSequence const& sequence) const
{
	auto connector_sequence = get_input_sequence()->cartesian_product(*get_output_sequence());
	return CuboidMultiIndexSequence({m_connection_sequence->size()})
	    .related_sequence_subset_restriction(
	        *m_connection_sequence, *get_synapse_connections(sequence));
}

std::unique_ptr<MultiIndexSequence> SequenceConnector::get_synapse_connections(
    MultiIndexSequence const& sequence) const
{
	auto connector_sequence = m_input_sequence->cartesian_product(*m_output_sequence);
	if (!connector_sequence->includes(sequence)) {
		throw std::out_of_range(
		    "Given sequence not included in product of input and output sequence of connector.");
	}
	return m_connection_sequence->subset_restriction(sequence);
}

std::unique_ptr<Projection::Connector> SequenceConnector::get_section(
    MultiIndexSequence const& sequence) const
{
	if (!m_input_sequence->cartesian_product(*m_output_sequence)->includes(sequence)) {
		throw std::invalid_argument("Given sequence not included in the product of input and "
		                            "output sequence of connector.");
	}
	std::set<size_t> input_dimensions;
	for (size_t i = 0; i < m_input_sequence->dimensionality(); ++i) {
		input_dimensions.insert(i);
	}
	auto section_input_sequence =
	    m_input_sequence->subset_restriction(*sequence.distinct_projection(input_dimensions));
	std::set<size_t> output_dimensions;
	for (size_t i = 0; i < m_output_sequence->dimensionality(); ++i) {
		output_dimensions.insert(i + m_input_sequence->dimensionality());
	}
	auto section_output_sequence =
	    m_output_sequence->subset_restriction(*sequence.distinct_projection(output_dimensions));
	return std::make_unique<SequenceConnector>(
	    *section_input_sequence, *section_output_sequence,
	    *m_connection_sequence->subset_restriction(sequence));
}

std::unique_ptr<Projection::Connector> SequenceConnector::copy() const
{
	return std::make_unique<SequenceConnector>(*this);
}

std::unique_ptr<Projection::Connector> SequenceConnector::move()
{
	return std::make_unique<SequenceConnector>(std::move(*this));
}

std::ostream& SequenceConnector::print(std::ostream& os) const
{
	os << "SequenceConnector(input: " << m_input_sequence << ", output: " << m_output_sequence
	   << ", connection: " << m_connection_sequence << ")";
	return os;
}

bool SequenceConnector::is_equal_to(Projection::Connector const& other) const
{
	auto const& other_connector = static_cast<SequenceConnector const&>(other);
	return m_input_sequence == other_connector.m_input_sequence &&
	       m_output_sequence == other_connector.m_output_sequence &&
	       m_connection_sequence == other_connector.m_connection_sequence;
}

} // namespace grenade::common
