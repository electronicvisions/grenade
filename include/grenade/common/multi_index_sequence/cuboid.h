#pragma once
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence.h"
#include "hate/visibility.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Multi-index sequence defined by a cuboid in an index space following row-major order.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) CuboidMultiIndexSequence : public MultiIndexSequence
{
	CuboidMultiIndexSequence() = default;

	/**
	 * Construct sequence.
	 * @param shape Shape of cuboid
	 * @param origin Origin of cuboid in the index space
	 * @throws std::invalid_argument On shape or origin not having same dimensionality
	 */
	CuboidMultiIndexSequence(std::vector<size_t> shape, MultiIndex origin);

	/**
	 * Construct sequence.
	 * Constructor present for Python wrapping to work.
	 * @param shape Shape of cuboid
	 * @param origin Origin of cuboid in the index space
	 * @param dimension_units Dimension units of multi indices
	 * @throws std::invalid_argument On shape, origin or dimension units not having same
	 * dimensionality
	 */
	CuboidMultiIndexSequence(
	    std::vector<size_t> shape,
	    MultiIndex origin,
	    std::vector<std::optional<std::reference_wrapper<DimensionUnit const>>> const&
	        dimension_units);

	/**
	 * Construct sequence.
	 * @param shape Shape of cuboid
	 * @param origin Origin of cuboid in the index space
	 * @param dimension_units Dimension units of multi indices
	 * @throws std::invalid_argument On shape, origin or dimension units not having same
	 * dimensionality
	 */
	CuboidMultiIndexSequence(
	    std::vector<size_t> shape, MultiIndex origin, DimensionUnits dimension_units)
	    GENPYBIND(hidden);

	/**
	 * Construct sequence with zero origin.
	 * @param shape Shape of cuboid
	 * @throws std::invalid_argument On shape or origin not having same dimensionality
	 */
	CuboidMultiIndexSequence(std::vector<size_t> shape);

	/**
	 * Construct sequence with zero origin.
	 * Constructor present for Python wrapping to work.
	 * @param shape Shape of cuboid
	 * @param dimension_units Dimension units of multi indices
	 * @throws std::invalid_argument On shape or dimension units not having same
	 * dimensionality
	 */
	CuboidMultiIndexSequence(
	    std::vector<size_t> shape,
	    std::vector<std::optional<std::reference_wrapper<DimensionUnit const>>> const&
	        dimension_units);

	/**
	 * Construct sequence with zero origin.
	 * @param shape Shape of cuboid
	 * @param dimension_units Dimension units of multi indices
	 * @throws std::invalid_argument On shape or dimension units not having same
	 * dimensionality
	 */
	CuboidMultiIndexSequence(std::vector<size_t> shape, DimensionUnits dimension_units)
	    GENPYBIND(hidden);

	/**
	 * Get dimensionality of multi-indices in sequence.
	 */
	virtual size_t dimensionality() const override;

	/**
	 * Get number of elements in sequence.
	 */
	virtual size_t size() const override;

	/**
	 * Get elements in sequence.
	 */
	virtual std::vector<MultiIndex> get_elements() const override;

	/**
	 * Get origin of cuboid in the index space.
	 */
	MultiIndex get_origin() const;

	/**
	 * Set origin of cuboid in the index space.
	 * @param value Value to set
	 * @throws std::invalid_argument On dimensionality of origin not matching sequence.
	 */
	void set_origin(MultiIndex value);

	/**
	 * Get shape of cuboid in the index space.
	 */
	std::vector<size_t> get_shape() const;

	/**
	 * Set shape of cuboid in the index space.
	 * @param value Value to set
	 * @throws std::invalid_argument On dimensionality of shape not matching sequence.
	 */
	void set_shape(std::vector<size_t> value);

	/**
	 * Get dimension units.
	 */
	virtual DimensionUnits get_dimension_units() const override;

	/**
	 * Set dimension units.
	 * @param value Dimension units to set
	 * @throws std::invalid_argument On number of dimension units not matching dimensionality of
	 * elements in sequence
	 */
	virtual void set_dimension_units(DimensionUnits value) override;

	/**
	 * Get whether all elements of sequence are present in other sequence.
	 * The dimension units are required to match in addition to geometric inclusion.
	 */
	virtual bool includes(MultiIndexSequence const& other) const override;

	/**
	 * Get whether sequence of elements is injective.
	 * This is the case if all elements are distinct.
	 * Cuboid sequences are always injective.
	 */
	virtual bool is_injective() const override;

	/**
	 * Get restriction of sequence onto subset of elements present in other sequence.
	 * @throws std::invalid_argument On dimensionality or dimension units not matching
	 */
	virtual std::unique_ptr<MultiIndexSequence> subset_restriction(
	    MultiIndexSequence const& other) const override;

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
	    MultiIndexSequence const& related, MultiIndexSequence const& subset) const override;

	/**
	 * Get distinct projection of sequence onto given dimensions.
	 * @param dimensions Dimensions onto which to project
	 * @throws std::invalid_argument On given dimensions not present in this sequence
	 */
	virtual std::unique_ptr<MultiIndexSequence> distinct_projection(
	    std::set<size_t> const& dimensions) const override;

	/**
	 * Construct sequence which is the cartesian product of this sequence and the other sequence.
	 */
	virtual std::unique_ptr<MultiIndexSequence> cartesian_product(
	    MultiIndexSequence const& other) const override;

	/**
	 * Slice sequence linearly at given indices.
	 * The splits then contain sequences of the elements at the positions in the open intervals
	 * [0, indices[0]), [indices[i], indices[i+1]), ..., [indices[N-1], size).
	 */
	virtual std::vector<std::unique_ptr<MultiIndexSequence>> slice(
	    std::set<size_t> indices) const override;

	virtual std::unique_ptr<MultiIndexSequence> copy() const override;
	virtual std::unique_ptr<MultiIndexSequence> move() override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(MultiIndexSequence const& other) const override;

private:
	std::vector<size_t> m_shape;
	MultiIndex m_origin;
	DimensionUnits m_dimension_units;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive&, std::uint32_t);
};

} // namespace common
} // namespace grenade
