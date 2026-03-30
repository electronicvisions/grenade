#include "grenade/common/multi_index_sequence/cartesian_product.h"

#include "dapr/empty_property.h"
#include "dapr/property.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include <memory>
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>


using namespace grenade::common;

namespace grenade_common_multi_index_sequence_cartesian_product_tests {

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

} // namespace grenade_common_multi_index_sequence_cartesian_product_tests

using namespace grenade_common_multi_index_sequence_cartesian_product_tests;


TEST(CartesianProductMultiIndexSequence, General)
{
	CartesianProductMultiIndexSequence dimensionless_sequence(
	    ListMultiIndexSequence({MultiIndex({1})}), ListMultiIndexSequence({MultiIndex({2})}));

	EXPECT_EQ(dimensionless_sequence.get_first(), ListMultiIndexSequence({MultiIndex({1})}));
	EXPECT_EQ(dimensionless_sequence.get_second(), ListMultiIndexSequence({MultiIndex({2})}));

	EXPECT_EQ(dimensionless_sequence.dimensionality(), 2);
	EXPECT_EQ(dimensionless_sequence.size(), 1);
	EXPECT_EQ(dimensionless_sequence.get_elements().at(0), (MultiIndex({1, 2})));
	EXPECT_EQ(dimensionless_sequence.get_dimension_units().size(), 2);
	EXPECT_FALSE(dimensionless_sequence.get_dimension_units().at(0));
	EXPECT_FALSE(dimensionless_sequence.get_dimension_units().at(1));

	EXPECT_THROW(dimensionless_sequence.set_dimension_units({}), std::invalid_argument);
	dimensionless_sequence.set_dimension_units({DummyDimensionUnit(), DummyDimensionUnit()});
	EXPECT_EQ(dimensionless_sequence.get_dimension_units().size(), 2);
	EXPECT_EQ(dimensionless_sequence.get_dimension_units().at(0), DummyDimensionUnit());
	EXPECT_EQ(dimensionless_sequence.get_dimension_units().at(1), DummyDimensionUnit());

	CartesianProductMultiIndexSequence sequence(
	    ListMultiIndexSequence({MultiIndex({1})}, {DummyDimensionUnit()}),
	    ListMultiIndexSequence({MultiIndex({2})}, {DummyDimensionUnit()}));

	auto sequence_copy = sequence.copy();
	assert(sequence_copy);
	EXPECT_EQ(*sequence_copy, sequence);
	auto sequence_move = sequence_copy->move();
	assert(sequence_move);
	EXPECT_EQ(*sequence_move, sequence);
	dynamic_cast<CartesianProductMultiIndexSequence&>(*sequence_move)
	    .set_first(ListMultiIndexSequence({MultiIndex({0})}, {DummyDimensionUnit()}));
	EXPECT_NE(*sequence_move, sequence);
	dynamic_cast<CartesianProductMultiIndexSequence&>(*sequence_move)
	    .set_first(ListMultiIndexSequence({MultiIndex({1})}, {DummyDimensionUnit()}));
	dynamic_cast<CartesianProductMultiIndexSequence&>(*sequence_move)
	    .set_second(ListMultiIndexSequence({MultiIndex({0})}, {DummyDimensionUnit()}));
	EXPECT_NE(*sequence_move, sequence);

	EXPECT_TRUE(sequence.is_injective());
	sequence.set_first(
	    ListMultiIndexSequence({MultiIndex({1}), MultiIndex({1})}, {DummyDimensionUnit()}));
	EXPECT_FALSE(sequence.is_injective());
	sequence.set_first(ListMultiIndexSequence({MultiIndex({1})}, {DummyDimensionUnit()}));
	sequence.set_first(
	    ListMultiIndexSequence({MultiIndex({1}), MultiIndex({1})}, {DummyDimensionUnit()}));

	EXPECT_TRUE(sequence.includes(sequence));
	EXPECT_TRUE((sequence.includes(ListMultiIndexSequence(
	    {MultiIndex({1, 2})}, {DummyDimensionUnit(), DummyDimensionUnit()}))));
	sequence.set_first(CuboidMultiIndexSequence({1}, MultiIndex({1}), {DummyDimensionUnit()}));
	sequence.set_second(CuboidMultiIndexSequence({1}, MultiIndex({2}), {DummyDimensionUnit()}));
	EXPECT_TRUE((sequence.includes(CuboidMultiIndexSequence(
	    {1, 1}, MultiIndex({1, 2}), {DummyDimensionUnit(), DummyDimensionUnit()}))));

	EXPECT_EQ(
	    *sequence.distinct_projection({1}),
	    CuboidMultiIndexSequence({1}, MultiIndex({2}), {DummyDimensionUnit()}));
	EXPECT_EQ(*sequence.distinct_projection({}), ListMultiIndexSequence());
	EXPECT_EQ(*sequence.distinct_projection({0, 1}), sequence);
	// dimension not present
	EXPECT_THROW(sequence.distinct_projection({2}), std::invalid_argument);
}

CEREAL_REGISTER_TYPE(
    grenade_common_multi_index_sequence_cartesian_product_tests::DummyDimensionUnit)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::MultiIndexSequence::DimensionUnit,
    grenade_common_multi_index_sequence_cartesian_product_tests::DummyDimensionUnit)

TEST(CartesianProductMultiIndexSequence, Cerealization)
{
	CartesianProductMultiIndexSequence obj(
	    ListMultiIndexSequence({MultiIndex({1})}, {DummyDimensionUnit()}),
	    ListMultiIndexSequence({MultiIndex({2})}, {DummyDimensionUnit()}));

	CartesianProductMultiIndexSequence obj2;

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
