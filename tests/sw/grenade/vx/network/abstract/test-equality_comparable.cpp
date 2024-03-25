#include "grenade/vx/network/abstract/equality_comparable.h"

#include <vector>
#include <gtest/gtest.h>


struct DummyEqualityComparable
    : public grenade::vx::network::EqualityComparable<DummyEqualityComparable>
{
	int value;

	DummyEqualityComparable(int value) : value(value) {}

protected:
	virtual bool is_equal_to(DummyEqualityComparable const& other) const override
	{
		return value == other.value;
	}
};


struct DerivedDummyEqualityComparable : public DummyEqualityComparable
{
	int derived_value;

	DerivedDummyEqualityComparable(int value, int derived_value) :
	    DummyEqualityComparable(value), derived_value(derived_value)
	{}

protected:
	virtual bool is_equal_to(DummyEqualityComparable const& other) const override
	{
		return derived_value ==
		           static_cast<DerivedDummyEqualityComparable const&>(other).derived_value &&
		       DummyEqualityComparable::is_equal_to(other);
	}
};


TEST(EqualityComparable, General)
{
	DummyEqualityComparable dummy_5_1(5);
	DummyEqualityComparable dummy_5_2(5);
	DummyEqualityComparable dummy_7(7);

	EXPECT_TRUE(dummy_5_1 == dummy_5_1);
	EXPECT_FALSE(dummy_5_1 != dummy_5_1);

	EXPECT_TRUE(dummy_5_1 == dummy_5_2);
	EXPECT_FALSE(dummy_5_1 != dummy_5_2);

	EXPECT_FALSE(dummy_5_1 == dummy_7);
	EXPECT_TRUE(dummy_5_1 != dummy_7);

	DerivedDummyEqualityComparable derived_dummy_15_5_1(15, 5);
	DerivedDummyEqualityComparable derived_dummy_15_5_2(15, 5);
	DerivedDummyEqualityComparable derived_dummy_16_5(16, 5);
	DerivedDummyEqualityComparable derived_dummy_16_7(16, 7);

	EXPECT_FALSE(derived_dummy_15_5_1 == dummy_5_1);
	EXPECT_FALSE(derived_dummy_15_5_1 == dummy_5_1);

	EXPECT_TRUE(derived_dummy_15_5_1 == derived_dummy_15_5_1);
	EXPECT_FALSE(derived_dummy_15_5_1 != derived_dummy_15_5_1);

	EXPECT_TRUE(derived_dummy_15_5_1 == derived_dummy_15_5_2);
	EXPECT_FALSE(derived_dummy_15_5_1 != derived_dummy_15_5_2);

	EXPECT_FALSE(derived_dummy_15_5_1 == derived_dummy_16_5);
	EXPECT_TRUE(derived_dummy_15_5_1 != derived_dummy_16_5);

	EXPECT_FALSE(derived_dummy_16_7 == derived_dummy_16_5);
	EXPECT_TRUE(derived_dummy_16_7 != derived_dummy_16_5);
}
