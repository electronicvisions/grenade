#include "grenade/common/multi_index_sequence.h"

#include "grenade/common/multi_index_sequence/list.h"
#include "hate/join.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace grenade::common {

MultiIndexSequence::DimensionUnit::~DimensionUnit() {}


MultiIndexSequence::~MultiIndexSequence() {}

bool MultiIndexSequence::includes(MultiIndexSequence const& other) const
{
	if (get_dimension_units() != other.get_dimension_units()) {
		std::stringstream ss;
		ss << "Dimension units between sequences don't match:\n";
		ss << "this:  " << hate::join(get_dimension_units(), ", ") << "\n";
		ss << "other: " << hate::join(other.get_dimension_units(), ", ") << "\n";
		ss << "for sequences:\n";
		ss << "this:  " << *this << "\n";
		ss << "other: " << other << "\n";
		throw std::invalid_argument(ss.str());
	}
	auto elements = get_elements();
	std::sort(elements.begin(), elements.end());
	auto const unique_end = std::unique(elements.begin(), elements.end());
	elements.erase(unique_end, elements.end());
	auto other_elements = other.get_elements();
	std::sort(other_elements.begin(), other_elements.end());
	auto const other_unique_end = std::unique(other_elements.begin(), other_elements.end());
	other_elements.erase(other_unique_end, other_elements.end());
	auto ret = std::includes(
	    elements.begin(), elements.end(), other_elements.begin(), other_elements.end());
	return ret;
}

bool MultiIndexSequence::is_injective() const
{
	auto const elements = get_elements();
	return std::set<MultiIndex>{elements.begin(), elements.end()}.size() == elements.size();
}

bool MultiIndexSequence::is_disjunct(MultiIndexSequence const& other) const
{
	auto const subset_restricted_sequence = subset_restriction(other);
	assert(subset_restricted_sequence);
	return subset_restricted_sequence->size() == 0;
}

std::unique_ptr<MultiIndexSequence> MultiIndexSequence::subset_restriction(
    MultiIndexSequence const& other) const
{
	if (other.includes(*this)) {
		return copy();
	}
	auto dimension_units = get_dimension_units();
	if (dimension_units != other.get_dimension_units()) {
		std::stringstream ss;
		ss << "Dimension units between sequences don't match:\n";
		ss << "this:  " << hate::join(dimension_units, ", ") << "\n";
		ss << "other: " << hate::join(other.get_dimension_units(), ", ") << "\n";
		ss << "for sequences:\n";
		ss << "this:  " << *this << "\n";
		ss << "other: " << other << "\n";
		throw std::invalid_argument(ss.str());
	}
	auto elements = get_elements();
	auto other_elements = other.get_elements();
	std::vector<MultiIndex> restricted_elements;
	for (auto& element : elements) {
		if (std::find(other_elements.begin(), other_elements.end(), element) !=
		    other_elements.end()) {
			restricted_elements.emplace_back(std::move(element));
		}
	}
	return std::make_unique<ListMultiIndexSequence>(
	    std::move(restricted_elements), std::move(dimension_units));
}

std::unique_ptr<MultiIndexSequence> MultiIndexSequence::related_sequence_subset_restriction(
    MultiIndexSequence const& related, MultiIndexSequence const& subset) const
{
	if (size() != related.size()) {
		std::stringstream ss;
		ss << "Given sequence and related sequence don't form a bijective mapping:\n";
		ss << "this:    " << size() << "\n";
		ss << "related: " << related.size() << "\n";
		ss << "for sequences:\n";
		ss << "this:    " << *this << "\n";
		ss << "related: " << related << "\n";
		throw std::invalid_argument(ss.str());
	}
	if (subset.includes(related)) {
		return copy();
	}
	auto dimension_units = get_dimension_units();
	if (related.get_dimension_units() != subset.get_dimension_units()) {
		std::stringstream ss;
		ss << "Dimension units between sequences don't match:\n";
		ss << "related:   " << hate::join(related.get_dimension_units(), ", ") << "\n";
		ss << "subset: " << hate::join(subset.get_dimension_units(), ", ") << "\n";
		ss << "for sequences:\n";
		ss << "this:   " << related << "\n";
		ss << "subset: " << subset << "\n";
		throw std::invalid_argument(ss.str());
	}
	auto elements = get_elements();
	auto const related_elements = related.get_elements();
	auto const subset_elements = subset.get_elements();
	std::vector<MultiIndex> restricted_elements;
	for (size_t index = 0; auto& element : related_elements) {
		if (std::find(subset_elements.begin(), subset_elements.end(), element) !=
		    subset_elements.end()) {
			restricted_elements.emplace_back(std::move(elements.at(index)));
		}
		index++;
	}
	return std::make_unique<ListMultiIndexSequence>(
	    std::move(restricted_elements), get_dimension_units());
}

std::unique_ptr<MultiIndexSequence> MultiIndexSequence::projection(
    std::set<size_t> const& dimensions) const
{
	if (auto max_element = std::max_element(dimensions.begin(), dimensions.end());
	    max_element != dimensions.end() && *max_element >= dimensionality()) {
		throw std::invalid_argument("Given dimensions don't match dimensionality.");
	}
	auto elements = get_elements();
	std::vector<MultiIndex> projected_elements;
	for (auto const& element : elements) {
		MultiIndex projected_element;
		for (auto const& dimension : dimensions) {
			projected_element.value.push_back(element.value.at(dimension));
		}
		projected_elements.push_back(projected_element);
	}
	DimensionUnits new_dimension_units;
	auto dimension_units = get_dimension_units();
	if (!dimension_units.empty()) {
		for (auto const& dimension : dimensions) {
			new_dimension_units.push_back(std::move(dimension_units.at(dimension)));
		}
	}
	return std::make_unique<ListMultiIndexSequence>(
	    std::move(projected_elements), new_dimension_units);
}

std::unique_ptr<MultiIndexSequence> MultiIndexSequence::distinct_projection(
    std::set<size_t> const& dimensions) const
{
	auto const projection_ptr = projection(dimensions);
	assert(projection_ptr);
	auto elements = projection_ptr->get_elements();
	std::set<MultiIndex> projected_elements(elements.begin(), elements.end());
	return std::make_unique<ListMultiIndexSequence>(
	    std::vector<MultiIndex>(projected_elements.begin(), projected_elements.end()),
	    projection_ptr->get_dimension_units());
}

std::unique_ptr<MultiIndexSequence> MultiIndexSequence::cartesian_product(
    MultiIndexSequence const& other) const
{
	std::vector<MultiIndex> ret;
	auto const elements = get_elements();
	auto const other_elements = other.get_elements();
	for (auto const& element : elements) {
		for (auto const& other_element : other_elements) {
			auto new_element_value = element.value;
			new_element_value.insert(
			    new_element_value.end(), other_element.value.begin(), other_element.value.end());
			ret.push_back(MultiIndex(std::move(new_element_value)));
		}
	}
	if (get_dimension_units().empty() != other.get_dimension_units().empty()) {
		throw std::invalid_argument(
		    "Product of sequences with differing optionality of dimension units not supported.");
	}
	auto new_dimension_units = get_dimension_units();
	auto other_dimension_units = other.get_dimension_units();
	new_dimension_units.insert(
	    new_dimension_units.end(), other_dimension_units.begin(), other_dimension_units.end());
	return std::make_unique<ListMultiIndexSequence>(std::move(ret), new_dimension_units);
}

std::vector<std::unique_ptr<MultiIndexSequence>> MultiIndexSequence::slice(
    std::set<size_t> indices) const
{
	auto elements = get_elements();
	std::vector<std::unique_ptr<MultiIndexSequence>> ret;
	if (indices.empty()) {
		ret.push_back(copy());
		return ret;
	}
	if (*std::max_element(indices.begin(), indices.end()) >= size()) {
		throw std::invalid_argument("Given indices out of range of sequence.");
	}
	std::vector<size_t> indices_vector(indices.begin(), indices.end());
	ret.push_back(std::make_unique<ListMultiIndexSequence>(
	    std::vector<MultiIndex>(elements.begin(), elements.begin() + indices_vector.at(0)),
	    get_dimension_units()));
	for (size_t i = 0; i < indices_vector.size() - 1; ++i) {
		ret.push_back(std::make_unique<ListMultiIndexSequence>(
		    std::vector<MultiIndex>(
		        elements.begin() + indices_vector.at(i),
		        elements.begin() + indices_vector.at(i + 1)),
		    get_dimension_units()));
	}
	ret.push_back(std::make_unique<ListMultiIndexSequence>(
	    std::vector<MultiIndex>(
	        elements.begin() + indices_vector.at(indices.size() - 1), elements.end()),
	    get_dimension_units()));
	return ret;
}

std::string MultiIndexSequence::print_dimension_unit(
    DimensionUnits::value_type const& dimension_unit)
{
	if (dimension_unit) {
		std::stringstream ss;
		ss << dimension_unit;
		return ss.str();
	}
	return std::string("unitless");
}

} // namespace grenade::common
