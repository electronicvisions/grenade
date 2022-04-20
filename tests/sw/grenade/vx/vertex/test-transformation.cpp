#include <gtest/gtest.h>

#include "grenade/vx/port.h"
#include "grenade/vx/vertex/transformation.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

/**
 * Transformation for testing creating a matrix of zero values with the same size as the input.
 */
struct ZerosLike : public vertex::Transformation::Function
{
	ZerosLike() = default;

	ZerosLike(size_t size) : m_size(size) {}

	std::vector<Port> inputs() const
	{
		return {Port(m_size, ConnectionType::Int8)};
	}
	Port output() const
	{
		return inputs().at(0);
	}

	bool equal(vertex::Transformation::Function const& other) const
	{
		ZerosLike const* o = dynamic_cast<ZerosLike const*>(&other);
		if (o == nullptr) {
			return false;
		}
		return m_size == o->m_size;
	}

	Value apply(std::vector<Value> const& value) const
	{
		assert(value.size() == 1);
		auto const& input =
		    std::get<std::vector<TimedDataSequence<std::vector<Int8>>>>(value.at(0));
		std::vector<TimedDataSequence<std::vector<Int8>>> ret(input.size());
		size_t i = 0;
		for (auto& r : ret) {
			r.resize(input.at(i).size());
			for (auto& e : r) {
				e.data.resize(m_size);
			}
			i++;
		}
		return ret;
	}

private:
	size_t m_size{};
};


TEST(Transformation, General)
{
	ZerosLike function_copy(123);
	auto function = std::make_unique<ZerosLike>(function_copy);

	Transformation transformation(std::move(function));

	EXPECT_EQ(transformation.inputs(), function_copy.inputs());
	EXPECT_EQ(transformation.output(), function_copy.output());

	{
		auto other_function = std::make_unique<ZerosLike>(function_copy);
		Transformation other_transformation(std::move(other_function));
		EXPECT_EQ(transformation, other_transformation);
	}
	{
		auto other_function = std::make_unique<ZerosLike>(321);
		Transformation other_transformation(std::move(other_function));
		EXPECT_NE(transformation, other_transformation);
	}

	std::vector<Int8> value(123, Int8(42));
	EXPECT_EQ(
	    transformation.apply({std::vector<TimedDataSequence<std::vector<Int8>>>{
	        {{haldls::vx::v3::FPGATime(), haldls::vx::v3::ChipTime(), value}}}}),
	    function_copy.apply({std::vector<TimedDataSequence<std::vector<Int8>>>{
	        {{haldls::vx::v3::FPGATime(), haldls::vx::v3::ChipTime(), value}}}}));
}
