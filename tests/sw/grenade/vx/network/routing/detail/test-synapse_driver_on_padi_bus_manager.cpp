#include <gtest/gtest.h>

#include "grenade/vx/network/routing/greedy/detail/synapse_driver_on_padi_bus_manager.h"
#include "halco/common/iter_all.h"


using namespace grenade::vx::network::routing::greedy::detail;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(detail_SynapseDriverOnPADIBusManager, forwards)
{
	for (auto const synapse_driver : iter_all<SynapseDriverOnPADIBusManager::SynapseDriver>()) {
		for (auto const label : iter_all<SynapseDriverOnPADIBusManager::Label>()) {
			for (auto const mask : iter_all<SynapseDriverOnPADIBusManager::Mask>()) {
				EXPECT_EQ(
				    SynapseDriverOnPADIBusManager::forwards(label, mask, synapse_driver),
				    (label.value() & mask.value()) == (synapse_driver.value() & mask.value()));
			}
		}
	}
}

TEST(detail_SynapseDriverOnPADIBusManager, has_unique_labels)
{
	// unique labels
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{1, false}}, SynapseDriverOnPADIBusManager::Label(0)},
		    {{{1, false}}, SynapseDriverOnPADIBusManager::Label(10)},
		};
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::has_unique_labels(requested_allocations));
	}
	// non-unique labels
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{1, false}}, SynapseDriverOnPADIBusManager::Label(0)},
		    {{{1, false}}, SynapseDriverOnPADIBusManager::Label(10)},
		    {{{1, false}}, SynapseDriverOnPADIBusManager::Label(10)},
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::has_unique_labels(requested_allocations));
	}
}

TEST(detail_SynapseDriverOnPADIBusManager, allocations_fit_available_size)
{
	std::set<SynapseDriverOnPADIBus> unavailable_synapse_drivers;
	// use iteration of synapse drivers to sweep size of set of unavailable synapse drivers
	for (auto const synapse_driver : iter_all<SynapseDriverOnPADIBusManager::SynapseDriver>()) {
		unavailable_synapse_drivers.insert(synapse_driver);
		// always requested less than available synapse drivers
		{
			std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
			    {{{SynapseDriverOnPADIBusManager::SynapseDriver::size -
			           std::min(
			               SynapseDriverOnPADIBusManager::SynapseDriver::size,
			               1 + unavailable_synapse_drivers.size()),
			       false}},
			     SynapseDriverOnPADIBusManager::Label()},
			};
			EXPECT_TRUE(SynapseDriverOnPADIBusManager::allocations_fit_available_size(
			    unavailable_synapse_drivers, requested_allocations));
		}
		// always requested equal than available synapse drivers
		{
			std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
			    {{{SynapseDriverOnPADIBusManager::SynapseDriver::size -
			           unavailable_synapse_drivers.size(),
			       false}},
			     SynapseDriverOnPADIBusManager::Label()},
			};
			EXPECT_TRUE(SynapseDriverOnPADIBusManager::allocations_fit_available_size(
			    unavailable_synapse_drivers, requested_allocations));
		}
		// always requested more than available synapse drivers
		{
			std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
			    {{{1 + SynapseDriverOnPADIBusManager::SynapseDriver::size -
			           unavailable_synapse_drivers.size(),
			       false}},
			     SynapseDriverOnPADIBusManager::Label()},
			};
			EXPECT_FALSE(SynapseDriverOnPADIBusManager::allocations_fit_available_size(
			    unavailable_synapse_drivers, requested_allocations));
		}
	}
}

TEST(detail_SynapseDriverOnPADIBusManager, generate_isolating_masks)
{
	// no requested allocations -> no isolating masks
	{
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::generate_isolating_masks({}).empty());
	}
	// single requested allocation -> all masks isolate
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		auto const masks =
		    SynapseDriverOnPADIBusManager::generate_isolating_masks(requested_allocations);
		EXPECT_EQ(masks.size(), 1);
		EXPECT_EQ(masks.at(0).size(), SynapseDriverOnPADIBusManager::Mask::size);
	}
	// three requested allocation with different label -> some masks isolate
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label(0b00001)},
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label(0b00010)},
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label(0b00110)},
		};
		auto const masks =
		    SynapseDriverOnPADIBusManager::generate_isolating_masks(requested_allocations);
		EXPECT_EQ(masks.size(), 3);
		EXPECT_GT(masks.at(0).size(), 0);
		EXPECT_GT(masks.at(1).size(), 0);
		EXPECT_GT(masks.at(2).size(), 0);
		std::vector<SynapseDriverOnPADIBusManager::Mask> expected_masks_0;
		std::vector<SynapseDriverOnPADIBusManager::Mask> expected_masks_1;
		std::vector<SynapseDriverOnPADIBusManager::Mask> expected_masks_2;
		for (auto const mask : iter_all<SynapseDriverOnPADIBusManager::Mask>()) {
			// if no bits in the mask are set, where the label is unequal to all other requests, the
			// mask does not isolate the request
			if (mask.value() & 0b00011) {
				expected_masks_0.push_back(mask);
			}
			// to differentiate between request 1 and 2, the third bit is required to be set.
			// Additionally to differentiate between request 0 and 1 one of the bits [0,1] is
			// required to be set.
			if ((mask.value() & 0b00100) && (mask.value() & 0b00011)) {
				expected_masks_1.push_back(mask);
			}
			// to differentiate between request 2 and [0,1], the tird bit is required to be set
			if (mask.value() & 0b00100) {
				expected_masks_2.push_back(mask);
			}
		}
		EXPECT_EQ(masks.at(0), expected_masks_0);
		EXPECT_EQ(masks.at(1), expected_masks_1);
		EXPECT_EQ(masks.at(2), expected_masks_2);
	}
	// two requested allocation with same label -> no masks isolate
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label(0b00001)},
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label(0b00001)},
		};
		auto const masks =
		    SynapseDriverOnPADIBusManager::generate_isolating_masks(requested_allocations);
		EXPECT_EQ(masks.size(), 0);
	}
}

TEST(detail_SynapseDriverOnPADIBusManager, allocations_can_be_isolated)
{
	// allocations can be isolated exactly if for every requested allocation, isolating mask values
	// are available
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		SynapseDriverOnPADIBusManager::IsolatingMasks isolating_masks{};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::allocations_can_be_isolated(
		    isolating_masks, requested_allocations));
	}
	{
		// content agnostic
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label()},
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		SynapseDriverOnPADIBusManager::IsolatingMasks isolating_masks{
		    {0, {SynapseDriverOnPADIBusManager::Mask(0)}},
		    {1, {SynapseDriverOnPADIBusManager::Mask(0)}}};
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::allocations_can_be_isolated(
		    isolating_masks, requested_allocations));
	}
}

TEST(detail_SynapseDriverOnPADIBusManager, generate_isolated_synapse_drivers)
{
	std::set<SynapseDriverOnPADIBus> unavailable_synapse_drivers{SynapseDriverOnPADIBus()};
	// allocations can be isolated exactly if for every requested allocation, isolating mask values
	// are available
	std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
	    {{{0, false}}, SynapseDriverOnPADIBusManager::Label(0b00001)},
	    {{{0, false}}, SynapseDriverOnPADIBusManager::Label(0b00010)},
	};
	SynapseDriverOnPADIBusManager::IsolatingMasks isolating_masks{
	    {0,
	     {SynapseDriverOnPADIBusManager::Mask(0b11110),
	      SynapseDriverOnPADIBusManager::Mask(0b11111),
	      SynapseDriverOnPADIBusManager::Mask(0b11101)}}, // incomplete, but enough for test
	    {1,
	     {SynapseDriverOnPADIBusManager::Mask(0b11110),
	      SynapseDriverOnPADIBusManager::Mask(0b11111),
	      SynapseDriverOnPADIBusManager::Mask(0b11101)}}};
	auto const isolated_synapse_drivers =
	    SynapseDriverOnPADIBusManager::generate_isolated_synapse_drivers(
	        unavailable_synapse_drivers, isolating_masks, requested_allocations);
	EXPECT_EQ(isolated_synapse_drivers.size(), 3);
	for (auto const& [synapse_driver, masked_allocations] : isolated_synapse_drivers) {
		EXPECT_FALSE(unavailable_synapse_drivers.contains(synapse_driver));
	}
	auto const synapse_driver_1 = isolated_synapse_drivers.at(SynapseDriverOnPADIBus(1));
	std::vector<std::pair<size_t, SynapseDriverOnPADIBusManager::Mask>> expected_synapse_driver_1{
	    {0, SynapseDriverOnPADIBusManager::Mask(0b11110)}};
	EXPECT_EQ(synapse_driver_1, expected_synapse_driver_1);
	auto const synapse_driver_2 = isolated_synapse_drivers.at(SynapseDriverOnPADIBus(2));
	std::vector<std::pair<size_t, SynapseDriverOnPADIBusManager::Mask>> expected_synapse_driver_2{
	    {1, SynapseDriverOnPADIBusManager::Mask(0b11110)}};
	EXPECT_EQ(synapse_driver_2, expected_synapse_driver_2);
	auto const synapse_driver_3 = isolated_synapse_drivers.at(SynapseDriverOnPADIBus(3));
	std::vector<std::pair<size_t, SynapseDriverOnPADIBusManager::Mask>> expected_synapse_driver_3{
	    {0, SynapseDriverOnPADIBusManager::Mask(0b11101)},
	    {1, SynapseDriverOnPADIBusManager::Mask(0b11110)}};
	EXPECT_EQ(synapse_driver_3, expected_synapse_driver_3);
}

TEST(detail_SynapseDriverOnPADIBusManager, allocations_can_be_placed_individually)
{
	// non-contiguous positive
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		SynapseDriverOnPADIBusManager::IsolatedSynapseDrivers isolated_synapse_drivers{
		    {SynapseDriverOnPADIBus(0), {{0, SynapseDriverOnPADIBusManager::Mask()}}},
		    {SynapseDriverOnPADIBus(2), {{0, SynapseDriverOnPADIBusManager::Mask()}}},
		};
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::allocations_can_be_placed_individually(
		    isolated_synapse_drivers, requested_allocations));
	}
	// non-contiguous negative
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		SynapseDriverOnPADIBusManager::IsolatedSynapseDrivers isolated_synapse_drivers{
		    {SynapseDriverOnPADIBus(0), {{0, SynapseDriverOnPADIBusManager::Mask()}}},
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::allocations_can_be_placed_individually(
		    isolated_synapse_drivers, requested_allocations));
	}
	// contiguous positive
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, true}}, SynapseDriverOnPADIBusManager::Label()},
		};
		SynapseDriverOnPADIBusManager::IsolatedSynapseDrivers isolated_synapse_drivers{
		    {SynapseDriverOnPADIBus(1), {{0, SynapseDriverOnPADIBusManager::Mask()}}},
		    {SynapseDriverOnPADIBus(2), {{0, SynapseDriverOnPADIBusManager::Mask()}}},
		};
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::allocations_can_be_placed_individually(
		    isolated_synapse_drivers, requested_allocations));
	}
	// contiguous negative - non-contiguous
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, true}}, SynapseDriverOnPADIBusManager::Label()},
		};
		SynapseDriverOnPADIBusManager::IsolatedSynapseDrivers isolated_synapse_drivers{
		    {SynapseDriverOnPADIBus(1), {{0, SynapseDriverOnPADIBusManager::Mask()}}},
		    {SynapseDriverOnPADIBus(3), {{0, SynapseDriverOnPADIBusManager::Mask()}}},
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::allocations_can_be_placed_individually(
		    isolated_synapse_drivers, requested_allocations));
	}
	// contiguous negative - not enough
	{
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, true}}, SynapseDriverOnPADIBusManager::Label()},
		};
		SynapseDriverOnPADIBusManager::IsolatedSynapseDrivers isolated_synapse_drivers{
		    {SynapseDriverOnPADIBus(1), {{0, SynapseDriverOnPADIBusManager::Mask()}}},
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::allocations_can_be_placed_individually(
		    isolated_synapse_drivers, requested_allocations));
	}
}

TEST(detail_SynapseDriverOnPADIBusManager, is_contiguous)
{
	// empty set is contiguous
	EXPECT_TRUE(SynapseDriverOnPADIBusManager::is_contiguous({}));
	// full set is contiguous
	{
		std::set<SynapseDriverOnPADIBusManager::SynapseDriver> synapse_drivers;
		for (auto const synapse_driver : iter_all<SynapseDriverOnPADIBusManager::SynapseDriver>()) {
			synapse_drivers.insert(synapse_driver);
		}
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::is_contiguous(synapse_drivers));
	}
	// contiguous
	{
		std::set<SynapseDriverOnPADIBusManager::SynapseDriver> synapse_drivers{
		    SynapseDriverOnPADIBusManager::SynapseDriver(3),
		    SynapseDriverOnPADIBusManager::SynapseDriver(4),
		    SynapseDriverOnPADIBusManager::SynapseDriver(5),
		    SynapseDriverOnPADIBusManager::SynapseDriver(6),
		};
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::is_contiguous(synapse_drivers));
	}
	// non-contiguous
	{
		std::set<SynapseDriverOnPADIBusManager::SynapseDriver> synapse_drivers{
		    SynapseDriverOnPADIBusManager::SynapseDriver(3),
		    SynapseDriverOnPADIBusManager::SynapseDriver(4),
		    SynapseDriverOnPADIBusManager::SynapseDriver(6),
		    SynapseDriverOnPADIBusManager::SynapseDriver(7),
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::is_contiguous(synapse_drivers));
	}
}

TEST(detail_SynapseDriverOnPADIBusManager, valid)
{
	// empty
	{
		std::vector<SynapseDriverOnPADIBusManager::Allocation> allocations{
		    {{{}}},
		};
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{0, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::valid(allocations, requested_allocations));
	}
	// too few shapes
	{
		std::vector<SynapseDriverOnPADIBusManager::Allocation> allocations{
		    {{}},
		};
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::valid(allocations, requested_allocations));
	}
	// non-unique synapse drivers in one shape
	{
		std::vector<SynapseDriverOnPADIBusManager::Allocation> allocations{
		    {{{{SynapseDriverOnPADIBus(0), SynapseDriverOnPADIBusManager::Mask()},
		       {SynapseDriverOnPADIBus(0), SynapseDriverOnPADIBusManager::Mask()}}}},
		};
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::valid(allocations, requested_allocations));
	}
	// non-unique synapse drivers in different allocations
	{
		std::vector<SynapseDriverOnPADIBusManager::Allocation> allocations{
		    {{{{SynapseDriverOnPADIBus(0), SynapseDriverOnPADIBusManager::Mask()}}}},
		    {{{{SynapseDriverOnPADIBus(0), SynapseDriverOnPADIBusManager::Mask()}}}},
		};
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{1, false}}, SynapseDriverOnPADIBusManager::Label()},
		    {{{1, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::valid(allocations, requested_allocations));
	}
	// too few synapse drivers for shape
	{
		std::vector<SynapseDriverOnPADIBusManager::Allocation> allocations{
		    {{{}}},
		};
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::valid(allocations, requested_allocations));
	}
	// non-contiguous, but contiguous expected
	{
		std::vector<SynapseDriverOnPADIBusManager::Allocation> allocations{
		    {{{{SynapseDriverOnPADIBus(0), SynapseDriverOnPADIBusManager::Mask()},
		       {SynapseDriverOnPADIBus(2), SynapseDriverOnPADIBusManager::Mask()}}}},
		};
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, true}}, SynapseDriverOnPADIBusManager::Label()},
		};
		EXPECT_FALSE(SynapseDriverOnPADIBusManager::valid(allocations, requested_allocations));
	}
	// non-contiguous and non-contiguous expected
	{
		std::vector<SynapseDriverOnPADIBusManager::Allocation> allocations{
		    {{{{SynapseDriverOnPADIBus(0), SynapseDriverOnPADIBusManager::Mask()},
		       {SynapseDriverOnPADIBus(2), SynapseDriverOnPADIBusManager::Mask()}}}},
		};
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, false}}, SynapseDriverOnPADIBusManager::Label()},
		};
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::valid(allocations, requested_allocations));
	}
	// contiguous and contiguous expected
	{
		std::vector<SynapseDriverOnPADIBusManager::Allocation> allocations{
		    {{{{SynapseDriverOnPADIBus(0), SynapseDriverOnPADIBusManager::Mask()},
		       {SynapseDriverOnPADIBus(1), SynapseDriverOnPADIBusManager::Mask()}}}},
		};
		std::vector<SynapseDriverOnPADIBusManager::AllocationRequest> requested_allocations{
		    {{{2, true}}, SynapseDriverOnPADIBusManager::Label()},
		};
		EXPECT_TRUE(SynapseDriverOnPADIBusManager::valid(allocations, requested_allocations));
	}
}
