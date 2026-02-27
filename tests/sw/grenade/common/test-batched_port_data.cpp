#include "grenade/common/port_data/batched.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace grenade_common_batched_port_data_tests {

struct DummyBatchedPortData : BatchedPortData
{
	int value{};

	DummyBatchedPortData() = default;
	DummyBatchedPortData(int value) : value(value) {}

	virtual size_t batch_size() const override
	{
		return value;
	}

	virtual std::unique_ptr<PortData> copy() const override
	{
		return std::make_unique<DummyBatchedPortData>(*this);
	}

	virtual std::unique_ptr<PortData> move() override
	{
		return std::make_unique<DummyBatchedPortData>(std::move(*this));
	}

protected:
	virtual bool is_equal_to(grenade::common::PortData const& other) const override
	{
		return value == static_cast<DummyBatchedPortData const&>(other).value;
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
		ar(cereal::base_class<BatchedPortData>(this));
		ar(value);
	}
};

} // namespace grenade_common_batched_port_data_tests

using namespace grenade_common_batched_port_data_tests;

CEREAL_REGISTER_TYPE(grenade_common_batched_port_data_tests::DummyBatchedPortData)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::common::BatchedPortData, grenade_common_batched_port_data_tests::DummyBatchedPortData)

TEST(BatchedPortData, Cerealization)
{
	DummyBatchedPortData obj{42};

	DummyBatchedPortData obj2;

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
