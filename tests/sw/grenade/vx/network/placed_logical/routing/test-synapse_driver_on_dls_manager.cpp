#include <gtest/gtest.h>

#include "grenade/vx/network/placed_logical/routing/synapse_driver_on_dls_manager.h"
#include "halco/common/iter_all.h"
#include "hate/math.h"
#include <iostream>
#include <random>

using namespace grenade::vx::network::placed_logical;
using namespace grenade::vx::network::placed_logical::routing;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(SynapseDriverOnDLSManager_AllocationRequest, valid)
{
	// empty shapes and empty labels
	{
		SynapseDriverOnDLSManager::AllocationRequest request;
		EXPECT_FALSE(request.valid());
	}
	// empty shapes
	{
		SynapseDriverOnDLSManager::AllocationRequest request{
		    {}, {SynapseDriverOnDLSManager::Label()}, {}};
		EXPECT_FALSE(request.valid());
	}
	// empty labels
	{
		SynapseDriverOnDLSManager::AllocationRequest request{{{}}, {}, {}};
		EXPECT_FALSE(request.valid());
	}
	// valid
	{
		SynapseDriverOnDLSManager::AllocationRequest request{
		    {{}}, {SynapseDriverOnDLSManager::Label()}, {}};
		EXPECT_TRUE(request.valid());
	}
}

TEST(SynapseDriverOnDLSManager_AllocationRequest, General)
{
	SynapseDriverOnDLSManager::AllocationRequest request{
	    {{PADIBusOnDLS(Enum(3)), {{0, false}}}},
	    {SynapseDriverOnDLSManager::Label(2)},
	    SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)};

	SynapseDriverOnDLSManager::AllocationRequest request_copy = request;
	EXPECT_EQ(request, request_copy);

	SynapseDriverOnDLSManager::AllocationRequest other_request_0 = request;
	other_request_0.shapes[PADIBusOnDLS(Enum(1))] = {{1, true}};
	EXPECT_NE(other_request_0, request);

	SynapseDriverOnDLSManager::AllocationRequest other_request_1 = request;
	other_request_1.labels.at(0) = SynapseDriverOnDLSManager::Label(3);
	EXPECT_NE(other_request_1, request);

	SynapseDriverOnDLSManager::AllocationRequest other_request_2 = request;
	other_request_2.dependent_label_group = std::nullopt;
	EXPECT_NE(other_request_2, request);
}

TEST(SynapseDriverOnDLSManager, Empty)
{
	SynapseDriverOnDLSManager manager;

	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requests;
	auto const allocations = manager.solve(requests);

	EXPECT_TRUE(allocations);
	EXPECT_TRUE(allocations->empty());
}

void check(
    std::vector<SynapseDriverOnDLSManager::AllocationRequest> const& requests,
    std::optional<std::vector<SynapseDriverOnDLSManager::Allocation>> const& allocations)
{
	EXPECT_TRUE(allocations);
	assert(allocations);
	EXPECT_EQ(allocations->size(), requests.size());
	for (size_t i = 0; i < requests.size(); ++i) {
		auto const label_it = std::find(
		    requests.at(i).labels.begin(), requests.at(i).labels.end(), allocations->at(i).label);
		EXPECT_TRUE(label_it != requests.at(i).labels.end());
	}
}

TEST(SynapseDriverOnDLSManager, EmptyShapes)
{
	SynapseDriverOnDLSManager manager;

	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
	    {}, {SynapseDriverOnPADIBusManager::Label(0b11111)}, {}});
	EXPECT_THROW(manager.solve(requests), std::runtime_error);
}

TEST(SynapseDriverOnDLSManager, EmptyLabels)
{
	SynapseDriverOnDLSManager manager;

	using Shape = SynapseDriverOnPADIBusManager::AllocationRequest::Shape;
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
	    std::map{std::pair{PADIBusOnDLS(Enum(0)), std::vector{Shape{1, false}}}}, {}, {}});
	EXPECT_THROW(manager.solve(requests), std::runtime_error);
}

TEST(SynapseDriverOnDLSManager, EmptyShapesOnPADIBus)
{
	SynapseDriverOnDLSManager manager;

	using Shape = SynapseDriverOnPADIBusManager::AllocationRequest::Shape;
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
	    std::map{
	        std::pair{PADIBusOnDLS(Enum(0)), std::vector<Shape>{}},
	        std::pair{PADIBusOnDLS(Enum(1)), std::vector{Shape{1, false}}}},
	    {SynapseDriverOnPADIBusManager::Label(0b11111)},
	    {}});
	auto const allocations = manager.solve(requests);

	check(requests, allocations);
}

TEST(SynapseDriverOnDLSManager, SingleLabelSingleAllocation)
{
	SynapseDriverOnDLSManager manager;
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requests;

	std::vector<SynapseDriverOnDLSManager::Label> labels{SynapseDriverOnDLSManager::Label()};

	using Shape = SynapseDriverOnPADIBusManager::AllocationRequest::Shape;

	requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
	    std::map{
	        std::pair{PADIBusOnDLS(Enum(0)), std::vector{Shape{12, true}}},
	        std::pair{PADIBusOnDLS(Enum(1)), std::vector{Shape{1, false}}}},
	    {SynapseDriverOnPADIBusManager::Label(0b00000)},
	    {}});
	auto const allocations = manager.solve(requests);

	check(requests, allocations);
}

TEST(SynapseDriverOnDLSManager, MultiLabelMultiAllocation)
{
	SynapseDriverOnDLSManager manager;
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requests;

	std::vector<SynapseDriverOnPADIBusManager::Label> labels;
	for (auto const l : halco::common::iter_all<SynapseDriverOnPADIBusManager::Label>()) {
		labels.push_back(l);
	}

	using Shape = SynapseDriverOnDLSManager::AllocationRequest::Shape;

	requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
	    std::map{
	        std::pair{PADIBusOnDLS(Enum(0)), std::vector{Shape{12, true}}},
	        std::pair{PADIBusOnDLS(Enum(1)), std::vector{Shape{1, false}}}},
	    {SynapseDriverOnPADIBusManager::Label(0b00000)},
	    {}});
	requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
	    std::map{
	        std::pair{PADIBusOnDLS(Enum(0)), std::vector{Shape{0, false}}},
	        std::pair{PADIBusOnDLS(Enum(1)), std::vector{Shape{7, true}}}},
	    labels,
	    {}});
	requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
	    std::map{
	        std::pair{PADIBusOnDLS(Enum(0)), std::vector{Shape{7, true}}},
	        std::pair{PADIBusOnDLS(Enum(1)), std::vector{Shape{0, false}}}},
	    labels,
	    {}});
	auto const allocations = manager.solve(requests);

	check(requests, allocations);
}

TEST(SynapseDriverOnDLSManager, DependentLabelGroup)
{
	SynapseDriverOnDLSManager manager;
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requests;

	std::vector<SynapseDriverOnPADIBusManager::Label> labels_0;
	std::vector<SynapseDriverOnPADIBusManager::Label> labels_1;
	for (auto const l : halco::common::iter_all<SynapseDriverOnPADIBusManager::Label>()) {
		if (l % 2) {
			labels_1.push_back(l);
		} else {
			labels_0.push_back(l);
		}
	}
	assert(labels_0.size() == labels_1.size());

	using Shape = SynapseDriverOnDLSManager::AllocationRequest::Shape;
	using DependentLabelGroup = SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup;

	requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
	    std::map{std::pair{PADIBusOnDLS(Enum(0)), std::vector{Shape{7, false}}}}, labels_0,
	    DependentLabelGroup(0)});
	requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
	    std::map{std::pair{PADIBusOnDLS(Enum(0)), std::vector{Shape{7, false}}}}, labels_1,
	    DependentLabelGroup(0)});
	auto const allocations = manager.solve(requests);

	check(requests, allocations);
	EXPECT_TRUE(allocations->at(0).label + 1 == allocations->at(1).label);
}

TEST(SynapseDriverOnDLSManager, UnavailableSynapseDriver)
{
	std::set<SynapseDriverOnDLS> unavailable_synapse_drivers;
	for (size_t i = 0; i < 17; ++i) {
		unavailable_synapse_drivers.insert(SynapseDriverOnDLS(
		    SynapseDriverOnPADIBusManager::SynapseDriver(i)
		        .toSynapseDriverOnSynapseDriverBlock()[PADIBusOnPADIBusBlock(0)],
		    SynapseDriverBlockOnDLS(0)));
	}
	SynapseDriverOnDLSManager manager(unavailable_synapse_drivers);
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requests;

	std::vector<SynapseDriverOnDLSManager::Label> labels;
	using Shape = SynapseDriverOnDLSManager::AllocationRequest::Shape;
	for (auto const l : halco::common::iter_all<SynapseDriverOnPADIBusManager::Label>()) {
		labels.push_back(l);
	}
	for (auto const synapse_driver : iter_all<SynapseDriverOnPADIBusManager::SynapseDriver>()) {
		if (unavailable_synapse_drivers.contains(SynapseDriverOnDLS(
		        synapse_driver.toSynapseDriverOnSynapseDriverBlock()[PADIBusOnPADIBusBlock(0)],
		        SynapseDriverBlockOnDLS(0)))) {
			continue;
		}
		requests.push_back(SynapseDriverOnDLSManager::AllocationRequest{
		    std::map{std::pair{PADIBusOnDLS(Enum(0)), std::vector{Shape{1, true}}}},
		    {SynapseDriverOnPADIBusManager::Label(synapse_driver.value())},
		    {}});
	}

	auto const allocations = manager.solve(requests);

	check(requests, allocations);

	for (size_t i = 0; i < requests.size(); ++i) {
		for (size_t j = 0; j < requests.at(i).shapes[PADIBusOnDLS(Enum(0))].size(); ++j) {
			for (auto const& [s, _] : allocations->at(i)
			                              .synapse_drivers.at(PADIBusOnDLS(Enum(0)))
			                              .synapse_drivers.at(j)) {
				EXPECT_FALSE(unavailable_synapse_drivers.contains(SynapseDriverOnDLS(
				    s.toSynapseDriverOnSynapseDriverBlock()[PADIBusOnPADIBusBlock(0)],
				    SynapseDriverBlockOnDLS(0))));
			}
		}
	}
}


struct RandomTestGenerator
{
	using AllocationRequest = SynapseDriverOnDLSManager::AllocationRequest;
	using Shape = SynapseDriverOnDLSManager::AllocationRequest::Shape;

	RandomTestGenerator(uintmax_t seed) : m_seed(seed), m_rng(seed) {}

	std::pair<SynapseDriverOnDLSManager, std::vector<AllocationRequest>> operator()()
	{
		auto const unavailable_synapse_driver = get_unavailable_synapse_driver();
		auto const manager = SynapseDriverOnDLSManager(unavailable_synapse_driver);

		auto const allocation_requests = get_allocation_requests(unavailable_synapse_driver);

		return {manager, allocation_requests};
	}

	std::set<SynapseDriverOnDLS> get_unavailable_synapse_driver()
	{
		std::bernoulli_distribution d_unavailable(std::uniform_real_distribution<double>{}(m_rng));

		std::set<SynapseDriverOnDLS> unavailable_synapse_driver;
		for (auto const synapse_driver : iter_all<SynapseDriverOnDLS>()) {
			if (d_unavailable(m_rng)) {
				unavailable_synapse_driver.insert(synapse_driver);
			}
		}
		return unavailable_synapse_driver;
	}

	size_t get_num_allocation_requests(size_t num_shapes)
	{
		std::uniform_int_distribution<size_t> d(0, std::min(num_shapes, static_cast<size_t>(10)));
		return d(m_rng);
	}

	std::vector<std::pair<PADIBusOnDLS, Shape>> get_shapes(
	    std::set<SynapseDriverOnDLS> const& unavailable_synapse_driver)
	{
		std::bernoulli_distribution d_contiguous(std::uniform_real_distribution<double>{}(m_rng));
		std::vector<std::pair<PADIBusOnDLS, Shape>> ret;
		for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
			size_t num_unavailable_synapse_driver = 0;
			for (auto const& synapse_driver : unavailable_synapse_driver) {
				if (PADIBusOnDLS(
				        synapse_driver.toSynapseDriverOnSynapseDriverBlock()
				            .toPADIBusOnPADIBusBlock(),
				        synapse_driver.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS()) ==
				    padi_bus) {
					num_unavailable_synapse_driver++;
				}
			}
			size_t num_available_synapse_driver =
			    SynapseDriverOnPADIBus::size - num_unavailable_synapse_driver;
			std::uniform_int_distribution<size_t> d(0, num_available_synapse_driver);
			size_t num_shapes = d(m_rng);
			if (num_shapes == 0) {
				continue;
			}
			for (size_t shape = 0; shape < num_shapes - 1; ++shape) {
				std::uniform_int_distribution<size_t> d(
				    0, num_available_synapse_driver - (num_shapes - shape));
				auto const size = d(m_rng) + 1;
				num_available_synapse_driver -= size;
				ret.push_back({padi_bus, Shape{size, d_contiguous(m_rng)}});
			}
			ret.push_back({padi_bus, Shape{num_available_synapse_driver, d_contiguous(m_rng)}});
		}
		std::shuffle(ret.begin(), ret.end(), m_rng);
		return ret;
	}

	std::vector<AllocationRequest> get_allocation_requests(
	    std::set<SynapseDriverOnDLS> const& unavailable_synapse_driver)
	{
		auto shapes = get_shapes(unavailable_synapse_driver);

		auto const num_allocation_requests = get_num_allocation_requests(shapes.size());
		assert(num_allocation_requests <= shapes.size());
		std::vector<AllocationRequest> allocation_requests(num_allocation_requests);

		if (allocation_requests.empty()) {
			return allocation_requests;
		}

		size_t num_unassigned_shapes = shapes.size();
		for (size_t request = 0; request < num_allocation_requests - 1; ++request) {
			std::uniform_int_distribution<size_t> d(
			    0, num_unassigned_shapes - (num_allocation_requests - request));
			auto const size = d(m_rng) + 1;
			num_unassigned_shapes -= size;
			for (size_t i = 0; i < size; ++i) {
				assert(!shapes.empty());
				std::uniform_int_distribution<size_t> d_shape(0, shapes.size() - 1);
				auto const shape_index = d_shape(m_rng);
				auto const shape = shapes.at(shape_index);
				shapes.erase(shapes.begin() + shape_index);
				allocation_requests.at(request).shapes[shape.first].push_back(shape.second);
			}
		}
		for (auto const& shape : shapes) {
			allocation_requests.back().shapes[shape.first].push_back(shape.second);
		}
		std::shuffle(allocation_requests.begin(), allocation_requests.end(), m_rng);

		std::uniform_int_distribution<size_t> d_num_dependent_label_groups(
		    1, allocation_requests.size());
		auto const num_dependent_label_groups = d_num_dependent_label_groups(m_rng);
		std::uniform_int_distribution<size_t> d_dependent_label_group(
		    0, num_dependent_label_groups - 1);

		std::bernoulli_distribution d_has_dependent_label_group(
		    std::uniform_real_distribution<double>{}(m_rng));

		for (auto& allocation_request : allocation_requests) {
			if (d_has_dependent_label_group(m_rng)) {
				allocation_request.dependent_label_group.emplace(
				    AllocationRequest::DependentLabelGroup(d_dependent_label_group(m_rng)));
			}
		}

		// to explore different orders of magnitude, we sample approx. logarithmically
		// reasonable high maximal value, exact value doesn't matter, since labels are drawn
		// randomly
		std::uniform_int_distribution<size_t> d_num_labels_exponent(0, 1);
		std::map<AllocationRequest::DependentLabelGroup, size_t> num_labels;
		for (size_t i = 0; i < num_dependent_label_groups; ++i) {
			auto const num_labels_exponent = d_num_labels_exponent(m_rng);
			std::uniform_int_distribution<size_t> d_num_labels_fine(
			    0, hate::math::pow(2, num_labels_exponent) - 1);
			auto const num = hate::math::pow(2, num_labels_exponent) + d_num_labels_fine(m_rng);
			assert(num < 32);
			num_labels[AllocationRequest::DependentLabelGroup(i)] = num;
		}

		std::uniform_int_distribution<size_t> d_label(
		    SynapseDriverOnDLSManager::Label::min, SynapseDriverOnDLSManager::Label::max);
		for (auto& allocation_request : allocation_requests) {
			size_t num = 0;
			if (allocation_request.dependent_label_group) {
				num = num_labels.at(*allocation_request.dependent_label_group);
			} else {
				auto const num_labels_exponent = d_num_labels_exponent(m_rng);
				std::uniform_int_distribution<size_t> d_num_labels_fine(
				    0, hate::math::pow(2, num_labels_exponent) - 1);
				num = hate::math::pow(2, num_labels_exponent) + d_num_labels_fine(m_rng);
				assert(num < 32);
			}
			assert(num);
			for (size_t i = 0; i < num; ++i) {
				allocation_request.labels.push_back(
				    SynapseDriverOnDLSManager::Label(d_label(m_rng)));
			}
		}

		return allocation_requests;
	}

private:
	uintmax_t m_seed;
	std::mt19937 m_rng;
};


/**
 * Tests parameterized over the allocation policy.
 * If you add a new one, add it to the test case instantiation at the bottom of this file.
 */
struct SynapseDriverOnDLSManagerTestsFixture
    : public testing::TestWithParam<SynapseDriverOnDLSManager::AllocationPolicy>
{};

TEST_P(SynapseDriverOnDLSManagerTestsFixture, random)
{
	constexpr size_t num = 100;

	for (size_t i = 0; i < num; ++i) {
		auto const seed = std::random_device{}();
		RandomTestGenerator generator(seed);
		auto [manager, allocation_requests] = generator();
		EXPECT_NO_THROW(
		    manager.solve(allocation_requests, GetParam(), std::chrono::milliseconds(100)))
		    << "Seed: " << seed;
	}
}

INSTANTIATE_TEST_SUITE_P(
    SynapseDriverOnDLSManager,
    SynapseDriverOnDLSManagerTestsFixture,
    testing::Values(
        SynapseDriverOnDLSManager::AllocationPolicyGreedy(false),
        SynapseDriverOnDLSManager::AllocationPolicyGreedy(true),
        SynapseDriverOnDLSManager::AllocationPolicyBacktracking(std::chrono::milliseconds(100))));
