#include "grenade/common/multi_index_sequence/list.h"

#include "dapr/empty_property.h"
#include "dapr/property.h"
#include "grenade/common/multi_index.h"
#include <memory>
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>


using namespace grenade::common;

namespace grenade_common_multi_index_sequence_list_tests {

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

} // namespace grenade_common_multi_index_sequence_list_tests

using namespace grenade_common_multi_index_sequence_list_tests;

TEST(ListMultiIndexSequence, General)
{
	ListMultiIndexSequence empty_sequence;

	EXPECT_EQ(empty_sequence.dimensionality(), 0);
	EXPECT_EQ(empty_sequence.size(), 0);
	EXPECT_TRUE(empty_sequence.get_elements().empty());

	ListMultiIndexSequence dimensionless_sequence({MultiIndex({0}), MultiIndex({1})});

	EXPECT_EQ(dimensionless_sequence.dimensionality(), 1);
	EXPECT_EQ(dimensionless_sequence.size(), 2);
	EXPECT_EQ(
	    dimensionless_sequence.get_elements(), (std::vector{MultiIndex({0}), MultiIndex({1})}));
	EXPECT_EQ(dimensionless_sequence.get_dimension_units().size(), 1);
	EXPECT_FALSE(dimensionless_sequence.get_dimension_units().at(0));

	dimensionless_sequence.set_elements({MultiIndex({1})});
	EXPECT_EQ(dimensionless_sequence.size(), 1);
	// dimensionality doesn't match
	EXPECT_THROW(
	    dimensionless_sequence.set_dimension_units({DummyDimensionUnit(), DummyDimensionUnit()}),
	    std::invalid_argument);
	EXPECT_NO_THROW(dimensionless_sequence.set_dimension_units({DummyDimensionUnit()}));

	ListMultiIndexSequence sequence({MultiIndex({0}), MultiIndex({1})}, {DummyDimensionUnit()});

	EXPECT_EQ(sequence.dimensionality(), 1);
	EXPECT_EQ(sequence.size(), 2);
	EXPECT_EQ(sequence.get_elements(), (std::vector{MultiIndex({0}), MultiIndex({1})}));
	EXPECT_EQ(
	    sequence.get_dimension_units(),
	    ListMultiIndexSequence::DimensionUnits{DummyDimensionUnit()});

	sequence.set_dimension_units({DummyDimensionUnit()});
	// dimensionality doesn't match
	EXPECT_THROW(
	    (sequence.set_dimension_units({DummyDimensionUnit(), DummyDimensionUnit()})),
	    std::invalid_argument);
	EXPECT_THROW(sequence.set_elements({MultiIndex({0, 0})}), std::invalid_argument);
	// heterogeneous dimensionality
	EXPECT_THROW((sequence.set_elements({MultiIndex(), MultiIndex({1})})), std::invalid_argument);

	auto sequence_copy = sequence.copy();
	assert(sequence_copy);
	EXPECT_EQ(*sequence_copy, sequence);
	auto sequence_move = sequence_copy->move();
	assert(sequence_move);
	EXPECT_EQ(*sequence_move, sequence);
	dynamic_cast<ListMultiIndexSequence&>(*sequence_copy).set_dimension_units({});
	EXPECT_NE(*sequence_copy, sequence);
	dynamic_cast<ListMultiIndexSequence&>(*sequence_move)
	    .set_elements({MultiIndex({2}), MultiIndex({3})});
	EXPECT_NE(*sequence_move, sequence);

	EXPECT_TRUE(sequence.is_injective());
	sequence.set_elements({MultiIndex({1}), MultiIndex({1})});
	EXPECT_FALSE(sequence.is_injective());
	sequence.set_elements({MultiIndex({0}), MultiIndex({1})});

	EXPECT_TRUE(sequence.includes(sequence));
	EXPECT_FALSE((sequence.includes(
	    ListMultiIndexSequence({MultiIndex({1}), MultiIndex({2})}, {DummyDimensionUnit()}))));

	EXPECT_FALSE(sequence.is_disjunct(sequence));
	EXPECT_TRUE((sequence.is_disjunct(
	    ListMultiIndexSequence({MultiIndex({2}), MultiIndex({3})}, {DummyDimensionUnit()}))));
	// dimension units don't match
	EXPECT_THROW(
	    (sequence.is_disjunct(ListMultiIndexSequence({MultiIndex({2}), MultiIndex({3})}))),
	    std::invalid_argument);

	EXPECT_EQ(*sequence.subset_restriction(sequence), sequence);
	EXPECT_EQ(
	    (*sequence.subset_restriction(
	        ListMultiIndexSequence({MultiIndex({1}), MultiIndex({2})}, {DummyDimensionUnit()}))),
	    (ListMultiIndexSequence({MultiIndex({1})}, {DummyDimensionUnit()})));
	// dimension units don't match
	EXPECT_THROW(
	    sequence.subset_restriction(ListMultiIndexSequence({MultiIndex({0})})),
	    std::invalid_argument);

	EXPECT_EQ((*sequence.related_sequence_subset_restriction(sequence, sequence)), sequence);
	EXPECT_EQ(
	    (*sequence.related_sequence_subset_restriction(
	        ListMultiIndexSequence({MultiIndex({4, 1}), MultiIndex({5, 2})}),
	        ListMultiIndexSequence({MultiIndex({5, 2})}))),
	    (ListMultiIndexSequence({MultiIndex({1})}, {DummyDimensionUnit()})));
	// related sequence not bijective to sequence
	EXPECT_THROW(
	    (sequence.related_sequence_subset_restriction(
	        ListMultiIndexSequence({MultiIndex({5, 2})}),
	        ListMultiIndexSequence({MultiIndex({5, 2})}))),
	    std::invalid_argument);

	EXPECT_EQ(
	    (*ListMultiIndexSequence(
	          {MultiIndex({4, 0}), MultiIndex({5, 1})},
	          {DummyDimensionUnit(), DummyDimensionUnit()})
	          .projection({1})),
	    sequence);
	// dimension not present
	EXPECT_THROW(
	    (*ListMultiIndexSequence(
	          {MultiIndex({4, 0}), MultiIndex({5, 1})},
	          {DummyDimensionUnit(), DummyDimensionUnit()})
	          .projection({2})),
	    std::invalid_argument);

	EXPECT_EQ(
	    (*ListMultiIndexSequence({MultiIndex({0, 1})})
	          .cartesian_product(ListMultiIndexSequence({MultiIndex({2, 3})}))),
	    (ListMultiIndexSequence({MultiIndex({0, 1, 2, 3})})));

	auto slices =
	    ListMultiIndexSequence({MultiIndex({0}), MultiIndex({1}), MultiIndex({2}), MultiIndex({3})})
	        .slice({0, 2});
	EXPECT_EQ(slices.size(), 3);
	EXPECT_EQ(
	    *slices.at(0), ListMultiIndexSequence({}, ListMultiIndexSequence::DimensionUnits{{}}));
	EXPECT_EQ(*slices.at(1), (ListMultiIndexSequence({MultiIndex({0}), MultiIndex({1})})));
	EXPECT_EQ(*slices.at(2), (ListMultiIndexSequence({MultiIndex({2}), MultiIndex({3})})));
}

CEREAL_REGISTER_TYPE(grenade_common_multi_index_sequence_list_tests::DummyDimensionUnit)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::MultiIndexSequence::DimensionUnit,
    grenade_common_multi_index_sequence_list_tests::DummyDimensionUnit)

TEST(ListMultiIndexSequence, Cerealization)
{
	ListMultiIndexSequence obj({MultiIndex({0}), MultiIndex({1})}, {DummyDimensionUnit()});

	ListMultiIndexSequence obj2;

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
