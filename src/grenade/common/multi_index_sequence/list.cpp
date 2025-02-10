#include "grenade/common/multi_index_sequence/list.h"

#include "grenade/common/multi_index_sequence.h"
#include "hate/join.h"
#include <cassert>
#include <ostream>
#include <stdexcept>

namespace grenade::common {

ListMultiIndexSequence::ListMultiIndexSequence(std::vector<MultiIndex> elements) :
    m_elements(), m_dimension_units()
{
	if (!elements.empty() &&
	    std::any_of(elements.begin(), elements.end(), [elements](auto const& element) {
		    return element.value.size() != elements.at(0).value.size();
	    })) {
		throw std::runtime_error("Elements feature heterogeneous dimensionality.");
	}
	m_elements = std::move(elements);
	if (!m_elements.empty()) {
		m_dimension_units.resize(m_elements.at(0).value.size());
	}
}

ListMultiIndexSequence::ListMultiIndexSequence(
    std::vector<MultiIndex> elements,
    std::vector<
        std::optional<std::reference_wrapper<MultiIndexSequence::DimensionUnit const>>> const&
        dimension_units) :
    m_elements(), m_dimension_units()
{
	if (!elements.empty() &&
	    std::any_of(elements.begin(), elements.end(), [elements](auto const& element) {
		    return element.value.size() != elements.at(0).value.size();
	    })) {
		throw std::runtime_error("Elements feature heterogeneous dimensionality.");
	}
	if (!elements.empty() && dimension_units.size() != elements.at(0).value.size()) {
		throw std::runtime_error("Dimension units' size doesn't match dimensionality of "
		                         "elements.");
	}
	m_elements = std::move(elements);
	for (auto const& dimension_unit : dimension_units) {
		if (dimension_unit) {
			m_dimension_units.push_back(dimension_unit->get());
		} else {
			m_dimension_units.push_back({});
		}
	}
}

ListMultiIndexSequence::ListMultiIndexSequence(
    std::vector<MultiIndex> elements, DimensionUnits dimension_units) :
    m_elements(), m_dimension_units()
{
	if (!elements.empty() &&
	    std::any_of(elements.begin(), elements.end(), [elements](auto const& element) {
		    return element.value.size() != elements.at(0).value.size();
	    })) {
		throw std::invalid_argument("Elements feature heterogeneous dimensionality.");
	}
	if (!elements.empty() && dimension_units.size() != elements.at(0).value.size()) {
		throw std::invalid_argument("Dimension units' size doesn't match dimensionality of "
		                            "elements.");
	}
	m_elements = std::move(elements);
	m_dimension_units = std::move(dimension_units);
}

size_t ListMultiIndexSequence::dimensionality() const
{
	if (!m_elements.empty()) {
		return m_elements.at(0).value.size();
	}
	return m_dimension_units.size();
}

size_t ListMultiIndexSequence::size() const
{
	return m_elements.size();
}

std::vector<MultiIndex> ListMultiIndexSequence::get_elements() const
{
	return m_elements;
}

void ListMultiIndexSequence::set_elements(std::vector<MultiIndex> const& value)
{
	if (!value.empty() && std::any_of(value.begin(), value.end(), [value](auto const& element) {
		    return element.value.size() != value.at(0).value.size();
	    })) {
		throw std::invalid_argument("Elements feature heterogeneous dimensionality.");
	}
	if (!value.empty() && m_dimension_units.size() != value.at(0).value.size()) {
		throw std::invalid_argument("Dimension units' size doesn't match dimensionality of "
		                            "elements.");
	}
	m_elements = value;
}

void ListMultiIndexSequence::set_elements(std::vector<MultiIndex>&& value)
{
	if (!value.empty() && std::any_of(value.begin(), value.end(), [value](auto const& element) {
		    return element.value.size() != value.at(0).value.size();
	    })) {
		throw std::invalid_argument("Elements feature heterogeneous dimensionality.");
	}
	if (!value.empty() && m_dimension_units.size() != value.at(0).value.size()) {
		throw std::invalid_argument("Dimension units' size doesn't match dimensionality of "
		                            "elements.");
	}
	m_elements = std::move(value);
}

ListMultiIndexSequence::DimensionUnits ListMultiIndexSequence::get_dimension_units() const
{
	return m_dimension_units;
}

void ListMultiIndexSequence::set_dimension_units(DimensionUnits value)
{
	if (!m_elements.empty() && value.size() != m_elements.at(0).value.size()) {
		throw std::invalid_argument("Dimension units' size doesn't match dimensionality of "
		                            "elements.");
	}
	m_dimension_units = std::move(value);
}

std::unique_ptr<MultiIndexSequence> ListMultiIndexSequence::copy() const
{
	return std::make_unique<ListMultiIndexSequence>(*this);
}

std::unique_ptr<MultiIndexSequence> ListMultiIndexSequence::move()
{
	return std::make_unique<ListMultiIndexSequence>(std::move(*this));
}

std::ostream& ListMultiIndexSequence::print(std::ostream& os) const
{
	return os << "ListMultiIndexSequence([" << hate::join(m_elements, ", ")
	          << "], dimension_units: ["
	          << hate::join(m_dimension_units, ", ", print_dimension_unit) << "])";
}

bool ListMultiIndexSequence::is_equal_to(MultiIndexSequence const& other) const
{
	auto const& other_sequence = static_cast<ListMultiIndexSequence const&>(other);
	return m_elements == other_sequence.m_elements &&
	       m_dimension_units == other_sequence.m_dimension_units;
}

} // namespace grenade::common
