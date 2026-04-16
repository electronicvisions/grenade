#include "grenade/common/multi_index_sequence/cuboid.h"

#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "hate/join.h"
#include "hate/multidim_iterator.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <ostream>

namespace grenade::common {

CuboidMultiIndexSequence::CuboidMultiIndexSequence(std::vector<size_t> shape, MultiIndex origin) :
    m_shape(), m_origin(), m_dimension_units()
{
	if (shape.size() != origin.value.size()) {
		throw std::invalid_argument("Shape and origin dimensionality don't match.");
	}
	m_shape = std::move(shape);
	m_origin = std::move(origin);
	m_dimension_units.resize(m_shape.size());
}

CuboidMultiIndexSequence::CuboidMultiIndexSequence(
    std::vector<size_t> shape,
    MultiIndex origin,
    std::vector<std::optional<std::reference_wrapper<DimensionUnit const>>> const&
        dimension_units) :
    m_shape(), m_origin(), m_dimension_units()
{
	if (shape.size() != origin.value.size()) {
		throw std::invalid_argument("Shape and origin dimensionality don't match.");
	}
	if (shape.size() != dimension_units.size()) {
		throw std::invalid_argument("Dimension unit's size doesn't match dimensionality of shape.");
	}
	m_shape = std::move(shape);
	m_origin = std::move(origin);
	for (auto const& dimension_unit : dimension_units) {
		if (dimension_unit) {
			m_dimension_units.push_back(dimension_unit->get());
		} else {
			m_dimension_units.push_back({});
		}
	}
}

CuboidMultiIndexSequence::CuboidMultiIndexSequence(
    std::vector<size_t> shape, MultiIndex origin, DimensionUnits dimension_units) :
    m_shape(), m_origin(), m_dimension_units()
{
	if (shape.size() != origin.value.size()) {
		throw std::invalid_argument("Shape and origin dimensionality don't match.");
	}
	if (shape.size() != dimension_units.size()) {
		throw std::invalid_argument("Dimension unit's size doesn't match dimensionality of shape.");
	}
	m_shape = std::move(shape);
	m_origin = std::move(origin);
	m_dimension_units = std::move(dimension_units);
}

CuboidMultiIndexSequence::CuboidMultiIndexSequence(std::vector<size_t> shape) :
    m_shape(std::move(shape)), m_origin(), m_dimension_units()
{
	m_origin = MultiIndex(std::vector<size_t>(m_shape.size(), 0));
	m_dimension_units.resize(m_shape.size());
}

CuboidMultiIndexSequence::CuboidMultiIndexSequence(
    std::vector<size_t> shape,
    std::vector<std::optional<std::reference_wrapper<DimensionUnit const>>> const&
        dimension_units) :
    m_shape(), m_origin(), m_dimension_units()
{
	if (shape.size() != dimension_units.size()) {
		throw std::invalid_argument("Dimension unit's size doesn't match dimensionality of shape.");
	}
	m_shape = std::move(shape);
	m_origin = MultiIndex(std::vector<size_t>(m_shape.size(), 0));
	for (auto const& dimension_unit : dimension_units) {
		if (dimension_unit) {
			m_dimension_units.push_back(dimension_unit->get());
		} else {
			m_dimension_units.push_back({});
		}
	}
}

CuboidMultiIndexSequence::CuboidMultiIndexSequence(
    std::vector<size_t> shape, DimensionUnits dimension_units) :
    m_shape(), m_origin(), m_dimension_units()
{
	if (shape.size() != dimension_units.size()) {
		throw std::invalid_argument("Dimension unit's size doesn't match dimensionality of shape.");
	}
	m_shape = std::move(shape);
	m_origin = MultiIndex(std::vector<size_t>(m_shape.size(), 0));
	m_dimension_units = std::move(dimension_units);
}

size_t CuboidMultiIndexSequence::dimensionality() const
{
	return m_shape.size();
}

size_t CuboidMultiIndexSequence::size() const
{
	if (m_shape.empty()) {
		return 0;
	}
	size_t ret = 1;
	for (auto const s : m_shape) {
		ret *= s;
	}
	return ret;
}

std::vector<MultiIndex> CuboidMultiIndexSequence::get_elements() const
{
	std::vector<MultiIndex> ret;
	if (size() == 0) {
		return ret;
	}
	std::vector<int64_t> const s(m_shape.begin(), m_shape.end());
	for (hate::MultidimIterator it(s); it != it.end(); ++it) {
		ret.emplace_back(MultiIndex(std::vector<size_t>((*it).begin(), (*it).end())));
	}
	auto const origin = get_origin();
	for (auto& point : ret) {
		for (size_t d = 0; d < point.value.size(); ++d) {
			point.value.at(d) += origin.value.at(d);
		}
	}
	return ret;
}

MultiIndex CuboidMultiIndexSequence::get_origin() const
{
	return m_origin;
}

void CuboidMultiIndexSequence::set_origin(MultiIndex value)
{
	if (m_shape.size() != value.value.size()) {
		throw std::invalid_argument("Shape and origin dimensionality don't match.");
	}
	m_origin = value;
}

std::vector<size_t> CuboidMultiIndexSequence::get_shape() const
{
	return m_shape;
}

void CuboidMultiIndexSequence::set_shape(std::vector<size_t> value)
{
	if (value.size() != m_origin.value.size()) {
		throw std::invalid_argument("Shape and origin dimensionality don't match.");
	}
	m_shape = value;
}

CuboidMultiIndexSequence::DimensionUnits CuboidMultiIndexSequence::get_dimension_units() const
{
	return m_dimension_units;
}

void CuboidMultiIndexSequence::set_dimension_units(DimensionUnits value)
{
	if (m_shape.size() != value.size()) {
		throw std::invalid_argument("Dimension unit's size doesn't match dimensionality of shape.");
	}
	m_dimension_units = std::move(value);
}

bool CuboidMultiIndexSequence::includes(MultiIndexSequence const& other) const
{
	if (m_dimension_units != other.get_dimension_units()) {
		std::stringstream ss;
		ss << "Dimension units between sequences don't match:\n";
		ss << "this:  " << hate::join(get_dimension_units(), ", ") << "\n";
		ss << "other: " << hate::join(other.get_dimension_units(), ", ") << "\n";
		ss << "for sequences:\n";
		ss << "this:  " << *this << "\n";
		ss << "other: " << other << "\n";
		throw std::invalid_argument(ss.str());
	}
	if (other.size() == 0) {
		return true;
	}
	auto const min_point = get_origin();
	auto max_point = get_origin();
	for (size_t i = 0; i < dimensionality(); ++i) {
		max_point.value.at(i) += m_shape.at(i);
	}
	if (auto const ptr = dynamic_cast<CuboidMultiIndexSequence const*>(&other); ptr) {
		auto const other_min_point = ptr->get_origin();
		auto other_max_point = ptr->get_origin();
		for (size_t i = 0; i < dimensionality(); ++i) {
			other_max_point.value.at(i) += ptr->m_shape.at(i);
		}
		for (size_t i = 0; i < dimensionality(); ++i) {
			if (min_point.value.at(i) > other_min_point.value.at(i) ||
			    max_point.value.at(i) < other_max_point.value.at(i)) {
				return false;
			}
		}
	} else {
		auto const other_elements = other.get_elements();
		for (auto const& other_point : other_elements) {
			for (size_t i = 0; i < dimensionality(); ++i) {
				if (min_point.value.at(i) > other_point.value.at(i) ||
				    max_point.value.at(i) <= other_point.value.at(i)) {
					return false;
				}
			}
		}
	}
	return true;
}

bool CuboidMultiIndexSequence::is_injective() const
{
	return true;
}

std::unique_ptr<MultiIndexSequence> CuboidMultiIndexSequence::subset_restriction(
    MultiIndexSequence const& other) const
{
	if (m_dimension_units != other.get_dimension_units()) {
		throw std::invalid_argument("Dimension units between sequences doesn't match.");
	}
	if (dimensionality() != other.dimensionality()) {
		throw std::invalid_argument("Dimensionality between sequences doesn't match.");
	}
	if (size() == 0) {
		return copy();
	}
	auto const min_point = get_origin();
	auto max_point = get_origin();
	for (size_t i = 0; i < dimensionality(); ++i) {
		max_point.value.at(i) += m_shape.at(i);
	}
	if (auto const ptr = dynamic_cast<CuboidMultiIndexSequence const*>(&other); ptr) {
		auto const other_min_point = ptr->get_origin();
		auto other_max_point = ptr->get_origin();
		for (size_t i = 0; i < dimensionality(); ++i) {
			other_max_point.value.at(i) += ptr->m_shape.at(i);
		}
		auto new_min_point = min_point;
		auto new_max_point = max_point;
		for (size_t i = 0; i < dimensionality(); ++i) {
			new_min_point.value.at(i) =
			    std::max(new_min_point.value.at(i), other_min_point.value.at(i));
			new_max_point.value.at(i) =
			    std::min(new_max_point.value.at(i), other_max_point.value.at(i));
		}
		std::vector<size_t> new_shape;
		for (size_t i = 0; i < dimensionality(); ++i) {
			new_shape.push_back(
			    new_max_point.value.at(i) >= new_min_point.value.at(i)
			        ? new_max_point.value.at(i) - new_min_point.value.at(i)
			        : 0);
		}
		return std::make_unique<CuboidMultiIndexSequence>(
		    std::move(new_shape), std::move(new_min_point), m_dimension_units);
	}
	auto other_elements = other.get_elements();
	std::set<MultiIndex> unique_other_elements(other_elements.begin(), other_elements.end());
	std::vector<MultiIndex> ret;
	auto const inside = [this, min_point, max_point](MultiIndex const& point) {
		for (size_t i = 0; i < dimensionality(); ++i) {
			if (min_point.value.at(i) > point.value.at(i) ||
			    max_point.value.at(i) <= point.value.at(i)) {
				return false;
			}
		}
		return true;
	};
	for (auto const& other_point : unique_other_elements) {
		if (inside(other_point)) {
			ret.push_back(other_point);
		}
	}
	return std::make_unique<ListMultiIndexSequence>(std::move(ret), m_dimension_units);
}

std::unique_ptr<MultiIndexSequence> CuboidMultiIndexSequence::related_sequence_subset_restriction(
    MultiIndexSequence const& related, MultiIndexSequence const& subset) const
{
	if (size() != related.size()) {
		throw std::invalid_argument(
		    "Given sequence and related sequence don't form a bijective mapping.");
	}
	if (auto const related_ptr = dynamic_cast<CuboidMultiIndexSequence const*>(&related);
	    related_ptr) {
		if (auto const subset_ptr = dynamic_cast<CuboidMultiIndexSequence const*>(&subset);
		    subset_ptr) {
			if (related.includes(subset)) {
				// Dimensions of shape 1 don't alter the iteration order, they just insert a
				// `index=constant` for this dimension for all elements. We can calculate the
				// related sequence subset restriction if the size > 1 dimensions are of the same
				// order and are equal. If not, we need to go back to calculating it via iterating
				// all elements (the default impl. of MultiIndexSequence).
				std::vector<size_t> this_dimensions_of_size_not_1;
				std::vector<size_t> this_stripped_shape;
				for (size_t i = 0; i < m_shape.size(); ++i) {
					if (m_shape.at(i) != 1) {
						this_stripped_shape.push_back(m_shape.at(i));
						this_dimensions_of_size_not_1.push_back(i);
					}
				}
				std::vector<size_t> related_dimensions_of_size_not_1;
				std::vector<size_t> related_stripped_shape;
				for (size_t i = 0; i < related_ptr->m_shape.size(); ++i) {
					if (related_ptr->m_shape.at(i) != 1) {
						related_stripped_shape.push_back(related_ptr->m_shape.at(i));
						related_dimensions_of_size_not_1.push_back(i);
					}
				}
				if (this_stripped_shape == related_stripped_shape) {
					auto new_origin = m_origin;
					auto new_shape = m_shape;
					for (size_t i = 0; i < this_stripped_shape.size(); ++i) {
						new_origin.value.at(this_dimensions_of_size_not_1.at(i)) +=
						    subset_ptr->m_origin.value.at(related_dimensions_of_size_not_1.at(i)) -
						    related_ptr->m_origin.value.at(related_dimensions_of_size_not_1.at(i));
						new_shape.at(this_dimensions_of_size_not_1.at(i)) =
						    subset_ptr->m_shape.at(related_dimensions_of_size_not_1.at(i));
					}
					return std::make_unique<CuboidMultiIndexSequence>(
					    std::move(new_shape), std::move(new_origin), m_dimension_units);
				}
			} else {
				auto const restricted_subset = subset.subset_restriction(related);
				assert(restricted_subset);
				return related_sequence_subset_restriction(related, *restricted_subset);
			}
		}
	}
	return MultiIndexSequence::related_sequence_subset_restriction(related, subset);
}

std::unique_ptr<MultiIndexSequence> CuboidMultiIndexSequence::distinct_projection(
    std::set<size_t> const& dimensions) const
{
	if (auto max_element = std::max_element(dimensions.begin(), dimensions.end());
	    max_element != dimensions.end() && *max_element >= dimensionality()) {
		throw std::invalid_argument(
		    "Given dimensions to projection onto not part of sequence (>= dimensionality).");
	}
	std::vector<size_t> new_m_shape;
	auto const origin = get_origin();
	MultiIndex new_origin;
	for (auto const& dimension : dimensions) {
		new_m_shape.push_back(m_shape.at(dimension));
		new_origin.value.push_back(origin.value.at(dimension));
	}
	DimensionUnits new_dimension_units;
	for (auto const& dimension : dimensions) {
		new_dimension_units.push_back(m_dimension_units.at(dimension));
	}
	return std::make_unique<CuboidMultiIndexSequence>(
	    std::move(new_m_shape), std::move(new_origin), std::move(new_dimension_units));
}

std::unique_ptr<MultiIndexSequence> CuboidMultiIndexSequence::cartesian_product(
    MultiIndexSequence const& other) const
{
	if (size() == 0) {
		return other.copy();
	}
	if (other.size() == 0) {
		return copy();
	}
	if (auto const ptr = dynamic_cast<CuboidMultiIndexSequence const*>(&other); ptr) {
		std::vector<size_t> new_m_shape;
		MultiIndex new_origin;
		auto const origin = get_origin();
		for (size_t i = 0; i < dimensionality(); ++i) {
			new_m_shape.push_back(m_shape.at(i));
			new_origin.value.push_back(origin.value.at(i));
		}
		auto const other_origin = ptr->get_origin();
		for (size_t i = 0; i < other.dimensionality(); ++i) {
			new_m_shape.push_back(ptr->m_shape.at(i));
			new_origin.value.push_back(other_origin.value.at(i));
		}
		DimensionUnits const other_dimension_units = other.get_dimension_units();
		DimensionUnits new_dimension_units = get_dimension_units();
		for (auto const& other_dimension_unit : other_dimension_units) {
			new_dimension_units.push_back(other_dimension_unit);
		}
		if (std::all_of(new_origin.value.begin(), new_origin.value.end(), [](auto const& value) {
			    return value == 0;
		    })) {
			return std::make_unique<CuboidMultiIndexSequence>(
			    std::move(new_m_shape), std::move(new_dimension_units));
		}
		return std::make_unique<CuboidMultiIndexSequence>(
		    std::move(new_m_shape), std::move(new_origin), std::move(new_dimension_units));
	}
	return MultiIndexSequence::cartesian_product(other);
}

std::vector<std::unique_ptr<MultiIndexSequence>> CuboidMultiIndexSequence::slice(
    std::set<size_t> indices) const
{
	if (dimensionality() != 1) {
		return MultiIndexSequence::slice(indices);
	}
	std::vector<std::unique_ptr<MultiIndexSequence>> ret;
	if (indices.empty()) {
		ret.push_back(copy());
		return ret;
	}
	if (*std::max_element(indices.begin(), indices.end()) > size()) {
		throw std::invalid_argument("Given indices out of range of sequence.");
	}
	auto elements = get_elements();
	std::vector<size_t> indices_vector(indices.begin(), indices.end());
	ret.push_back(std::make_unique<CuboidMultiIndexSequence>(
	    std::vector{indices_vector.at(0)}, get_origin(), m_dimension_units));
	for (size_t i = 0; i < indices_vector.size() - 1; ++i) {
		ret.push_back(std::make_unique<CuboidMultiIndexSequence>(
		    std::vector{indices_vector.at(i + 1) - indices_vector.at(i)},
		    MultiIndex({get_origin().value.at(0) + indices_vector.at(i)}), m_dimension_units));
	}
	ret.push_back(std::make_unique<CuboidMultiIndexSequence>(
	    std::vector{size() - indices_vector.at(indices.size() - 1)},
	    MultiIndex({get_origin().value.at(0) + indices_vector.at(indices.size() - 1)}),
	    m_dimension_units));
	return ret;
}

std::unique_ptr<MultiIndexSequence> CuboidMultiIndexSequence::copy() const
{
	return std::make_unique<CuboidMultiIndexSequence>(*this);
}

std::unique_ptr<MultiIndexSequence> CuboidMultiIndexSequence::move()
{
	return std::make_unique<CuboidMultiIndexSequence>(std::move(*this));
}

std::ostream& CuboidMultiIndexSequence::print(std::ostream& os) const
{
	return os << "CuboidMultiIndexSequence(shape: [" << hate::join(m_shape, ", ")
	          << "], origin: " << m_origin << ", dimension units: ["
	          << hate::join(m_dimension_units, ", ", print_dimension_unit) << "])";
}

bool CuboidMultiIndexSequence::is_equal_to(MultiIndexSequence const& other) const
{
	auto const& other_cuboid = static_cast<CuboidMultiIndexSequence const&>(other);
	return m_shape == other_cuboid.m_shape && m_origin == other_cuboid.m_origin &&
	       m_dimension_units == other_cuboid.m_dimension_units;
}

} // namespace grenade::common
