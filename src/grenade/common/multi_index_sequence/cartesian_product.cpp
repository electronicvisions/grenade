#include "grenade/common/multi_index_sequence/cartesian_product.h"

#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include <stdexcept>


namespace grenade::common {

namespace {

void check_dimension_units(MultiIndexSequence const& first, MultiIndexSequence const& second)
{
	if ((first.size() == 0) != (second.size() == 0)) {
		throw std::invalid_argument("CartesianProductMultiIndexSequence with differing emptiness "
		                            "of sequences not supported.");
	}
}

} // namespace

CartesianProductMultiIndexSequence::CartesianProductMultiIndexSequence(
    MultiIndexSequence const& first, MultiIndexSequence const& second)
{
	check_dimension_units(first, second);
	m_first = first;
	m_second = second;
}

CartesianProductMultiIndexSequence::CartesianProductMultiIndexSequence(
    MultiIndexSequence&& first, MultiIndexSequence&& second)
{
	check_dimension_units(first, second);
	m_first = std::move(first);
	m_second = std::move(second);
}

MultiIndexSequence const& CartesianProductMultiIndexSequence::get_first() const
{
	return *m_first;
}

void CartesianProductMultiIndexSequence::set_first(MultiIndexSequence const& value)
{
	check_dimension_units(value, *m_second);
	m_first = value;
}

void CartesianProductMultiIndexSequence::set_first(MultiIndexSequence&& value)
{
	check_dimension_units(value, *m_second);
	m_first = std::move(value);
}

MultiIndexSequence const& CartesianProductMultiIndexSequence::get_second() const
{
	return *m_second;
}

void CartesianProductMultiIndexSequence::set_second(MultiIndexSequence const& value)
{
	check_dimension_units(*m_first, value);
	m_second = value;
}

void CartesianProductMultiIndexSequence::set_second(MultiIndexSequence&& value)
{
	check_dimension_units(*m_first, value);
	m_second = std::move(value);
}

CartesianProductMultiIndexSequence::DimensionUnits
CartesianProductMultiIndexSequence::get_dimension_units() const
{
	DimensionUnits ret = m_first->get_dimension_units();
	auto second_dimension_units = m_second->get_dimension_units();
	for (auto&& dimension_unit : second_dimension_units) {
		ret.push_back(std::move(dimension_unit));
	}
	return ret;
}

void CartesianProductMultiIndexSequence::set_dimension_units(DimensionUnits value)
{
	if (dimensionality() != value.size()) {
		throw std::invalid_argument("Dimension units' size doesn't match dimensionality of "
		                            "sequence.");
	}
	DimensionUnits value_first(m_first->dimensionality());
	DimensionUnits value_second(m_second->dimensionality());
	for (size_t i = 0; i < value_first.size(); ++i) {
		value_first.at(i) = std::move(value.at(i));
	}
	for (size_t i = 0; i < value_second.size(); ++i) {
		value_second.at(i) = std::move(value.at(i + value_first.size()));
	}
	m_first->set_dimension_units(std::move(value_first));
	m_second->set_dimension_units(std::move(value_second));
}

size_t CartesianProductMultiIndexSequence::dimensionality() const
{
	return m_first->dimensionality() + m_second->dimensionality();
}

size_t CartesianProductMultiIndexSequence::size() const
{
	return m_first->size() * m_second->size();
}

std::vector<MultiIndex> CartesianProductMultiIndexSequence::get_elements() const
{
	std::vector<MultiIndex> ret;
	auto const first_elements = m_first->get_elements();
	auto const second_elements = m_second->get_elements();
	for (auto const& first_element : first_elements) {
		for (auto const& second_element : second_elements) {
			auto element = first_element;
			element.value.insert(
			    element.value.end(), second_element.value.begin(), second_element.value.end());
			ret.push_back(std::move(element));
		}
	}
	return ret;
}

bool CartesianProductMultiIndexSequence::includes(MultiIndexSequence const& other) const
{
	if (auto const other_ptr = dynamic_cast<CartesianProductMultiIndexSequence const*>(&other);
	    other_ptr && other_ptr->m_first->dimensionality() == m_first->dimensionality() &&
	    other_ptr->m_second->dimensionality() == m_second->dimensionality()) {
		return m_first->includes(*other_ptr->m_first) && m_second->includes(*other_ptr->m_second);
	}
	auto const cuboid_first_ptr = dynamic_cast<CuboidMultiIndexSequence const*>(&*m_first);
	auto const cuboid_second_ptr = dynamic_cast<CuboidMultiIndexSequence const*>(&*m_second);
	if (cuboid_first_ptr && cuboid_second_ptr) {
		auto cuboid_product_sequence = cuboid_first_ptr->cartesian_product(*cuboid_second_ptr);
		return cuboid_product_sequence->includes(other);
	}
	return MultiIndexSequence::includes(other);
}

bool CartesianProductMultiIndexSequence::is_injective() const
{
	return m_first->is_injective() && m_second->is_injective();
}

std::unique_ptr<MultiIndexSequence> CartesianProductMultiIndexSequence::distinct_projection(
    std::set<size_t> const& dimensions) const
{
	if (auto max_element = std::max_element(dimensions.begin(), dimensions.end());
	    max_element != dimensions.end() && *max_element >= dimensionality()) {
		throw std::invalid_argument("Given dimensions don't match dimensionality.");
	}
	if (dimensions.empty()) {
		return std::make_unique<ListMultiIndexSequence>(ListMultiIndexSequence());
	}
	if (*std::max_element(dimensions.begin(), dimensions.end()) < m_first->dimensionality()) {
		return m_first->distinct_projection(dimensions);
	} else if (
	    *std::min_element(dimensions.begin(), dimensions.end()) >= m_first->dimensionality()) {
		std::set<size_t> second_dimensions;
		for (auto const& dimension : dimensions) {
			second_dimensions.insert(dimension - m_first->dimensionality());
		}
		return m_second->distinct_projection(second_dimensions);
	} else {
		std::set<size_t> first_dimensions;
		std::set<size_t> second_dimensions;
		for (auto const& dimension : dimensions) {
			if (dimension < m_first->dimensionality()) {
				first_dimensions.insert(dimension);
			} else {
				second_dimensions.insert(dimension - m_first->dimensionality());
			}
		}
		return std::make_unique<CartesianProductMultiIndexSequence>(
		    *m_first->distinct_projection(first_dimensions),
		    *m_second->distinct_projection(second_dimensions));
	}
}

std::unique_ptr<MultiIndexSequence> CartesianProductMultiIndexSequence::copy() const
{
	return std::make_unique<CartesianProductMultiIndexSequence>(*this);
}

std::unique_ptr<MultiIndexSequence> CartesianProductMultiIndexSequence::move()
{
	return std::make_unique<CartesianProductMultiIndexSequence>(std::move(*this));
}

bool CartesianProductMultiIndexSequence::is_equal_to(MultiIndexSequence const& other) const
{
	auto const& other_product = static_cast<CartesianProductMultiIndexSequence const&>(other);
	return m_first == other_product.m_first && m_second == other_product.m_second;
}

std::ostream& CartesianProductMultiIndexSequence::print(std::ostream& os) const
{
	return os << "CartesianProductMultiIndexSequence(" << m_first << ", " << m_second << ")";
}

} // namespace grenade::common
