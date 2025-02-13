#include "grenade/common/detail/property_holder.h"

#include "grenade/common/empty_property.h"
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

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os << "DerivedDummyProperty(" << value << ")";
	}

protected:
	virtual bool is_equal_to(DummyProperty const& other) const override
	{
		return value == static_cast<DerivedDummyProperty const&>(other).value;
	}
};


TEST(PropertyHolder, General)
{
	DerivedDummyProperty derived_dummy_5{5};
	grenade::common::detail::PropertyHolder<DummyProperty> holder_5(derived_dummy_5);

	EXPECT_TRUE(holder_5);
	EXPECT_NO_THROW(*holder_5);
	EXPECT_NO_THROW(holder_5.operator->());

	std::stringstream ss;
	ss << holder_5;
	EXPECT_EQ(ss.str(), "DerivedDummyProperty(5)");

	EXPECT_EQ(holder_5, holder_5);

	auto const holder_5_move = std::move(holder_5);

	EXPECT_NE(holder_5, holder_5_move);
	EXPECT_FALSE(holder_5);
	EXPECT_THROW(*holder_5, std::runtime_error);
	EXPECT_THROW(holder_5.operator->(), std::runtime_error);
	EXPECT_EQ(holder_5, holder_5);

	DerivedDummyProperty derived_dummy_7{7};
	grenade::common::detail::PropertyHolder<DummyProperty> holder_7(std::move(derived_dummy_7));

	auto const holder_7_copy = holder_7;

	EXPECT_NE(holder_5_move, holder_7);
	EXPECT_EQ(holder_7_copy, holder_7);
}
