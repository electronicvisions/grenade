#include <gtest/gtest.h>

#include "grenade/vx/network/synapse_driver_on_padi_bus_manager.h"
#include "halco/common/iter_all.h"
#include <random>


using namespace grenade::vx::network;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;


TEST(SynapseDriverOnPADIBusManager_AllocationRequest_Shape, General)
{
	using Shape = SynapseDriverOnPADIBusManager::AllocationRequest::Shape;

	Shape shape{17, true};

	Shape shape_copy = shape;
	EXPECT_EQ(shape, shape_copy);

	Shape other_shape_0{12, true};
	EXPECT_NE(other_shape_0, shape);

	Shape other_shape_1{17, false};
	EXPECT_NE(other_shape_1, shape);
}

TEST(SynapseDriverOnPADIBusManager_AllocationRequest, General)
{
	using Label = SynapseDriverOnPADIBusManager::Label;
	using Shape = SynapseDriverOnPADIBusManager::AllocationRequest::Shape;
	using AllocationRequest = SynapseDriverOnPADIBusManager::AllocationRequest;

	{
		AllocationRequest request{{Shape{17, true}, Shape{3, false}}, Label()};
		EXPECT_EQ(request.size(), 20);
		EXPECT_TRUE(request.is_sensitive_for_shape_allocation_order());
	}

	{
		AllocationRequest request{{Shape{17, false}, Shape{3, false}}, Label()};
		EXPECT_FALSE(request.is_sensitive_for_shape_allocation_order());
	}

	{
		AllocationRequest request{{Shape{3, true}, Shape{3, true}, Shape{3, true}}, Label()};
		EXPECT_FALSE(request.is_sensitive_for_shape_allocation_order());
	}

	AllocationRequest request{{Shape{17, true}}, Label(12)};

	AllocationRequest request_copy = request;
	EXPECT_EQ(request, request_copy);

	AllocationRequest other_request_0{{Shape{17, true}}, Label(10)};
	EXPECT_NE(other_request_0, request);

	AllocationRequest other_request_1{{Shape{16, true}}, Label(12)};
	EXPECT_NE(other_request_1, request);
}


/**
 * Tests parameterized over the allocation policy.
 * If you add a new one, add it to the test case instantiation at the bottom of this file.
 */
struct SynapseDriverOnPADIBusManagerTestsFixture
    : public testing::TestWithParam<SynapseDriverOnPADIBusManager::AllocationPolicy>
{};

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, Empty)
{
	SynapseDriverOnPADIBusManager manager;
	auto const allocations = manager.solve({}, GetParam());
	EXPECT_TRUE(allocations);
	EXPECT_TRUE(allocations->empty());
}

void check(
    std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> const& requests,
    std::optional<std::vector<SynapseDriverOnPADIBusManager::Allocation>> const& allocations)
{
	assert(allocations);
	EXPECT_EQ(allocations->size(), requests.size());
	size_t global_size = 0;
	std::set<SynapseDriverOnPADIBusManager::SynapseDriver> global_unique;
	for (size_t i = 0; i < requests.size(); ++i) {
		EXPECT_EQ(allocations->at(i).synapse_drivers.size(), requests.at(i).shapes.size());
		for (size_t j = 0; j < requests.at(i).shapes.size(); ++j) {
			EXPECT_EQ(
			    allocations->at(i).synapse_drivers.at(j).size(), requests.at(i).shapes.at(j).size);

			std::set<SynapseDriverOnPADIBusManager::SynapseDriver> unique;
			for (auto const& [s, _] : allocations->at(i).synapse_drivers.at(j)) {
				unique.insert(s);
				global_unique.insert(s);
				global_size++;
			}
			EXPECT_EQ(unique.size(), allocations->at(i).synapse_drivers.at(j).size());
		}
	}
	EXPECT_EQ(global_unique.size(), global_size);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, EmptyShapes)
{
	SynapseDriverOnPADIBusManager manager;

	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {}, SynapseDriverOnPADIBusManager::Label(0b01100)});

	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, EmptyShapesAndOtherAllocation)
{
	SynapseDriverOnPADIBusManager manager;

	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {}, SynapseDriverOnPADIBusManager::Label(0b01100)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{12, false}}, SynapseDriverOnPADIBusManager::Label(0b00011)});

	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, EmptyShapesAndOtherAllocationSameLabel)
{
	SynapseDriverOnPADIBusManager manager;

	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {}, SynapseDriverOnPADIBusManager::Label(0b01100)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{12, false}}, SynapseDriverOnPADIBusManager::Label(0b01100)});

	auto const allocations = manager.solve(requests, GetParam());

	EXPECT_FALSE(allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, SameLabel)
{
	SynapseDriverOnPADIBusManager manager;

	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{3, true}}, SynapseDriverOnPADIBusManager::Label(0b01100)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{12, false}}, SynapseDriverOnPADIBusManager::Label(0b01100)});

	auto const allocations = manager.solve(requests, GetParam());

	EXPECT_FALSE(allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, SingleGroupSingleAllocation)
{
	SynapseDriverOnPADIBusManager manager;

	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{12, false}}, SynapseDriverOnPADIBusManager::Label(0b01100)});

	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, SingleGroupMultipleAllocations)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	bool const contiguous =
	    std::holds_alternative<SynapseDriverOnPADIBusManager::AllocationPolicyBacktracking>(
	        GetParam());
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{12, contiguous}}, SynapseDriverOnPADIBusManager::Label(0b01100)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{7, false}}, SynapseDriverOnPADIBusManager::Label(0b10000)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{7, false}}, SynapseDriverOnPADIBusManager::Label(0b00010)});
	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, MultipleGroupsSingleAllocation)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{12, true}, {2, false}}, SynapseDriverOnPADIBusManager::Label(0b01100)});
	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, MultipleGroupMultipleAllocations)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	bool const contiguous =
	    std::holds_alternative<SynapseDriverOnPADIBusManager::AllocationPolicyBacktracking>(
	        GetParam());
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{8, contiguous}, {2, false}, {2, false}}, SynapseDriverOnPADIBusManager::Label(0b01100)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{5, false}, {2, false}}, SynapseDriverOnPADIBusManager::Label(0b10000)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{4, false}, {3, false}}, SynapseDriverOnPADIBusManager::Label(0b00010)});
	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, MaxSizeAllocation)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{SynapseDriverOnPADIBusManager::SynapseDriver::size, false}},
	    SynapseDriverOnPADIBusManager::Label(0b01100)});
	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, MaxSizeAllocationContiguous)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{SynapseDriverOnPADIBusManager::SynapseDriver::size, true}},
	    SynapseDriverOnPADIBusManager::Label(0b01100)});
	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, MaxNumGroupsAllocation)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	SynapseDriverOnPADIBusManager::AllocationRequest request{
	    {}, SynapseDriverOnPADIBusManager::Label(0b01100)};
	for ([[maybe_unused]] auto const synapse_driver :
	     iter_all<SynapseDriverOnPADIBusManager::SynapseDriver>()) {
		request.shapes.push_back({1, false});
	}
	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, MaxNumAllocations)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	for (auto const synapse_driver : iter_all<SynapseDriverOnPADIBusManager::SynapseDriver>()) {
		requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
		    {{1, false}}, SynapseDriverOnPADIBusManager::Label(synapse_driver.value())});
	}
	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, UnavailableSynapseDriver)
{
	std::set<SynapseDriverOnPADIBusManager::SynapseDriver> unavailable_synapse_drivers;
	for (size_t i = 0; i < 17; ++i) {
		unavailable_synapse_drivers.insert(SynapseDriverOnPADIBusManager::SynapseDriver(i));
	}
	SynapseDriverOnPADIBusManager manager(unavailable_synapse_drivers);
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	for (auto const synapse_driver : iter_all<SynapseDriverOnPADIBusManager::SynapseDriver>()) {
		if (unavailable_synapse_drivers.contains(synapse_driver)) {
			continue;
		}
		requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
		    {{1, false}}, SynapseDriverOnPADIBusManager::Label(synapse_driver.value())});
	}
	auto const allocations = manager.solve(requests, GetParam());

	check(requests, allocations);

	for (size_t i = 0; i < requests.size(); ++i) {
		for (size_t j = 0; j < requests.at(i).shapes.size(); ++j) {
			for (auto const& [s, _] : allocations->at(i).synapse_drivers.at(j)) {
				EXPECT_FALSE(unavailable_synapse_drivers.contains(s));
			}
		}
	}
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, NoIsolatingMask)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	// use same label, leads to no isolating masks for requests
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{8, true}}, SynapseDriverOnPADIBusManager::Label(0b01100)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{16, true}}, SynapseDriverOnPADIBusManager::Label(0b01100)});
	auto const allocations = manager.solve(requests, GetParam());

	EXPECT_FALSE(allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, TooLargeRequest)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{SynapseDriverOnPADIBusManager::SynapseDriver::size + 1, true}},
	    SynapseDriverOnPADIBusManager::Label(0b01100)});
	auto const allocations = manager.solve(requests, GetParam());

	EXPECT_FALSE(allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, TooLargeRequests)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	// two times all synapse drivers
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{SynapseDriverOnPADIBusManager::SynapseDriver::size, true}},
	    SynapseDriverOnPADIBusManager::Label(0b01100)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{SynapseDriverOnPADIBusManager::SynapseDriver::size, true}},
	    SynapseDriverOnPADIBusManager::Label(0b10010)});
	auto const allocations = manager.solve(requests, GetParam());

	EXPECT_FALSE(allocations);
}

TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, NoIndividualPlacementPossible)
{
	SynapseDriverOnPADIBusManager manager;
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requests;
	// two times half of all synapse drivers with labels such that no individual placement is
	// possible
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{16, true}}, SynapseDriverOnPADIBusManager::Label(0b01100)});
	requests.push_back(SynapseDriverOnPADIBusManager::AllocationRequest{
	    {{16, true}}, SynapseDriverOnPADIBusManager::Label(0b01000)});
	auto const allocations = manager.solve(requests, GetParam());

	EXPECT_FALSE(allocations);
}


struct RandomTestGenerator
{
	using AllocationRequest = SynapseDriverOnPADIBusManager::AllocationRequest;
	using Shape = SynapseDriverOnPADIBusManager::AllocationRequest::Shape;

	RandomTestGenerator(uintmax_t seed) : m_seed(seed), m_rng(seed) {}

	std::pair<SynapseDriverOnPADIBusManager, std::vector<AllocationRequest>> operator()()
	{
		auto const unavailable_synapse_driver = get_unavailable_synapse_driver();
		auto const manager = SynapseDriverOnPADIBusManager(unavailable_synapse_driver);

		auto const allocation_requests = get_allocation_requests(unavailable_synapse_driver);

		return {manager, allocation_requests};
	}

	std::set<SynapseDriverOnPADIBus> get_unavailable_synapse_driver()
	{
		std::bernoulli_distribution d_unavailable(std::uniform_real_distribution<double>{}(m_rng));

		std::set<SynapseDriverOnPADIBus> unavailable_synapse_driver;
		for (auto const synapse_driver : iter_all<SynapseDriverOnPADIBus>()) {
			if (d_unavailable(m_rng)) {
				unavailable_synapse_driver.insert(synapse_driver);
			}
		}
		return unavailable_synapse_driver;
	}

	size_t get_num_allocation_requests(size_t num_shapes)
	{
		std::uniform_int_distribution<size_t> d(0, num_shapes);
		return d(m_rng);
	}

	std::vector<Shape> get_shapes(
	    std::set<SynapseDriverOnPADIBus> const& unavailable_synapse_driver)
	{
		std::bernoulli_distribution d_contiguous(std::uniform_real_distribution<double>{}(m_rng));
		std::vector<Shape> ret;
		size_t num_unavailable_synapse_driver = unavailable_synapse_driver.size();
		size_t num_available_synapse_driver =
		    SynapseDriverOnPADIBus::size - num_unavailable_synapse_driver;
		std::uniform_int_distribution<size_t> d(0, num_available_synapse_driver);
		size_t num_shapes = d(m_rng);
		if (num_shapes == 0) {
			return {};
		}
		for (size_t shape = 0; shape < num_shapes - 1; ++shape) {
			std::uniform_int_distribution<size_t> d(
			    0, num_available_synapse_driver - (num_shapes - shape));
			auto const size = d(m_rng) + 1;
			num_available_synapse_driver -= size;
			ret.push_back(Shape{size, d_contiguous(m_rng)});
		}
		ret.push_back(Shape{num_available_synapse_driver, d_contiguous(m_rng)});
		std::shuffle(ret.begin(), ret.end(), m_rng);
		return ret;
	}

	std::vector<AllocationRequest> get_allocation_requests(
	    std::set<SynapseDriverOnPADIBus> const& unavailable_synapse_driver)
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
				allocation_requests.at(request).shapes.push_back(shape);
			}
		}
		for (auto const& shape : shapes) {
			allocation_requests.back().shapes.push_back(shape);
		}
		std::shuffle(allocation_requests.begin(), allocation_requests.end(), m_rng);

		std::uniform_int_distribution<size_t> d_label(
		    SynapseDriverOnPADIBusManager::Label::min, SynapseDriverOnPADIBusManager::Label::max);
		for (auto& allocation_request : allocation_requests) {
			allocation_request.label = SynapseDriverOnPADIBusManager::Label(d_label(m_rng));
		}

		return allocation_requests;
	}

private:
	uintmax_t m_seed;
	std::mt19937 m_rng;
};


TEST_P(SynapseDriverOnPADIBusManagerTestsFixture, random)
{
	constexpr size_t num = 100;

	for (size_t i = 0; i < num; ++i) {
		auto const seed = std::random_device{}();
		RandomTestGenerator generator(seed);
		auto [manager, allocation_requests] = generator();
		auto options = GetParam();
		if (std::holds_alternative<SynapseDriverOnPADIBusManager::AllocationPolicyBacktracking>(
		        options)) {
			std::get<SynapseDriverOnPADIBusManager::AllocationPolicyBacktracking>(options)
			    .max_duration = std::chrono::milliseconds(100);
		}
		EXPECT_NO_THROW(manager.solve(allocation_requests, options)) << "Seed: " << seed;
	}
}

INSTANTIATE_TEST_SUITE_P(
    SynapseDriverOnPADIBusManager,
    SynapseDriverOnPADIBusManagerTestsFixture,
    testing::Values(
        SynapseDriverOnPADIBusManager::AllocationPolicyGreedy{false},
        SynapseDriverOnPADIBusManager::AllocationPolicyGreedy{true},
        SynapseDriverOnPADIBusManager::AllocationPolicyBacktracking{}));
