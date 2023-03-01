#include "grenade/vx/network/placed_atomic/routing/detail/source_on_padi_bus_manager.h"

#include "grenade/vx/network/placed_atomic/routing/detail/source_on_padi_bus_manager.tcc"
#include "halco/common/iter_all.h"
#include "hate/math.h"
#include "hate/multidim_iterator.h"
#include "lola/vx/v3/synapse.h"
#include <stdexcept>

namespace grenade::vx::network::placed_atomic::routing::detail {

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;

std::vector<std::vector<size_t>> SourceOnPADIBusManager::split_linear(
    std::vector<size_t> const& filter)
{
	std::vector<std::vector<size_t>> split(hate::math::round_up_integer_division(
	    filter.size(), lola::vx::v3::SynapseMatrix::Label::size));

	for (size_t i = 0; i < filter.size(); ++i) {
		split.at(i / lola::vx::v3::SynapseMatrix::Label::size).push_back(filter.at(i));
	}

	return split;
}

std::vector<SynapseDriverOnDLSManager::AllocationRequest>
SourceOnPADIBusManager::get_allocation_requests_internal(
    std::vector<std::vector<size_t>> const& filter,
    halco::hicann_dls::vx::v3::PADIBusOnPADIBusBlock const& padi_bus,
    NeuronBackendConfigBlockOnDLS const& backend_block,
    halco::common::typed_array<std::vector<size_t>, halco::hicann_dls::vx::v3::PADIBusOnDLS> const&
        num_synapse_drivers)
{
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> allocation_requests(filter.size());
	for (size_t i = 0; i < filter.size(); ++i) {
		for (auto const padi_bus_block : iter_all<PADIBusBlockOnDLS>()) {
			PADIBusOnDLS const padi_bus_on_dls(padi_bus, padi_bus_block);
			auto const synapse_drivers = num_synapse_drivers[padi_bus_on_dls].at(i);
			allocation_requests.at(i).shapes[padi_bus_on_dls] = {
			    SynapseDriverOnDLSManager::AllocationRequest::Shape{synapse_drivers, false}};
		}
	}

	Label const fixture(padi_bus.value() << 2 | backend_block.value() << 4);
	std::vector<int64_t> label_space(filter.size());
	std::fill(label_space.begin(), label_space.end(), 4);
	for (auto it = hate::MultidimIterator(label_space); it != it.end(); ++it) {
		auto const labels = *it;
		std::set<int64_t> unique_labels(labels.begin(), labels.end());
		if (unique_labels.size() != labels.size()) {
			continue;
		}
		for (size_t i = 0; i < labels.size(); ++i) {
			allocation_requests.at(i).labels.push_back(Label(fixture + labels.at(i)));
		}
	}
	return allocation_requests;
}

std::vector<SynapseDriverOnDLSManager::AllocationRequest>
SourceOnPADIBusManager::get_allocation_requests_background(
    std::vector<std::vector<size_t>> const& filter,
    halco::hicann_dls::vx::v3::PADIBusOnDLS const& padi_bus,
    std::vector<size_t> const& num_synapse_drivers)
{
	std::vector<SynapseDriverOnDLSManager::AllocationRequest> allocation_requests(filter.size());
	for (size_t i = 0; i < filter.size(); ++i) {
		auto const synapse_drivers = num_synapse_drivers.at(i);
		allocation_requests.at(i).shapes[padi_bus] = {
		    SynapseDriverOnDLSManager::AllocationRequest::Shape{synapse_drivers, false}};
	}

	if (filter.size() == 1) {
		for (auto const label : iter_all<Label>()) {
			allocation_requests.at(0).labels.push_back(label);
		}
	} else if (filter.size() == 2) {
		for (size_t i = 0; i < Label::size / 2; ++i) {
			allocation_requests.at(0).labels.push_back(Label(i * 2 + 0));
			allocation_requests.at(0).labels.push_back(Label(i * 2 + 1));
			allocation_requests.at(1).labels.push_back(Label(i * 2 + 1));
			allocation_requests.at(1).labels.push_back(Label(i * 2 + 0));
		}
	} else if (filter.size() == 4) {
		for (size_t i = 0; i < Label::size / 4; ++i) {
			std::vector<int64_t> label_space(filter.size());
			std::fill(label_space.begin(), label_space.end(), 4);
			for (auto it = hate::MultidimIterator(label_space); it != it.end(); ++it) {
				auto const labels = *it;
				std::set<int64_t> unique_labels(labels.begin(), labels.end());
				if (unique_labels.size() != labels.size()) {
					continue;
				}
				for (size_t j = 0; j < labels.size(); ++j) {
					allocation_requests.at(j).labels.push_back(Label(labels.at(j) + i * 4));
				}
			}
		}
	} else {
		throw std::runtime_error("Unexpected number of splits for background sources.");
	}
	return allocation_requests;
}

SynapseDriverOnDLSManager::AllocationRequest
SourceOnPADIBusManager::get_allocation_requests_external(
    halco::hicann_dls::vx::v3::PADIBusOnDLS const& padi_bus, size_t num_synapse_drivers)
{
	SynapseDriverOnDLSManager::AllocationRequest allocation_requests;
	allocation_requests.shapes[padi_bus] = {
	    SynapseDriverOnDLSManager::AllocationRequest::Shape{num_synapse_drivers, false}};
	for (auto const label : iter_all<Label>()) {
		allocation_requests.labels.push_back(label);
	}
	return allocation_requests;
}

std::optional<halco::common::typed_array<
    std::vector<std::vector<size_t>>,
    halco::hicann_dls::vx::v3::PADIBusOnDLS>>
SourceOnPADIBusManager::distribute_external_sources_linear(
    std::vector<ExternalSource> const& sources,
    halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::PADIBusOnDLS> const&
        used_num_synapse_drivers)
{
	typed_array<std::vector<std::vector<size_t>>, PADIBusOnDLS> split_external_sources_per_padi_bus;
	for (auto const padi_bus_block : iter_all<PADIBusBlockOnDLS>()) {
		size_t i = 0;
		for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
			std::vector<size_t> filter;
			size_t unused_num_synapse_drivers =
			    SynapseDriverOnPADIBus::size -
			    used_num_synapse_drivers[PADIBusOnDLS(padi_bus, padi_bus_block)];
			while (i < sources.size() && get_num_synapse_drivers(sources, filter)[padi_bus_block] <=
			                                 unused_num_synapse_drivers) {
				if (filter.size() == lola::vx::v3::SynapseMatrix::Label::size) {
					unused_num_synapse_drivers -=
					    get_num_synapse_drivers(sources, filter)[padi_bus_block];
					split_external_sources_per_padi_bus[PADIBusOnDLS(padi_bus, padi_bus_block)]
					    .push_back(filter);
					filter.clear();
				}
				if (get_num_synapse_drivers(sources, {i})[padi_bus_block] > 0) {
					filter.push_back(i);
				}
				i++;
			}
			while (get_num_synapse_drivers(sources, filter)[padi_bus_block] >
			       unused_num_synapse_drivers) {
				filter.pop_back();
				i--;
			}
			if (!filter.empty()) {
				split_external_sources_per_padi_bus[PADIBusOnDLS(padi_bus, padi_bus_block)]
				    .push_back(filter);
			}
		}
		if (i != sources.size()) {
			return std::nullopt;
		}
	}
	return split_external_sources_per_padi_bus;
}

} // namespace grenade::vx::network::placed_atomic::routing::detail
