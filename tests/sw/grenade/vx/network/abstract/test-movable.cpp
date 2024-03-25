#include "grenade/vx/network/abstract/movable.h"

#include <vector>
#include <gtest/gtest.h>


struct DummyMovable : public grenade::vx::network::Movable<DummyMovable>
{
	std::vector<int> value;

	DummyMovable(int value) : value(10, value) {}

	virtual std::unique_ptr<DummyMovable> move() override
	{
		return std::make_unique<DummyMovable>(std::move(*this));
	}
};


struct DerivedDummyMovable : public DummyMovable
{
	std::vector<int> derived_value;

	DerivedDummyMovable(int value, int derived_value) :
	    DummyMovable(value), derived_value(15, derived_value)
	{}

	virtual std::unique_ptr<DummyMovable> move() override
	{
		return std::make_unique<DerivedDummyMovable>(std::move(*this));
	}
};


TEST(Movable, General)
{
	DummyMovable dummy(5);

	auto dummy_move = dummy.move();
	EXPECT_TRUE(dummy_move);
	EXPECT_EQ(dummy_move->value, std::vector<int>(10, 5));
	EXPECT_TRUE(dummy.value.empty());

	DerivedDummyMovable derived_dummy(15, 8);

	auto derived_dummy_move = derived_dummy.move();
	EXPECT_TRUE(derived_dummy_move);
	EXPECT_EQ(derived_dummy_move->value, std::vector<int>(10, 15));
	EXPECT_EQ(
	    dynamic_cast<DerivedDummyMovable const&>(*derived_dummy_move).value,
	    std::vector<int>(10, 15));
	EXPECT_EQ(
	    dynamic_cast<DerivedDummyMovable const&>(*derived_dummy_move).derived_value,
	    std::vector<int>(15, 8));
	EXPECT_TRUE(derived_dummy.value.empty());
	EXPECT_TRUE(derived_dummy.derived_value.empty());
}
