#include <gtest/gtest.h>

#include "grenade/vx/network/placed_atomic/routing/detail/source_on_padi_bus_manager.h"
#include "grenade/vx/network/placed_atomic/routing/detail/source_on_padi_bus_manager.tcc"
#include "halco/common/iter_all.h"
#include "lola/vx/v3/synapse.h"


using namespace grenade::vx::network::placed_atomic::routing::detail;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

TEST(detail_SourceOnPADIBusManager, split_linear)
{
	EXPECT_TRUE(SourceOnPADIBusManager::split_linear({}).empty());

	std::vector<size_t> filter;
	for (size_t i = 10; i < 123; ++i) {
		filter.push_back(i);
	}
	auto const split = SourceOnPADIBusManager::split_linear(filter);
	std::vector<std::vector<size_t>> expectation(2);
	for (size_t i = 0; i < lola::vx::v3::SynapseMatrix::Label::size; ++i) {
		expectation.at(0).push_back(filter.at(i));
	}
	for (size_t i = lola::vx::v3::SynapseMatrix::Label::size; i < filter.size(); ++i) {
		expectation.at(1).push_back(filter.at(i));
	}
	EXPECT_EQ(split, expectation);
}

TEST(detail_SourceOnPADIBusManager, get_num_synapse_drivers)
{
	std::vector<size_t> filter{0, 1, 2};
	std::vector<SourceOnPADIBusManager::InternalSource> sources(4);
	for (auto& source : sources) {
		source.out_degree[grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory]
		    .fill(0);
	}
	sources.at(0)
	    .out_degree[grenade::vx::network::placed_atomic::Projection::ReceptorType::inhibitory]
	    .fill(0);
	sources.at(0)
	    .out_degree[grenade::vx::network::placed_atomic::Projection::ReceptorType::inhibitory]
	    .at(AtomicNeuronOnDLS(NeuronColumnOnDLS(), NeuronRowOnDLS(1))) = 17;
	sources.at(0)
	    .out_degree[grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory]
	    .at(AtomicNeuronOnDLS(Enum(3))) = 12;
	sources.at(1)
	    .out_degree[grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory]
	    .at(AtomicNeuronOnDLS(Enum(3))) = 10;
	sources.at(2)
	    .out_degree[grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory]
	    .at(AtomicNeuronOnDLS(Enum(5))) = 5;

	auto const num_synapse_drivers =
	    SourceOnPADIBusManager::get_num_synapse_drivers(sources, filter);
	typed_array<size_t, PADIBusBlockOnDLS> expectation{11, 9};
	EXPECT_EQ(num_synapse_drivers, expectation);
}

TEST(detail_SourceOnPADIBusManager, distribute_external_sources_linear)
{
	std::vector<SourceOnPADIBusManager::ExternalSource> sources(68);
	for (size_t i = 0; i < 4; ++i) {
		sources.at(i)
		    .out_degree[grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory]
		    .fill(32);
	}
	for (size_t i = 4; i < 68; ++i) {
		sources.at(i)
		    .out_degree[grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory]
		    .fill(0);
		sources.at(i)
		    .out_degree[grenade::vx::network::placed_atomic::Projection::ReceptorType::excitatory]
		    .at(AtomicNeuronOnDLS(Enum(i))) = 1;
	}
	typed_array<size_t, PADIBusOnDLS> used_num_synapse_drivers{16, 0, 0, 0, 0, 0, 0, 0};
	auto const distributed_external_sources =
	    SourceOnPADIBusManager::distribute_external_sources_linear(
	        sources, used_num_synapse_drivers);

	typed_array<std::vector<std::vector<size_t>>, PADIBusOnDLS> expectation{
	    std::vector<std::vector<size_t>>{{0}},    std::vector<std::vector<size_t>>{{1, 2}},
	    std::vector<std::vector<size_t>>{{}}, // filled below
	    std::vector<std::vector<size_t>>{},       std::vector<std::vector<size_t>>{{0, 1}},
	    std::vector<std::vector<size_t>>{{2, 3}}, std::vector<std::vector<size_t>>{},
	    std::vector<std::vector<size_t>>{}};
	for (size_t i = 3; i < 67; ++i) {
		expectation[PADIBusOnDLS(Enum(2))].at(0).push_back(i);
	}
	expectation[PADIBusOnDLS(Enum(2))].push_back({67});

	EXPECT_EQ(distributed_external_sources, expectation);
}

TEST(detail_SourceOnPADIBusManager, get_allocation_requests_internal)
{
	std::vector<std::vector<size_t>> filter(2);
	for (size_t i = 0; i < lola::vx::v3::SynapseMatrix::Label::size; ++i) {
		filter.at(0).push_back(i);
	}
	for (size_t i = 0; i < 32; ++i) {
		filter.at(1).push_back(i + lola::vx::v3::SynapseMatrix::Label::size);
	}
	PADIBusOnPADIBusBlock padi_bus(Enum(2));

	typed_array<std::vector<size_t>, PADIBusOnDLS> num_synapse_drivers{
	    std::vector<size_t>{},     std::vector<size_t>{}, std::vector<size_t>{12, 10},
	    std::vector<size_t>{},     std::vector<size_t>{}, std::vector<size_t>{},
	    std::vector<size_t>{5, 1}, std::vector<size_t>{}};

	auto const allocation_requests = SourceOnPADIBusManager::get_allocation_requests_internal(
	    filter, padi_bus, NeuronBackendConfigBlockOnDLS(), num_synapse_drivers);

	std::vector<
	    grenade::vx::network::placed_atomic::routing::SynapseDriverOnDLSManager::AllocationRequest>
	    expectation{
	        {{{PADIBusOnDLS(Enum(2)), {{12, false}}}, {PADIBusOnDLS(Enum(6)), {{5, false}}}},
	         {SourceOnPADIBusManager::Label(8), SourceOnPADIBusManager::Label(8),
	          SourceOnPADIBusManager::Label(8), SourceOnPADIBusManager::Label(9),
	          SourceOnPADIBusManager::Label(9), SourceOnPADIBusManager::Label(9),
	          SourceOnPADIBusManager::Label(10), SourceOnPADIBusManager::Label(10),
	          SourceOnPADIBusManager::Label(10), SourceOnPADIBusManager::Label(11),
	          SourceOnPADIBusManager::Label(11), SourceOnPADIBusManager::Label(11)},
	         std::nullopt},
	        {{{PADIBusOnDLS(Enum(2)), {{10, false}}}, {PADIBusOnDLS(Enum(6)), {{1, false}}}},
	         {SourceOnPADIBusManager::Label(9), SourceOnPADIBusManager::Label(10),
	          SourceOnPADIBusManager::Label(11), SourceOnPADIBusManager::Label(8),
	          SourceOnPADIBusManager::Label(10), SourceOnPADIBusManager::Label(11),
	          SourceOnPADIBusManager::Label(8), SourceOnPADIBusManager::Label(9),
	          SourceOnPADIBusManager::Label(11), SourceOnPADIBusManager::Label(8),
	          SourceOnPADIBusManager::Label(9), SourceOnPADIBusManager::Label(10)},
	         std::nullopt}};

	EXPECT_EQ(allocation_requests, expectation);
}

TEST(detail_SourceOnPADIBusManager, get_allocation_requests_background)
{
	std::vector<std::vector<size_t>> filter(2);
	for (size_t i = 0; i < lola::vx::v3::SynapseMatrix::Label::size; ++i) {
		filter.at(0).push_back(i);
	}
	for (size_t i = 0; i < 32; ++i) {
		filter.at(1).push_back(i + lola::vx::v3::SynapseMatrix::Label::size);
	}
	PADIBusOnDLS padi_bus(Enum(2));

	std::vector<size_t> num_synapse_drivers{12, 10};

	auto const allocation_requests = SourceOnPADIBusManager::get_allocation_requests_background(
	    filter, padi_bus, num_synapse_drivers);

	std::vector<
	    grenade::vx::network::placed_atomic::routing::SynapseDriverOnDLSManager::AllocationRequest>
	    expectation{
	        {{{PADIBusOnDLS(Enum(2)), {{12, false}}}}, {}, std::nullopt},
	        {{{PADIBusOnDLS(Enum(2)), {{10, false}}}}, {}, std::nullopt}};

	EXPECT_EQ(allocation_requests.size(), expectation.size());
	for (size_t i = 0; i < expectation.size(); ++i) {
		EXPECT_EQ(allocation_requests.at(i).shapes, expectation.at(i).shapes);
		EXPECT_EQ(
		    allocation_requests.at(i).dependent_label_group,
		    expectation.at(i).dependent_label_group);
		EXPECT_EQ(allocation_requests.at(i).labels.size(), SourceOnPADIBusManager::Label::size);
	}
}

TEST(detail_SourceOnPADIBusManager, get_allocation_requests_external)
{
	PADIBusOnDLS padi_bus(Enum(2));

	size_t num_synapse_drivers = 12;

	auto const allocation_requests =
	    SourceOnPADIBusManager::get_allocation_requests_external(padi_bus, num_synapse_drivers);

	grenade::vx::network::placed_atomic::routing::SynapseDriverOnDLSManager::AllocationRequest
	    expectation{{{PADIBusOnDLS(Enum(2)), {{12, false}}}}, {}, std::nullopt};

	EXPECT_EQ(allocation_requests.shapes, expectation.shapes);
	EXPECT_EQ(allocation_requests.dependent_label_group, expectation.dependent_label_group);
	EXPECT_EQ(allocation_requests.labels.size(), SourceOnPADIBusManager::Label::size);
}
