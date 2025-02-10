#include "grenade/common/resource_estimator.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/common/edge.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;

namespace grenade_common_vertex_tests {

struct DummyResource : public ResourceEstimator::Resource
{
	std::vector<size_t> scalars;

	DummyResource() = default;
	DummyResource(std::vector<size_t> scalars) : scalars(scalars) {}

	virtual Resource& operator+=(Resource const& other) override
	{
		auto const& other_resource = dynamic_cast<DummyResource const&>(other);
		if (other_resource.scalars.size() != scalars.size()) {
			throw std::invalid_argument("Resources not matching.");
		}
		for (size_t i = 0; i < scalars.size(); ++i) {
			scalars.at(i) += other_resource.scalars.at(i);
		}
		return *this;
	}

	virtual Resource& operator*=(size_t factor) override
	{
		for (size_t i = 0; i < scalars.size(); ++i) {
			scalars.at(i) *= factor;
		}
		return *this;
	}

	virtual std::vector<size_t> scalar_values() const override
	{
		return scalars;
	}

	virtual std::unique_ptr<Resource> subsegment(MultiIndexSequence const&) const override
	{
		return copy();
	}

	virtual std::unique_ptr<Resource> copy() const override
	{
		return std::make_unique<DummyResource>(*this);
	}

	virtual std::unique_ptr<Resource> move() override
	{
		return std::make_unique<DummyResource>(std::move(*this));
	}

	virtual bool is_equal_to(Resource const& other) const override
	{
		return scalars == static_cast<DummyResource const&>(other).scalars;
	}

	virtual std::ostream& print(std::ostream& os) const override
	{
		return os;
	}
};


} // namespace grenade_common_vertex_tests

using namespace grenade_common_vertex_tests;

TEST(ResourceEstimator, Resource)
{
	DummyResource resource_1_1({1});
	DummyResource resource_1_2({2});
	DummyResource resource_2_2({1, 2});

	EXPECT_EQ(resource_1_1, resource_1_1);
	EXPECT_NE(resource_1_1, resource_1_2);
	EXPECT_NE(resource_1_1, resource_1_2);

	auto added_resource = resource_1_1 + resource_1_2;
	EXPECT_TRUE(added_resource);
	EXPECT_EQ(added_resource->scalar_values().size(), 1);
	EXPECT_EQ(added_resource->scalar_values().at(0), 3);

	auto scaled_resource = resource_2_2 * 3;
	EXPECT_TRUE(scaled_resource);
	EXPECT_EQ(scaled_resource->scalar_values().size(), 2);
	EXPECT_EQ(scaled_resource->scalar_values().at(0), 3);
	EXPECT_EQ(scaled_resource->scalar_values().at(1), 6);

	EXPECT_THROW(resource_1_1 + resource_2_2, std::invalid_argument);
}
