#include <gtest/gtest.h>

#include "grenade/vx/transformation/concatenation.h"

#include "grenade/vx/connection_type.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "haldls/vx/v3/event.h"

using namespace grenade::vx;
using namespace grenade::vx::transformation;

TEST(Concatenation, General)
{
	Concatenation concatenation(ConnectionType::UInt32, {12, 16, 5});

	EXPECT_EQ(concatenation.output().type, ConnectionType::UInt32);
	EXPECT_EQ(concatenation.output().size, 12 + 16 + 5);
	EXPECT_EQ(concatenation.inputs().size(), 3);
	EXPECT_EQ(concatenation.inputs().at(0).type, ConnectionType::UInt32);
	EXPECT_EQ(concatenation.inputs().at(1).type, ConnectionType::UInt32);
	EXPECT_EQ(concatenation.inputs().at(2).type, ConnectionType::UInt32);
	EXPECT_EQ(concatenation.inputs().at(0).size, 12);
	EXPECT_EQ(concatenation.inputs().at(1).size, 16);
	EXPECT_EQ(concatenation.inputs().at(2).size, 5);

	Concatenation concatenation_copy = concatenation;
	EXPECT_TRUE(concatenation_copy.equal(concatenation));
	EXPECT_TRUE(concatenation.equal(concatenation_copy));

	{
		Concatenation concatenation_other(ConnectionType::Int8, {12, 16, 5});
		EXPECT_FALSE(concatenation_other.equal(concatenation));
		EXPECT_FALSE(concatenation.equal(concatenation_other));
	}
	{
		Concatenation concatenation_other(ConnectionType::UInt32, {10, 16, 5});
		EXPECT_FALSE(concatenation_other.equal(concatenation));
		EXPECT_FALSE(concatenation.equal(concatenation_other));
	}

	constexpr size_t batch_size = 2;
	std::vector<Concatenation::Value> value(3);
	{
		std::vector<TimedDataSequence<std::vector<UInt32>>> values(batch_size);
		size_t i = 0;
		for (auto& v : values) {
			v.resize(1);
			v.at(0).data.resize(12, UInt32(123 + i));
			i++;
		}
		value.at(0) = values;
	}
	{
		std::vector<TimedDataSequence<std::vector<UInt32>>> values(batch_size);
		size_t i = 0;
		for (auto& v : values) {
			v.resize(1);
			v.at(0).data.resize(16, UInt32(456 + i));
			i++;
		}
		value.at(1) = values;
	}
	{
		std::vector<TimedDataSequence<std::vector<UInt32>>> values(batch_size);
		size_t i = 0;
		for (auto& v : values) {
			v.resize(1);
			v.at(0).data.resize(5, UInt32(789 + i));
			i++;
		}
		value.at(2) = values;
	}

	Concatenation::Value expectation;
	{
		std::vector<TimedDataSequence<std::vector<UInt32>>> values(batch_size);
		size_t i = 0;
		for (auto& v : values) {
			v.resize(1);
			v.at(0).data.insert(v.at(0).data.end(), 12, UInt32(123 + i));
			v.at(0).data.insert(v.at(0).data.end(), 16, UInt32(456 + i));
			v.at(0).data.insert(v.at(0).data.end(), 5, UInt32(789 + i));
			i++;
		}
		expectation = values;
	}
	EXPECT_EQ(concatenation.apply(value), expectation);
}
