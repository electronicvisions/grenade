#include "grenade/vx/network/detail/synapse_driver_on_dls_manager.h"

#include "grenade/vx/network/detail/synapse_driver_on_padi_bus_manager.h"
#include "halco/common/iter_all.h"
#include <log4cxx/logger.h>

namespace grenade::vx::network::detail {

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;

std::set<std::set<PADIBusOnDLS>> SynapseDriverOnDLSManager::get_interdependent_padi_busses(
    std::vector<AllocationRequest> const& requested_allocations)
{
	std::set<std::set<PADIBusOnDLS>> dependencies;
	for (auto const& requested_allocation : requested_allocations) {
		if (requested_allocation.labels.size() > 1) {
			// Allocation requests with label space larger one lead to interdependencies, because
			// the label space needs to be explored for the PADI-busses depending on each other. If
			// any of the newly depending-on PADI-busses are already present in another collection,
			// the collections need to be merged.

			// Get set of PADI-busses of requested allocation.
			std::set<PADIBusOnDLS> padi_busses;
			for (auto const& [padi_bus, _] : requested_allocation.shapes) {
				padi_busses.insert(padi_bus);
			}
			// For each already existing interdependent collection of PADI-busses check if it
			// contains any of the ones from the current requested allocation. If yes, add to
			// merged_dependency, if no leave unchanged.
			std::set<PADIBusOnDLS> merged_dependency;
			std::vector<std::reference_wrapper<std::set<PADIBusOnDLS> const>>
			    merged_dependency_references;
			for (auto const& dependency : dependencies) {
				if (std::find_if(
				        dependency.begin(), dependency.end(), [padi_busses](auto const& p) {
					        return padi_busses.contains(p);
				        }) != dependency.end()) {
					merged_dependency.insert(dependency.begin(), dependency.end());
					merged_dependency_references.push_back(dependency);
				}
			}
			// Remove merged dependencies from collection of interdependent collections.
			for (auto const& ref : merged_dependency_references) {
				dependencies.erase(ref.get());
			}
			// Add (remaining) interdependent PADI-busses of the current requested allocation.
			merged_dependency.insert(padi_busses.begin(), padi_busses.end());
			dependencies.insert(merged_dependency);
		} else {
			// Allocation requests with only one label possibility to choose from don't lead to
			// interdependencies because there is no label space to explore. Therefore we add their
			// PADI-busses as independent collections if they are not present yet.
			for (auto const& [padi_bus, _] : requested_allocation.shapes) {
				if (std::none_of(
				        dependencies.begin(), dependencies.end(),
				        [padi_bus](auto const& ps) { return ps.contains(padi_bus); })) {
					dependencies.insert({padi_bus});
				}
			}
		}
	}
	return dependencies;
}

SynapseDriverOnDLSManager::AllocationRequestPerPADIBus
SynapseDriverOnDLSManager::get_requested_allocations_per_padi_bus(
    std::vector<AllocationRequest> const& requested_allocations)
{
	AllocationRequestPerPADIBus allocation_properties;
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		for (size_t i = 0; i < requested_allocations.size(); ++i) {
			auto const& request = requested_allocations.at(i);
			for (auto const& [q, shapes] : request.shapes) {
				if (q == padi_bus) {
					SynapseDriverOnPADIBusManager::AllocationRequest ps{shapes, Label()};
					allocation_properties[padi_bus].first.push_back(ps);
					allocation_properties[padi_bus].second.push_back(i);
				}
			}
		}
	}
	return allocation_properties;
}

std::vector<size_t> SynapseDriverOnDLSManager::get_independent_allocation_requests(
    std::vector<AllocationRequest> const& requested_allocations,
    std::set<PADIBusOnDLS> const& padi_busses)
{
	std::vector<size_t> independent_requests;
	size_t i = 0;
	for (auto const& requested_allocation : requested_allocations) {
		bool const has_matching_padi_bus = std::any_of(
		    requested_allocation.shapes.begin(), requested_allocation.shapes.end(),
		    [padi_busses](auto const& p) { return padi_busses.contains(p.first); });
		if (has_matching_padi_bus && !requested_allocation.dependent_label_group) {
			independent_requests.push_back(i);
		}
		i++;
	}
	return independent_requests;
}

std::vector<SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup>
SynapseDriverOnDLSManager::get_unique_dependent_label_groups(
    std::vector<AllocationRequest> const& requested_allocations,
    std::set<PADIBusOnDLS> const& padi_busses)
{
	std::set<AllocationRequest::DependentLabelGroup> unique_dependent_label_groups_set;
	for (auto const& requested_allocation : requested_allocations) {
		bool const has_matching_padi_bus = std::any_of(
		    requested_allocation.shapes.begin(), requested_allocation.shapes.end(),
		    [padi_busses](auto const& p) { return padi_busses.contains(p.first); });
		if (has_matching_padi_bus && requested_allocation.dependent_label_group) {
			unique_dependent_label_groups_set.insert(*requested_allocation.dependent_label_group);
		}
	}
	std::vector<AllocationRequest::DependentLabelGroup> unique_dependent_label_groups;
	unique_dependent_label_groups.insert(
	    unique_dependent_label_groups.end(), unique_dependent_label_groups_set.begin(),
	    unique_dependent_label_groups_set.end());
	return unique_dependent_label_groups;
}

std::vector<int64_t> SynapseDriverOnDLSManager::get_label_space(
    std::vector<size_t> const& independent_allocation_requests,
    std::vector<AllocationRequest::DependentLabelGroup> const& unique_dependent_label_groups,
    std::vector<AllocationRequest> const& requested_allocations)
{
	std::vector<int64_t> label_space(
	    independent_allocation_requests.size() + unique_dependent_label_groups.size());
	// first place independent allocation requests
	for (size_t i = 0; i < independent_allocation_requests.size(); ++i) {
		label_space.at(i) =
		    requested_allocations.at(independent_allocation_requests.at(i)).labels.size();
	}
	// then place dependent label groups
	for (size_t i = 0; i < unique_dependent_label_groups.size(); ++i) {
		// use first allocation request in dependent label group to get labels size
		auto const it = std::find_if(
		    requested_allocations.begin(), requested_allocations.end(),
		    [i, unique_dependent_label_groups](auto const& ra) {
			    return ra.dependent_label_group &&
			           (*ra.dependent_label_group == unique_dependent_label_groups.at(i));
		    });
		assert(it != requested_allocations.end());
		label_space.at(independent_allocation_requests.size() + i) = it->labels.size();
	}
	return label_space;
}

size_t SynapseDriverOnDLSManager::get_label_space_index(
    std::vector<size_t> const& independent_allocation_requests,
    std::vector<AllocationRequest::DependentLabelGroup> const& unique_dependent_label_groups,
    std::vector<AllocationRequest> const& requested_allocations,
    size_t const index)
{
	auto const& dependent_label_group = requested_allocations.at(index).dependent_label_group;
	if (dependent_label_group) {
		return independent_allocation_requests.size() +
		       std::distance(
		           unique_dependent_label_groups.begin(),
		           std::find_if(
		               unique_dependent_label_groups.begin(), unique_dependent_label_groups.end(),
		               [dependent_label_group](auto const& in) {
			               return in == *dependent_label_group;
		               }));
	} else {
		return std::distance(
		    independent_allocation_requests.begin(),
		    std::find_if(
		        independent_allocation_requests.begin(), independent_allocation_requests.end(),
		        [index](auto const& in) { return in == index; }));
	}
}

void SynapseDriverOnDLSManager::update_labels(
    AllocationRequestPerPADIBus::mapped_type& requested_allocation,
    std::vector<AllocationRequest> const& requested_allocations,
    std::vector<int64_t> const& label_indices,
    std::vector<size_t> const& independent_allocation_requests,
    std::vector<AllocationRequest::DependentLabelGroup> const& unique_dependent_label_groups)
{
	auto const& local_indices = requested_allocation.second;
	for (size_t i = 0; i < local_indices.size(); ++i) {
		auto const& index = local_indices.at(i);
		requested_allocation.first.at(i).label =
		    requested_allocations.at(index).labels.at(label_indices.at(get_label_space_index(
		        independent_allocation_requests, unique_dependent_label_groups,
		        requested_allocations, index)));
	}
}

void SynapseDriverOnDLSManager::update_solution(
    std::vector<Allocation>& allocation,
    std::vector<SynapseDriverOnPADIBusManager::Allocation> const& local_allocations,
    AllocationRequestPerPADIBus::mapped_type const& requested_allocation,
    halco::hicann_dls::vx::v3::PADIBusOnDLS const& padi_bus,
    std::vector<AllocationRequest> const& requested_allocations,
    std::vector<int64_t> const& label_indices,
    std::vector<size_t> const& independent_allocation_requests,
    std::vector<AllocationRequest::DependentLabelGroup> const& unique_dependent_label_groups)
{
	auto const& local_indices = requested_allocation.second;
	for (size_t i = 0; i < local_indices.size(); ++i) {
		auto const& index = local_indices.at(i);
		auto& independent_allocation = allocation.at(index);
		independent_allocation.synapse_drivers[padi_bus] = local_allocations.at(i);
		independent_allocation.label =
		    requested_allocations.at(index).labels.at(label_indices.at(get_label_space_index(
		        independent_allocation_requests, unique_dependent_label_groups,
		        requested_allocations, index)));
	}
}

bool SynapseDriverOnDLSManager::valid_solution(
    std::vector<Allocation> const& allocations,
    std::vector<AllocationRequest> const& requested_allocations)
{
	auto logger = log4cxx::Logger::getLogger("grenade.SynapseDriverOnDLSManager");

	// check equal size
	if (allocations.size() != requested_allocations.size()) {
		LOG4CXX_ERROR(
		    logger, "valid_solution(): Solution size does not match requested allocation size.");
		return false;
	}
	for (size_t i = 0; i < allocations.size(); ++i) {
		auto const& allocation = allocations.at(i);
		auto const& requested_allocation = requested_allocations.at(i);
		// check label of allocation is a possible label in allocation request
		if (std::find(
		        requested_allocation.labels.begin(), requested_allocation.labels.end(),
		        allocation.label) == requested_allocation.labels.end()) {
			LOG4CXX_ERROR(
			    logger, "valid_solution(): Label("
			                << allocation.label << ") present in solution(" << i
			                << ") is not present in corresponding requested allocation.");
			return false;
		}
		// check that allocated PADI-busses are of same size
		if (allocation.synapse_drivers.size() != requested_allocation.shapes.size()) {
			LOG4CXX_ERROR(
			    logger, "valid_solution(): Number of PADI-busses in solution("
			                << i
			                << ") does not match number of PADI-busses in corresponding requested "
			                   "allocation.");
			return false;
		}
		// check that PADI-bus collection is equal
		for (auto const& [padi_bus, _] : allocation.synapse_drivers) {
			if (!requested_allocation.shapes.contains(padi_bus)) {
				LOG4CXX_ERROR(
				    logger, "valid_solution(): " << padi_bus << " present in solution(" << i
				                                 << ") is not present in requested allocation.");
				return false;
			}
		}
		for (auto const& [padi_bus, shapes] : requested_allocation.shapes) {
			// check that all shapes are allocated
			if (shapes.size() != allocation.synapse_drivers.at(padi_bus).synapse_drivers.size()) {
				LOG4CXX_ERROR(
				    logger, "valid_solution(): Not all shapes requested are allocated for solution("
				                << padi_bus << ": " << i << ").");
				return false;
			}
			for (size_t j = 0; j < shapes.size(); ++j) {
				// check that the size of all shapes matches
				if (shapes.at(j).size !=
				    allocation.synapse_drivers.at(padi_bus).synapse_drivers.at(j).size()) {
					LOG4CXX_ERROR(
					    logger, "valid_solution(): Shape of requested allocation("
					                << padi_bus << ": " << i << ", " << j
					                << ") is allocated in a different size.")
					return false;
				}
				// check that all shapes which shall be contiguous are contiguous
				if (shapes.at(j).contiguous) {
					std::set<SynapseDriver> synapse_drivers;
					for (auto const& [synapse_driver, _] :
					     allocation.synapse_drivers.at(padi_bus).synapse_drivers.at(j)) {
						synapse_drivers.insert(synapse_driver);
					}
					if (!SynapseDriverOnPADIBusManager::is_contiguous(synapse_drivers)) {
						LOG4CXX_ERROR(
						    logger, "valid_solution(): Shape of requested allocation("
						                << padi_bus << ": " << i << ", " << j
						                << ") is requested to be contiguous but is allocated "
						                   "non-contiguous.")
						return false;
					}
				}
			}
		}
	}
	return true;
}

} // namespace grenade::vx::network::detail
