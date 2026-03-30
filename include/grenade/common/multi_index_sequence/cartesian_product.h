#pragma once
#include "dapr/property_holder.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence.h"
#include "hate/visibility.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Multi-index sequence described by cartesian product of two multi-index sequences.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) CartesianProductMultiIndexSequence
    : public MultiIndexSequence
{
	CartesianProductMultiIndexSequence() = default;

	/**
	 * Construct sequence.
	 * @param first First sequence
	 * @param second Second sequence
	 * @throws std::invalid_argument On differing emptiness of sequences
	 */
	CartesianProductMultiIndexSequence(
	    MultiIndexSequence const& first, MultiIndexSequence const& second);

	/**
	 * Construct sequence.
	 * @param first First sequence
	 * @param second Second sequence
	 * @throws std::invalid_argument On differing emptiness of sequences
	 */
	CartesianProductMultiIndexSequence(MultiIndexSequence&& first, MultiIndexSequence&& second)
	    GENPYBIND(hidden);

	/**
	 * Get first sequence.
	 */
	GENPYBIND(return_value_policy(reference_internal))
	MultiIndexSequence const& get_first() const;

	/**
	 * Set first sequence.
	 * @throws std::invalid_argument On differing optionality of dimension units in sequences
	 */
	void set_first(MultiIndexSequence const& value);

	/**
	 * Set first sequence.
	 * @throws std::invalid_argument On differing optionality of dimension units in sequences
	 */
	void set_first(MultiIndexSequence&& value) GENPYBIND(hidden);

	/**
	 * Get second sequence.
	 */
	GENPYBIND(return_value_policy(reference_internal))
	MultiIndexSequence const& get_second() const;

	/**
	 * Set second sequence.
	 * @throws std::invalid_argument On differing optionality of dimension units in sequences
	 */
	void set_second(MultiIndexSequence const& value);

	/**
	 * Set second sequence.
	 * @throws std::invalid_argument On differing optionality of dimension units in sequences
	 */
	void set_second(MultiIndexSequence&& value) GENPYBIND(hidden);

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
	 * Get whether all elements of sequence are present in other sequence.
	 * The dimension units are required to match in addition to geometric inclusion.
	 */
	virtual bool includes(MultiIndexSequence const& other) const override;

	/**
	 * Get whether sequence of elements is injective.
	 * This is the case if all elements are distinct.
	 * For the cartesian product this is the case exactly if both the first and second sequence are
	 * injective.
	 */
	virtual bool is_injective() const override;

	/**
	 * Get distinct projection of sequence onto given dimensions.
	 * @param dimensions Dimensions onto which to project
	 * @throws std::invalid_argument On given dimensions not present in this sequence
	 */
	virtual std::unique_ptr<MultiIndexSequence> distinct_projection(
	    std::set<size_t> const& dimensions) const override;

	virtual std::unique_ptr<MultiIndexSequence> copy() const override;
	virtual std::unique_ptr<MultiIndexSequence> move() override;

protected:
	virtual bool is_equal_to(MultiIndexSequence const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	dapr::PropertyHolder<MultiIndexSequence> m_first;
	dapr::PropertyHolder<MultiIndexSequence> m_second;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive&, std::uint32_t);
};

} // namespace common
} // namespace grenade
