#include "grenade/vx/network/abstract/copyable.h"
#include <memory>

#include <gtest/gtest.h>


struct DummyCopyable : public grenade::vx::network::Copyable<DummyCopyable>
{
	int value;

	DummyCopyable(int value) : value(value) {}

	virtual std::unique_ptr<DummyCopyable> copy() const override
	{
		return std::make_unique<DummyCopyable>(*this);
	}
};


struct DerivedDummyCopyable : public DummyCopyable
{
	int derived_value;

	DerivedDummyCopyable(int value, int derived_value) :
	    DummyCopyable(value), derived_value(derived_value)
	{}

	virtual std::unique_ptr<DummyCopyable> copy() const override
	{
		return std::make_unique<DerivedDummyCopyable>(*this);
	}
};


TEST(Copyable, General)
{
	DummyCopyable const dummy(5);

	auto const dummy_copy = dummy.copy();
	EXPECT_TRUE(dummy_copy);
	EXPECT_EQ(dummy_copy->value, dummy.value);

	DerivedDummyCopyable const derived_dummy(15, 8);

	auto const derived_dummy_copy = derived_dummy.copy();
	EXPECT_TRUE(derived_dummy_copy);
	EXPECT_EQ(derived_dummy_copy->value, derived_dummy.value);
	EXPECT_EQ(
	    dynamic_cast<DerivedDummyCopyable const&>(*derived_dummy_copy).value, derived_dummy.value);
	EXPECT_EQ(
	    dynamic_cast<DerivedDummyCopyable const&>(*derived_dummy_copy).derived_value,
	    derived_dummy.derived_value);
}
