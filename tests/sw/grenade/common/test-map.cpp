#include "grenade/common/map.h"

#include "grenade/common/property.h"
#include <memory>
#include <gtest/gtest.h>


struct DummyProperty : public grenade::common::Property<DummyProperty>
{};


struct DerivedDummyProperty : public DummyProperty
{
	int value;

	DerivedDummyProperty(int value) : value(value) {}

	virtual std::unique_ptr<DummyProperty> copy() const override
	{
		return std::make_unique<DerivedDummyProperty>(*this);
	}

	virtual std::unique_ptr<DummyProperty> move() override
	{
		return std::make_unique<DerivedDummyProperty>(std::move(*this));
	}

	virtual bool is_equal_to(DummyProperty const& other) const override
	{
		return value == static_cast<DerivedDummyProperty const&>(other).value;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os << "DerivedDummyProperty(" << value << ")";
	}
};


TEST(Map, General)
{
	DerivedDummyProperty const dummy(5);

	grenade::common::Map<int, DummyProperty> map;

	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.size(), 0);
	EXPECT_FALSE(map.contains(0));

	map.add(0, dummy);

	EXPECT_FALSE(map.empty());
	EXPECT_EQ(map.size(), 1);
	EXPECT_TRUE(map.contains(0));
	EXPECT_EQ(map.get(0), dummy);

	auto dummy_to_move = dummy;
	map.add(1, std::move(dummy_to_move));

	EXPECT_EQ(map.size(), 2);
	EXPECT_TRUE(map.contains(1));
	EXPECT_EQ(map.get(1), dummy);

	auto dummy_copy = dummy;
	dummy_copy.value = 7;
	map.set(0, dummy_copy);

	EXPECT_EQ(map.get(0), dummy_copy);

	dummy_copy.value = 9;
	map.set(1, std::move(dummy_copy));

	EXPECT_EQ(dynamic_cast<DerivedDummyProperty const&>(map.get(1)).value, 9);

	std::map<int, DerivedDummyProperty> plain_map_copy;
	for (auto const& [key, value] : map) {
		plain_map_copy.emplace(key, dynamic_cast<DerivedDummyProperty const&>(value));
	}
	EXPECT_TRUE(plain_map_copy.contains(0));
	EXPECT_TRUE(plain_map_copy.contains(1));
	EXPECT_EQ(plain_map_copy.at(0).value, 7);
	EXPECT_EQ(plain_map_copy.at(1).value, 9);

	map.erase(1);
	EXPECT_FALSE(map.contains(1));
	EXPECT_EQ(map.size(), 1);

	auto map_copy = map;
	EXPECT_EQ(map, map_copy);

	map_copy.set(0, dummy);
	EXPECT_NE(map, map_copy);

	EXPECT_THROW(map.get(3), std::out_of_range);
	EXPECT_THROW(map.set(3, dummy), std::out_of_range);
}
