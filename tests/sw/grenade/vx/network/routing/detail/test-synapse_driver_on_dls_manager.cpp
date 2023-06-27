#include <gtest/gtest.h>

#include "grenade/vx/network/routing/greedy/detail/synapse_driver_on_dls_manager.h"
#include "halco/common/iter_all.h"


using namespace grenade::vx::network::routing::greedy::detail;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(detail_SynapseDriverOnDLSManager, get_interdependent_padi_busses)
{
	// empty
	{
		EXPECT_TRUE(SynapseDriverOnDLSManager::get_interdependent_padi_busses({}).empty());
	}
	// independent
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(0), SynapseDriverOnDLSManager::Label(1)},
		     std::nullopt},
		    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(0), SynapseDriverOnDLSManager::Label(1)},
		     std::nullopt},
		};
		auto const padi_busses =
		    SynapseDriverOnDLSManager::get_interdependent_padi_busses(requested_allocations);
		std::set<std::set<PADIBusOnDLS>> expectation{
		    {PADIBusOnDLS(Enum(0))}, {PADIBusOnDLS(Enum(1))}};
		EXPECT_EQ(padi_busses, expectation);
	}
	// interdependent
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}, {PADIBusOnDLS(Enum(1)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(0), SynapseDriverOnDLSManager::Label(1)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(0)},
		    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}, {PADIBusOnDLS(Enum(2)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(0), SynapseDriverOnDLSManager::Label(1)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(0)},
		};
		auto const padi_busses =
		    SynapseDriverOnDLSManager::get_interdependent_padi_busses(requested_allocations);
		std::set<std::set<PADIBusOnDLS>> expectation{
		    {PADIBusOnDLS(Enum(0)), PADIBusOnDLS(Enum(1)), PADIBusOnDLS(Enum(2))}};
		EXPECT_EQ(padi_busses, expectation);
	}
	// independent and interdependent
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}, {PADIBusOnDLS(Enum(1)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(0), SynapseDriverOnDLSManager::Label(1)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(0)},
		    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}, {PADIBusOnDLS(Enum(2)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(0), SynapseDriverOnDLSManager::Label(1)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(0)},
		    {{{PADIBusOnDLS(Enum(3)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(0), SynapseDriverOnDLSManager::Label(1)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(1)},
		};
		auto const padi_busses =
		    SynapseDriverOnDLSManager::get_interdependent_padi_busses(requested_allocations);
		std::set<std::set<PADIBusOnDLS>> expectation{
		    {PADIBusOnDLS(Enum(0)), PADIBusOnDLS(Enum(1)), PADIBusOnDLS(Enum(2))},
		    {PADIBusOnDLS(Enum(3))}};
		EXPECT_EQ(padi_busses, expectation);
	}
}

TEST(detail_SynapseDriverOnDLSManager, get_requested_allocations_per_padi_bus)
{
	{
		EXPECT_TRUE(SynapseDriverOnDLSManager::get_requested_allocations_per_padi_bus({}).empty());
	}
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{1, true}}},
		      {PADIBusOnDLS(Enum(1)), {{1, true}, {4, false}}},
		      {PADIBusOnDLS(Enum(2)), {{3, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		    {{{PADIBusOnDLS(Enum(1)), {{2, false}}}},
		     {SynapseDriverOnDLSManager::Label(1), SynapseDriverOnDLSManager::Label(2)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)},
		};
		auto const requested_allocations_per_padi_bus =
		    SynapseDriverOnDLSManager::get_requested_allocations_per_padi_bus(
		        requested_allocations);
		SynapseDriverOnDLSManager::AllocationRequestPerPADIBus expectation{
		    {PADIBusOnDLS(Enum(0)), {{{{{1, true}}, SynapseDriverOnDLSManager::Label()}}, {0}}},
		    {PADIBusOnDLS(Enum(1)),
		     {{{{{1, true}, {4, false}}, SynapseDriverOnDLSManager::Label()},
		       {{{2, false}}, SynapseDriverOnDLSManager::Label()}},
		      {0, 1}}},
		    {PADIBusOnDLS(Enum(2)), {{{{{3, false}}, SynapseDriverOnDLSManager::Label()}}, {0}}},
		};
		EXPECT_EQ(requested_allocations_per_padi_bus, expectation);
	}
}

TEST(detail_SynapseDriverOnDLSManager, get_independent_allocation_requests)
{
	{
		EXPECT_TRUE(SynapseDriverOnDLSManager::get_independent_allocation_requests({}, {}).empty());
	}
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(1), SynapseDriverOnDLSManager::Label(2)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)},
		    {{{PADIBusOnDLS(Enum(2)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(1)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(13)},
		};
		{
			auto const independent_allocation_requests =
			    SynapseDriverOnDLSManager::get_independent_allocation_requests(
			        requested_allocations,
			        {PADIBusOnDLS(Enum(0)), PADIBusOnDLS(Enum(1)), PADIBusOnDLS(Enum(2))});
			std::vector<size_t> expectation{0, 2};
			EXPECT_EQ(independent_allocation_requests, expectation);
		}
		{
			auto const independent_allocation_requests =
			    SynapseDriverOnDLSManager::get_independent_allocation_requests(
			        requested_allocations, {PADIBusOnDLS(Enum(0)), PADIBusOnDLS(Enum(1))});
			std::vector<size_t> expectation{0};
			EXPECT_EQ(independent_allocation_requests, expectation);
		}
	}
}

TEST(detail_SynapseDriverOnDLSManager, get_unique_dependent_label_groups)
{
	{
		EXPECT_TRUE(SynapseDriverOnDLSManager::get_unique_dependent_label_groups({}, {}).empty());
	}
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(12)},
		    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(1), SynapseDriverOnDLSManager::Label(2)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)},
		    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(1)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(13)},
		    {{{PADIBusOnDLS(Enum(2)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		};
		{
			auto const unique_dependent_label_groups =
			    SynapseDriverOnDLSManager::get_unique_dependent_label_groups(
			        requested_allocations,
			        {PADIBusOnDLS(Enum(0)), PADIBusOnDLS(Enum(1)), PADIBusOnDLS(Enum(2))});
			std::vector<SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup>
			    expectation{
			        SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(12),
			        SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(13),
			        SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)};
			EXPECT_EQ(unique_dependent_label_groups, expectation);
		}
		{
			auto const unique_dependent_label_groups =
			    SynapseDriverOnDLSManager::get_unique_dependent_label_groups(
			        requested_allocations, {PADIBusOnDLS(Enum(1))});
			std::vector<SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup>
			    expectation{
			        SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(13),
			        SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)};
			EXPECT_EQ(unique_dependent_label_groups, expectation);
		}
	}
}

TEST(detail_SynapseDriverOnDLSManager, get_label_space)
{
	{
		EXPECT_TRUE(SynapseDriverOnDLSManager::get_label_space({}, {}, {}).empty());
	}
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(1), SynapseDriverOnDLSManager::Label(2),
		      SynapseDriverOnDLSManager::Label(3)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)},
		    {{{PADIBusOnDLS(Enum(2)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3),
		      SynapseDriverOnDLSManager::Label(4), SynapseDriverOnDLSManager::Label(5)},
		     std::nullopt},
		    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(1)},
		     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(13)},
		};
		std::set<PADIBusOnDLS> padi_busses{
		    PADIBusOnDLS(Enum(0)), PADIBusOnDLS(Enum(1)), PADIBusOnDLS(Enum(2))};
		auto const independent_allocation_requests =
		    SynapseDriverOnDLSManager::get_independent_allocation_requests(
		        requested_allocations, padi_busses);
		auto const unique_dependent_label_groups =
		    SynapseDriverOnDLSManager::get_unique_dependent_label_groups(
		        requested_allocations, padi_busses);
		auto const label_space = SynapseDriverOnDLSManager::get_label_space(
		    independent_allocation_requests, unique_dependent_label_groups, requested_allocations);
		std::vector<int64_t> expectation{2, 4, 1, 3};
		EXPECT_EQ(label_space, expectation);
	}
}

TEST(detail_SynapseDriverOnDLSManager, get_label_space_index)
{
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
	    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
	     std::nullopt},
	    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(1), SynapseDriverOnDLSManager::Label(2),
	      SynapseDriverOnDLSManager::Label(3)},
	     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)},
	    {{{PADIBusOnDLS(Enum(2)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3),
	      SynapseDriverOnDLSManager::Label(4), SynapseDriverOnDLSManager::Label(5)},
	     std::nullopt},
	    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(1)},
	     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(13)},
	};
	std::set<PADIBusOnDLS> padi_busses{
	    PADIBusOnDLS(Enum(0)), PADIBusOnDLS(Enum(1)), PADIBusOnDLS(Enum(2))};
	auto const independent_allocation_requests =
	    SynapseDriverOnDLSManager::get_independent_allocation_requests(
	        requested_allocations, padi_busses);
	auto const unique_dependent_label_groups =
	    SynapseDriverOnDLSManager::get_unique_dependent_label_groups(
	        requested_allocations, padi_busses);
	std::vector<size_t> expectation{0, 3, 1, 2};
	for (size_t i = 0; i < 4; ++i) {
		auto const label_space_index = SynapseDriverOnDLSManager::get_label_space_index(
		    independent_allocation_requests, unique_dependent_label_groups, requested_allocations,
		    i);
		EXPECT_EQ(label_space_index, expectation.at(i));
	}
}

TEST(detail_SynapseDriverOnDLSManager, update_labels)
{
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
	    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
	     std::nullopt},
	    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(1), SynapseDriverOnDLSManager::Label(2),
	      SynapseDriverOnDLSManager::Label(3)},
	     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)},
	    {{{PADIBusOnDLS(Enum(2)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3),
	      SynapseDriverOnDLSManager::Label(4), SynapseDriverOnDLSManager::Label(5)},
	     std::nullopt},
	    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(1)},
	     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(13)},
	};
	std::set<PADIBusOnDLS> padi_busses{
	    PADIBusOnDLS(Enum(0)), PADIBusOnDLS(Enum(1)), PADIBusOnDLS(Enum(2))};
	auto const independent_allocation_requests =
	    SynapseDriverOnDLSManager::get_independent_allocation_requests(
	        requested_allocations, padi_busses);
	auto const unique_dependent_label_groups =
	    SynapseDriverOnDLSManager::get_unique_dependent_label_groups(
	        requested_allocations, padi_busses);
	auto requested_allocations_per_padi_bus =
	    SynapseDriverOnDLSManager::get_requested_allocations_per_padi_bus(requested_allocations);
	PADIBusOnDLS padi_bus(Enum(1));
	for (auto const& requested_allocation : requested_allocations_per_padi_bus.at(padi_bus).first) {
		EXPECT_EQ(requested_allocation.label, SynapseDriverOnDLSManager::Label());
	}
	SynapseDriverOnDLSManager::update_labels(
	    requested_allocations_per_padi_bus.at(padi_bus), requested_allocations,
	    std::vector<int64_t>{1, 3, 0, 2}, independent_allocation_requests,
	    unique_dependent_label_groups);
	std::vector<SynapseDriverOnDLSManager::Label> expectation{
	    SynapseDriverOnDLSManager::Label(3), SynapseDriverOnDLSManager::Label(1)};
	size_t i = 0;
	for (auto const& requested_allocation : requested_allocations_per_padi_bus.at(padi_bus).first) {
		EXPECT_EQ(requested_allocation.label, expectation.at(i));
		i++;
	}
}

TEST(detail_SynapseDriverOnDLSManager, update_solution)
{
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
	    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
	     std::nullopt},
	    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(1), SynapseDriverOnDLSManager::Label(2),
	      SynapseDriverOnDLSManager::Label(3)},
	     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(42)},
	    {{{PADIBusOnDLS(Enum(2)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3),
	      SynapseDriverOnDLSManager::Label(4), SynapseDriverOnDLSManager::Label(5)},
	     std::nullopt},
	    {{{PADIBusOnDLS(Enum(1)), {{0, false}}}},
	     {SynapseDriverOnDLSManager::Label(1)},
	     SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup(13)},
	};
	std::set<PADIBusOnDLS> padi_busses{
	    PADIBusOnDLS(Enum(0)), PADIBusOnDLS(Enum(1)), PADIBusOnDLS(Enum(2))};
	auto const independent_allocation_requests =
	    SynapseDriverOnDLSManager::get_independent_allocation_requests(
	        requested_allocations, padi_busses);
	auto const unique_dependent_label_groups =
	    SynapseDriverOnDLSManager::get_unique_dependent_label_groups(
	        requested_allocations, padi_busses);
	auto requested_allocations_per_padi_bus =
	    SynapseDriverOnDLSManager::get_requested_allocations_per_padi_bus(requested_allocations);
	PADIBusOnDLS padi_bus(Enum(1));
	for (auto const& requested_allocation : requested_allocations_per_padi_bus.at(padi_bus).first) {
		EXPECT_EQ(requested_allocation.label, SynapseDriverOnDLSManager::Label());
	}
	std::vector<grenade::vx::network::routing::greedy::SynapseDriverOnPADIBusManager::Allocation>
	    local_solution{
	        {{{}}},
	        {{{}}},
	    };
	std::vector<SynapseDriverOnDLSManager::Allocation> solution(requested_allocations.size());
	SynapseDriverOnDLSManager::update_solution(
	    solution, local_solution, requested_allocations_per_padi_bus.at(padi_bus), padi_bus,
	    requested_allocations, std::vector<int64_t>{1, 3, 0, 2}, independent_allocation_requests,
	    unique_dependent_label_groups);
	std::vector<SynapseDriverOnDLSManager::Label> expectation{
	    SynapseDriverOnDLSManager::Label(), SynapseDriverOnDLSManager::Label(3),
	    SynapseDriverOnDLSManager::Label(), SynapseDriverOnDLSManager::Label(1)};
	size_t i = 0;
	for (auto const& sol : solution) {
		EXPECT_EQ(sol.label, expectation.at(i));
		i++;
	}
}

TEST(detail_SynapseDriverOnDLSManager, valid_solution)
{
	// empty
	{
		EXPECT_TRUE(SynapseDriverOnDLSManager::valid_solution({}, {}));
	}
	// wrong size
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		};
		std::vector<SynapseDriverOnDLSManager::Allocation> solution{};
		EXPECT_FALSE(SynapseDriverOnDLSManager::valid_solution(solution, requested_allocations));
	}
	// wrong label
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		};
		std::vector<SynapseDriverOnDLSManager::Allocation> solution{
		    {{{{}}}, SynapseDriverOnDLSManager::Label(12)}};
		EXPECT_FALSE(SynapseDriverOnDLSManager::valid_solution(solution, requested_allocations));
	}
	// wrong PADI-bus number
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		};
		std::vector<SynapseDriverOnDLSManager::Allocation> solution{
		    {{{PADIBusOnDLS(Enum(0)), {}}, {PADIBusOnDLS(Enum(1)), {}}},
		     SynapseDriverOnDLSManager::Label(2)}};
		EXPECT_FALSE(SynapseDriverOnDLSManager::valid_solution(solution, requested_allocations));
	}
	// wrong PADI-bus
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{0, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		};
		std::vector<SynapseDriverOnDLSManager::Allocation> solution{
		    {{{PADIBusOnDLS(Enum(1)), {}}}, SynapseDriverOnDLSManager::Label(2)}};
		EXPECT_FALSE(SynapseDriverOnDLSManager::valid_solution(solution, requested_allocations));
	}
	// wrong shape number
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{1, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		};
		std::vector<SynapseDriverOnDLSManager::Allocation> solution{
		    {{{PADIBusOnDLS(Enum(0)), {{}}}}, SynapseDriverOnDLSManager::Label(2)}};
		EXPECT_FALSE(SynapseDriverOnDLSManager::valid_solution(solution, requested_allocations));
	}
	// wrong synapse driver number
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{1, false}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		};
		std::vector<SynapseDriverOnDLSManager::Allocation> solution{
		    {{{PADIBusOnDLS(Enum(0)), {{{}}}}}, SynapseDriverOnDLSManager::Label(2)}};
		EXPECT_FALSE(SynapseDriverOnDLSManager::valid_solution(solution, requested_allocations));
	}
	// not contiguous
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{2, true}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		};
		std::vector<SynapseDriverOnDLSManager::Allocation> solution{
		    {{{PADIBusOnDLS(Enum(0)),
		       {{{{SynapseDriverOnPADIBus(0),
		           grenade::vx::network::routing::greedy::SynapseDriverOnPADIBusManager::Mask()},
		          {SynapseDriverOnPADIBus(2), grenade::vx::network::routing::greedy::
		                                          SynapseDriverOnPADIBusManager::Mask()}}}}}},
		     SynapseDriverOnDLSManager::Label(2)}};
		EXPECT_FALSE(SynapseDriverOnDLSManager::valid_solution(solution, requested_allocations));
	}
	// valid
	{
		std::vector<SynapseDriverOnDLSManager::AllocationRequest> requested_allocations{
		    {{{PADIBusOnDLS(Enum(0)), {{2, true}}}},
		     {SynapseDriverOnDLSManager::Label(2), SynapseDriverOnDLSManager::Label(3)},
		     std::nullopt},
		};
		std::vector<SynapseDriverOnDLSManager::Allocation> solution{
		    {{{PADIBusOnDLS(Enum(0)),
		       {{{{SynapseDriverOnPADIBus(0),
		           grenade::vx::network::routing::greedy::SynapseDriverOnPADIBusManager::Mask()},
		          {SynapseDriverOnPADIBus(1), grenade::vx::network::routing::greedy::
		                                          SynapseDriverOnPADIBusManager::Mask()}}}}}},
		     SynapseDriverOnDLSManager::Label(2)}};
		EXPECT_TRUE(SynapseDriverOnDLSManager::valid_solution(solution, requested_allocations));
	}
}
