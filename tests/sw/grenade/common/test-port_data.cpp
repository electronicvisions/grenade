#include "grenade/common/port_data.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace grenade_common_port_data_tests {

struct DummyPortData : PortData
{
	int value{};

	DummyPortData() = default;
	DummyPortData(int value) : value(value) {}

	virtual std::unique_ptr<PortData> copy() const override
	{
		return std::make_unique<DummyPortData>(*this);
	}

	virtual std::unique_ptr<PortData> move() override
	{
		return std::make_unique<DummyPortData>(std::move(*this));
	}

protected:
	virtual bool is_equal_to(grenade::common::PortData const& other) const override
	{
		return value == static_cast<DummyPortData const&>(other).value;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<PortData>(this));
		ar(value);
	}
};

} // namespace grenade_common_port_data_tests

using namespace grenade_common_port_data_tests;

CEREAL_REGISTER_TYPE(grenade_common_port_data_tests::DummyPortData)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::PortData, grenade_common_port_data_tests::DummyPortData)

TEST(PortData, Cerealization)
{
	DummyPortData obj{42};

	DummyPortData obj2;

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
