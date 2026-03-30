#pragma once
#include "dapr/property.h"
#include "dapr/property_holder.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index.h"
#include <array>
#include <cstddef>
#include <functional>
#include <set>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Multi-index sequence within index space.
 * Used to describe shapes or identify signal channels of multi-dimensional data flow.
 *
 * A multi-index sequence contains (possibly non-distinct) multi-indices of homogeneous
 * dimensionality in a user-given order.
 *
 * Additionally, for each dimension of the multi-indices, a unit must be given, for which
 * equality is ensured in operations involving multiple sequences, which are supposed to reside in
 * the same index space.
 *
 * For example, when represending data flow across a two-dimensional collection of channels, one
 * channel can be represented by a multi index like (3, 7) and three channels in a two-dimensional
 * multi-index space can be represented by the sequence [(0, 0), (1, 0), (2, 0)] or also
 * [(3, 7), (5, 1), (14, 5)], when they are identified with different positions in the multi-index
 * space.
 */
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) MultiIndexSequence
    : public dapr::Property<MultiIndexSequence>
{
	/**
	 * Unit of dimension in index space.
	 */
	struct GENPYBIND(inline_base("*")) DimensionUnit : public dapr::Property<DimensionUnit>
	{
		virtual ~DimensionUnit();

	private:
		friend struct cereal::access;
		template <typename Archive>
		void serialize(Archive&, std::uint32_t);
	};

	/**
	 * Unit of dimensions of multi-indices in sequence.
	 * Can either be present for a dimension or empty.
	 */
	typedef std::vector<dapr::PropertyHolder<DimensionUnit>> DimensionUnits;

	MultiIndexSequence() = default;
	virtual ~MultiIndexSequence();

	/**
	 * Get dimension units.
	 */
	virtual DimensionUnits get_dimension_units() const = 0;

	/**
	 * Set dimension units.
	 */
	virtual void set_dimension_units(DimensionUnits value) GENPYBIND(hidden) = 0;

	/**
	 * Get dimensionality of multi-indices in sequence.
	 */
	virtual size_t dimensionality() const = 0;

	/**
	 * Get number of elements in sequence.
	 */
	virtual size_t size() const = 0;

	/**
	 * Get elements in sequence.
	 */
	virtual std::vector<MultiIndex> get_elements() const = 0;

	/**
	 * Get whether all elements of other sequence are present in this sequence.
	 * The dimension units are required to match in addition to set inclusion.
	 */
	virtual bool includes(MultiIndexSequence const& other) const;

	/**
	 * Get whether sequence of elements is injective.
	 * This is the case if all elements are distinct.
	 */
	virtual bool is_injective() const;

	/**
	 * Check whether no element of sequence is present in other sequence.
	 * @throws std::invalid_argument On dimension units not matching
	 */
	virtual bool is_disjunct(MultiIndexSequence const& other) const;

	/**
	 * Get restriction of sequence onto subset of elements present in other sequence.
	 * @throws std::invalid_argument On dimensionality or dimension units not matching
	 */
	virtual std::unique_ptr<MultiIndexSequence> subset_restriction(
	    MultiIndexSequence const& other) const;

	/**
	 * Get restriction of sequence defined by subset restriction of related sequence.
	 * Elements are part of the restricted sequence, if the element at the same index in the related
	 * sequence is part of the subset.
	 * @throws std::invalid_argument On this sequence and related sequence not representing a
	 * bijective mapping due to differing size
	 * @param related Related sequence
	 * @param subset Subset sequence onto which to restrict related sequence
	 */
	virtual std::unique_ptr<MultiIndexSequence> related_sequence_subset_restriction(
	    MultiIndexSequence const& related, MultiIndexSequence const& subset) const;

	/**
	 * Get projection of sequence onto given dimensions.
	 * @param dimensions Dimensions onto which to project
	 * @throws std::invalid_argument On given dimensions not present in this sequence
	 */
	virtual std::unique_ptr<MultiIndexSequence> projection(
	    std::set<size_t> const& dimensions) const;

	/**
	 * Get distinct projection of sequence onto given dimensions.
	 * The resulting sequence consists of distinct elements.
	 * @param dimensions Dimensions onto which to project
	 * @throws std::invalid_argument On given dimensions not present in this sequence
	 */
	virtual std::unique_ptr<MultiIndexSequence> distinct_projection(
	    std::set<size_t> const& dimensions) const;

	/**
	 * Construct sequence which is the cartesian product of this sequence and the other sequence.
	 */
	virtual std::unique_ptr<MultiIndexSequence> cartesian_product(
	    MultiIndexSequence const& other) const;

	/**
	 * Slice sequence linearly at given indices.
	 * The splits then contain sequences of the elements at the positions in the open intervals
	 * [0, indices[0]), [indices[i], indices[i+1]), ..., [indices[N-1], size).
	 */
	virtual std::vector<std::unique_ptr<MultiIndexSequence>> slice(std::set<size_t> indices) const;

protected:
	static std::string print_dimension_unit(DimensionUnits::value_type const& dimension_unit);

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive&, std::uint32_t);
};

} // namespace common
} // namespace grenade
