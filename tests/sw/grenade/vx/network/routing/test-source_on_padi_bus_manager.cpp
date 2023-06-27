#include <gtest/gtest.h>

#include "grenade/vx/network/routing/greedy/source_on_padi_bus_manager.h"
#include "halco/common/iter_all.h"
#include <random>

using namespace grenade::vx::network;
using namespace grenade::vx::network::routing::greedy;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(SourceOnPADIBusManager_Partition_Group, valid)
{
	// invalid
	{
		SourceOnPADIBusManager::Partition::Group group{
		    {0, 0, 1}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}};
		EXPECT_FALSE(group.valid());
	}
	{
		SourceOnPADIBusManager::Partition::Group group{{0, 1, 2}, {}};
		EXPECT_FALSE(group.valid());
	}
	// valid
	{
		SourceOnPADIBusManager::Partition::Group group{
		    {1, 0, 2}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}};
		EXPECT_TRUE(group.valid());
	}
}

TEST(SourceOnPADIBusManager_Partition, valid)
{
	// invalid
	{
		SourceOnPADIBusManager::Partition partition{
		    {{{0, 0, 1}, {}}},
		    {{{0, 1, 2}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}}},
		    {{{0, 1, 2}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}}}};
		EXPECT_FALSE(partition.valid());
	}
	{
		SourceOnPADIBusManager::Partition partition{
		    {{{0, 1, 2}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}}},
		    {{{0, 0, 1}, {}}},
		    {{{0, 1, 2}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}}}};
		EXPECT_FALSE(partition.valid());
	}
	{
		SourceOnPADIBusManager::Partition partition{
		    {{{0, 1, 2}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}}},
		    {{{0, 1, 2}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}}},
		    {{{0, 0, 1}, {}}}};
		EXPECT_FALSE(partition.valid());
	}
	// valid
	{
		SourceOnPADIBusManager::Partition partition{
		    {{{0, 1, 2}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}}},
		    {{{0, 1, 3}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}}},
		    {{{0, 1, 4}, {{{}}, {SynapseDriverOnDLSManager::Label()}, {}}}}};
		EXPECT_TRUE(partition.valid());
	}
}

TEST(SourceOnPADIBusManager, AllToAllInternalTop)
{
	SourceOnPADIBusManager manager;

	std::vector<SourceOnPADIBusManager::InternalSource> internal_sources;
	std::vector<SourceOnPADIBusManager::BackgroundSource> background_sources;
	std::vector<SourceOnPADIBusManager::ExternalSource> external_sources;

	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		SourceOnPADIBusManager::InternalSource source;
		source.out_degree[Receptor::Type::excitatory].fill(1);
		source.neuron = AtomicNeuronOnDLS(column, NeuronRowOnDLS::top);
		internal_sources.push_back(source);
	}

	auto const allocations = manager.solve(internal_sources, background_sources, external_sources);
}

TEST(SourceOnPADIBusManager, AllToAllInternalBottom)
{
	SourceOnPADIBusManager manager;

	std::vector<SourceOnPADIBusManager::InternalSource> internal_sources;
	std::vector<SourceOnPADIBusManager::BackgroundSource> background_sources;
	std::vector<SourceOnPADIBusManager::ExternalSource> external_sources;

	for (auto const column : iter_all<NeuronColumnOnDLS>()) {
		SourceOnPADIBusManager::InternalSource source;
		source.out_degree[Receptor::Type::excitatory].fill(1);
		source.neuron = AtomicNeuronOnDLS(column, NeuronRowOnDLS::bottom);
		internal_sources.push_back(source);
	}

	auto const allocations = manager.solve(internal_sources, background_sources, external_sources);
}

TEST(SourceOnPADIBusManager, Background64)
{
	SourceOnPADIBusManager manager;

	std::vector<SourceOnPADIBusManager::InternalSource> internal_sources;
	std::vector<SourceOnPADIBusManager::BackgroundSource> background_sources;
	std::vector<SourceOnPADIBusManager::ExternalSource> external_sources;

	for (size_t i = 0; i < 64; ++i) {
		SourceOnPADIBusManager::BackgroundSource source;
		source.out_degree[Receptor::Type::excitatory].fill(1);
		source.padi_bus = PADIBusOnDLS();
		background_sources.push_back(source);
	}

	auto const allocations = manager.solve(internal_sources, background_sources, external_sources);
}

TEST(SourceOnPADIBusManager, Background128)
{
	SourceOnPADIBusManager manager;

	std::vector<SourceOnPADIBusManager::InternalSource> internal_sources;
	std::vector<SourceOnPADIBusManager::BackgroundSource> background_sources;
	std::vector<SourceOnPADIBusManager::ExternalSource> external_sources;

	for (size_t i = 0; i < 128; ++i) {
		SourceOnPADIBusManager::BackgroundSource source;
		source.out_degree[Receptor::Type::excitatory].fill(0);
		source.out_degree[Receptor::Type::excitatory][AtomicNeuronOnDLS(Enum(i))] = 1;
		source.padi_bus = PADIBusOnDLS();
		background_sources.push_back(source);
	}

	auto const allocations = manager.solve(internal_sources, background_sources, external_sources);
}

TEST(SourceOnPADIBusManager, Background256)
{
	SourceOnPADIBusManager manager;

	std::vector<SourceOnPADIBusManager::InternalSource> internal_sources;
	std::vector<SourceOnPADIBusManager::BackgroundSource> background_sources;
	std::vector<SourceOnPADIBusManager::ExternalSource> external_sources;

	for (size_t i = 0; i < 256; ++i) {
		SourceOnPADIBusManager::BackgroundSource source;
		source.out_degree[Receptor::Type::excitatory].fill(0);
		source.out_degree[Receptor::Type::excitatory][AtomicNeuronOnDLS(Enum(i))] = 1;
		source.padi_bus = PADIBusOnDLS();
		background_sources.push_back(source);
	}

	auto const allocations = manager.solve(internal_sources, background_sources, external_sources);
}

TEST(SourceOnPADIBusManager, ExternalAllToAll)
{
	SourceOnPADIBusManager manager;

	std::vector<SourceOnPADIBusManager::InternalSource> internal_sources;
	std::vector<SourceOnPADIBusManager::BackgroundSource> background_sources;
	std::vector<SourceOnPADIBusManager::ExternalSource> external_sources;

	for (size_t i = 0; i < 256; ++i) {
		SourceOnPADIBusManager::ExternalSource source;
		source.out_degree[Receptor::Type::excitatory].fill(1);
		external_sources.push_back(source);
	}

	auto const allocations = manager.solve(internal_sources, background_sources, external_sources);
}
