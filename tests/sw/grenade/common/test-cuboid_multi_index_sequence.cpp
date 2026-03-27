#include "grenade/common/multi_index_sequence/cuboid.h"

#include "dapr/empty_property.h"
#include "dapr/property.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence/list.h"
#include <memory>
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>


using namespace grenade::common;

namespace grenade_common_multi_index_sequence_cuboid_tests {

struct DummyDimensionUnit
    : public dapr::EmptyProperty<DummyDimensionUnit, ListMultiIndexSequence::DimensionUnit>
{
private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<MultiIndexSequence::DimensionUnit>(this));
	}
};

} // namespace grenade_common_multi_index_sequence_cuboid_tests

using namespace grenade_common_multi_index_sequence_cuboid_tests;

TEST(CuboidMultiIndexSequence, General)
{
	CuboidMultiIndexSequence empty_sequence({}, MultiIndex());

	EXPECT_EQ(empty_sequence.dimensionality(), 0);
	EXPECT_EQ(empty_sequence.size(), 0);
	EXPECT_TRUE(empty_sequence.get_elements().empty());
	EXPECT_TRUE(empty_sequence.get_dimension_units().empty());
	EXPECT_EQ(empty_sequence.get_origin(), MultiIndex());
	EXPECT_TRUE(empty_sequence.get_shape().empty());

	CuboidMultiIndexSequence dimensionless_sequence({1, 2}, MultiIndex({3, 4}));

	EXPECT_EQ(dimensionless_sequence.dimensionality(), 2);
	EXPECT_EQ(dimensionless_sequence.size(), 2);
	EXPECT_EQ(
	    dimensionless_sequence.get_elements(),
	    (std::vector{MultiIndex({3, 4}), MultiIndex({3, 5})}));
	EXPECT_EQ(dimensionless_sequence.get_dimension_units().size(), 2);
	EXPECT_FALSE(dimensionless_sequence.get_dimension_units().at(0));
	EXPECT_FALSE(dimensionless_sequence.get_dimension_units().at(1));
	EXPECT_EQ(dimensionless_sequence.get_origin(), (MultiIndex({3, 4})));
	EXPECT_EQ(dimensionless_sequence.get_shape(), (std::vector<size_t>{1, 2}));

	dimensionless_sequence.set_origin(MultiIndex({4, 5}));
	// dimensionality mismatch
	EXPECT_THROW(dimensionless_sequence.set_origin(MultiIndex({4})), std::invalid_argument);

	dimensionless_sequence.set_shape({4, 5});
	// dimensionality mismatch
	EXPECT_THROW(dimensionless_sequence.set_shape({4}), std::invalid_argument);

	// dimensionality doesn't match
	EXPECT_THROW(
	    dimensionless_sequence.set_dimension_units({DummyDimensionUnit()}), std::invalid_argument);
	EXPECT_NO_THROW(
	    dimensionless_sequence.set_dimension_units({DummyDimensionUnit(), DummyDimensionUnit()}));

	CuboidMultiIndexSequence sequence(
	    {1, 2}, MultiIndex({3, 4}), {DummyDimensionUnit(), DummyDimensionUnit()});

	auto sequence_copy = sequence.copy();
	assert(sequence_copy);
	EXPECT_EQ(*sequence_copy, sequence);
	auto sequence_move = sequence_copy->move();
	assert(sequence_move);
	EXPECT_EQ(*sequence_move, sequence);
	dynamic_cast<CuboidMultiIndexSequence&>(*sequence_move).set_origin(MultiIndex({5, 6}));
	EXPECT_NE(*sequence_move, sequence);
	dynamic_cast<CuboidMultiIndexSequence&>(*sequence_move).set_origin(MultiIndex({3, 4}));
	dynamic_cast<CuboidMultiIndexSequence&>(*sequence_move).set_shape(std::vector<size_t>{5, 6});
	EXPECT_NE(*sequence_move, sequence);
	dynamic_cast<CuboidMultiIndexSequence&>(*sequence_move).set_shape(std::vector<size_t>{1, 2});
	dynamic_cast<CuboidMultiIndexSequence&>(*sequence_move)
	    .set_dimension_units(CuboidMultiIndexSequence::DimensionUnits(2));
	EXPECT_NE(*sequence_move, sequence);


	EXPECT_TRUE(sequence.is_injective());

	EXPECT_TRUE(sequence.includes(sequence));
	EXPECT_FALSE((sequence.includes(CuboidMultiIndexSequence(
	    {2, 2}, MultiIndex({3, 4}), {DummyDimensionUnit(), DummyDimensionUnit()}))));

	EXPECT_FALSE(sequence.is_disjunct(sequence));
	EXPECT_TRUE((sequence.is_disjunct(CuboidMultiIndexSequence(
	    {1, 2}, MultiIndex({5, 5}), {DummyDimensionUnit(), DummyDimensionUnit()}))));
	// dimension units don't match
	EXPECT_THROW(
	    (sequence.is_disjunct(CuboidMultiIndexSequence({1, 2}, MultiIndex({3, 4})))),
	    std::invalid_argument);

	EXPECT_EQ(*sequence.subset_restriction(sequence), sequence);
	EXPECT_EQ(
	    (*sequence.subset_restriction(CuboidMultiIndexSequence(
	        {6, 4}, MultiIndex({3, 5}), {DummyDimensionUnit(), DummyDimensionUnit()}))),
	    (CuboidMultiIndexSequence(
	        {1, 1}, MultiIndex({3, 5}), {DummyDimensionUnit(), DummyDimensionUnit()})));
	EXPECT_EQ(
	    (*sequence.subset_restriction(ListMultiIndexSequence(
	        {MultiIndex({3, 5}), MultiIndex({10, 10})},
	        {DummyDimensionUnit(), DummyDimensionUnit()}))),
	    (ListMultiIndexSequence(
	        {MultiIndex({3, 5})}, {DummyDimensionUnit(), DummyDimensionUnit()})));
	// dimension units don't match
	EXPECT_THROW(
	    sequence.subset_restriction(CuboidMultiIndexSequence({1, 1}, {MultiIndex({0, 1})})),
	    std::invalid_argument);

	EXPECT_EQ((*sequence.related_sequence_subset_restriction(sequence, sequence)), sequence);
	EXPECT_EQ(
	    (*sequence.related_sequence_subset_restriction(
	        CuboidMultiIndexSequence({1, 2}, MultiIndex({10, 10})),
	        CuboidMultiIndexSequence({1, 1}, MultiIndex({10, 11})))),
	    (CuboidMultiIndexSequence(
	        {1, 1}, MultiIndex({3, 5}), {DummyDimensionUnit(), DummyDimensionUnit()})));
	// related sequence not bijective to sequence
	EXPECT_THROW(
	    (sequence.related_sequence_subset_restriction(
	        CuboidMultiIndexSequence({1, 3}, MultiIndex({5, 2})),
	        CuboidMultiIndexSequence({1, 3}, MultiIndex({5, 2})))),
	    std::invalid_argument);

	EXPECT_EQ(
	    (*CuboidMultiIndexSequence(
	          {1, 2, 3}, MultiIndex({3, 4, 4}),
	          {DummyDimensionUnit(), DummyDimensionUnit(), DummyDimensionUnit()})
	          .distinct_projection({0, 1})),
	    sequence);
	// dimension not present
	EXPECT_THROW(sequence.distinct_projection({4}), std::invalid_argument);

	EXPECT_EQ(
	    (*CuboidMultiIndexSequence(
	          {1, 2}, MultiIndex({0, 1}), {DummyDimensionUnit(), DummyDimensionUnit()})
	          .cartesian_product(CuboidMultiIndexSequence(
	              {3, 4}, MultiIndex({2, 3}), {DummyDimensionUnit(), DummyDimensionUnit()}))),
	    (CuboidMultiIndexSequence(
	        {1, 2, 3, 4}, MultiIndex({0, 1, 2, 3}),
	        {DummyDimensionUnit(), DummyDimensionUnit(), DummyDimensionUnit(),
	         DummyDimensionUnit()})));

	auto slices = CuboidMultiIndexSequence({4}, MultiIndex({0})).slice({0, 2});
	EXPECT_EQ(slices.size(), 3);
	EXPECT_EQ(*slices.at(0), (CuboidMultiIndexSequence({0}, MultiIndex({0}))));
	EXPECT_EQ(*slices.at(1), (CuboidMultiIndexSequence({2}, MultiIndex({0}))));
	EXPECT_EQ(*slices.at(2), (CuboidMultiIndexSequence({2}, MultiIndex({2}))));
}

CEREAL_REGISTER_TYPE(grenade_common_multi_index_sequence_cuboid_tests::DummyDimensionUnit)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::MultiIndexSequence::DimensionUnit,
    grenade_common_multi_index_sequence_cuboid_tests::DummyDimensionUnit)

TEST(CuboidMultiIndexSequence, Cerealization)
{
	CuboidMultiIndexSequence obj(
	    {1, 2}, MultiIndex({3, 4}), {DummyDimensionUnit(), DummyDimensionUnit()});


	CuboidMultiIndexSequence obj2;

	std::ostringstream ostream;
	{
		cereal::JSONOutputArchive oa(ostream);
		oa(obj);
	}

	std::istringstream istream(ostream.str());
	{
		cereal::JSONInputArchive ia(istream);
		ia(obj2);
	}

	ASSERT_EQ(obj2, obj);
}
