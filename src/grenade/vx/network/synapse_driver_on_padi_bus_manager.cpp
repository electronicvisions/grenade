#include "grenade/vx/network/synapse_driver_on_padi_bus_manager.h"

#include "grenade/vx/network/detail/synapse_driver_on_padi_bus_manager.h"
#include "hate/timer.h"
#include "hate/variant.h"
#include <ostream>
#include <stdexcept>

namespace grenade::vx::network {

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;

bool SynapseDriverOnPADIBusManager::AllocationRequest::Shape::operator==(Shape const& other) const
{
	return size == other.size && contiguous == other.contiguous;
}

bool SynapseDriverOnPADIBusManager::AllocationRequest::Shape::operator!=(Shape const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(
    std::ostream& os, SynapseDriverOnPADIBusManager::AllocationRequest::Shape const& value)
{
	std::stringstream ss;
	ss << "Shape(size: " << value.size << ", contiguous: " << std::boolalpha << value.contiguous
	   << ")";
	os << ss.str();
	return os;
}


size_t SynapseDriverOnPADIBusManager::AllocationRequest::size() const
{
	size_t s = 0;
	for (auto const& shape : shapes) {
		s += shape.size;
	}
	return s;
}

bool SynapseDriverOnPADIBusManager::AllocationRequest::is_sensitive_for_shape_allocation_order()
    const
{
	// check if any shape shall be contiguous
	if (std::none_of(
	        shapes.begin(), shapes.end(), [](Shape const& shape) { return shape.contiguous; })) {
		return false;
	}
	// check if shapes are all equal
	if (shapes.empty()) {
		return false;
	}
	auto const first = shapes.front();
	if (std::any_of(
	        shapes.begin(), shapes.end(), [first](Shape const& shape) { return shape != first; })) {
		return true;
	}
	return false;
}

bool SynapseDriverOnPADIBusManager::AllocationRequest::operator==(
    AllocationRequest const& other) const
{
	return shapes == other.shapes && label == other.label;
}

bool SynapseDriverOnPADIBusManager::AllocationRequest::operator!=(
    AllocationRequest const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(
    std::ostream& os, SynapseDriverOnPADIBusManager::AllocationRequest const& request)
{
	os << "AllocationRequest(\n";
	os << "\tlabel: " << request.label << "\n";
	os << "\tshapes:\n";
	for (auto const& shape : request.shapes) {
		os << "\t\t" << shape << "\n";
	}
	os << ")";
	return os;
}


std::ostream& operator<<(
    std::ostream& os, SynapseDriverOnPADIBusManager::Allocation const& allocation)
{
	os << "Allocation(\n";
	for (auto const& synapse_drivers : allocation.synapse_drivers) {
		for (auto const& [synapse_driver, mask] : synapse_drivers) {
			os << "\t" << synapse_driver << ": " << mask << "\n";
		}
	}
	os << ")";
	return os;
}


SynapseDriverOnPADIBusManager::AllocationPolicyGreedy::AllocationPolicyGreedy(
    bool const enable_exclusive_first) :
    enable_exclusive_first(enable_exclusive_first)
{}

SynapseDriverOnPADIBusManager::AllocationPolicyBacktracking::AllocationPolicyBacktracking(
    std::optional<std::chrono::milliseconds> const& max_duration) :
    max_duration(max_duration)
{}


SynapseDriverOnPADIBusManager::SynapseDriverOnPADIBusManager(
    std::set<SynapseDriver> const& unavailable_synapse_drivers) :
    m_unavailable_synapse_drivers(unavailable_synapse_drivers)
{}

std::optional<std::vector<SynapseDriverOnPADIBusManager::Allocation>>
SynapseDriverOnPADIBusManager::solve(
    std::vector<AllocationRequest> const& requested_allocations,
    AllocationPolicy const& allocation_policy)
{
	if (!detail::SynapseDriverOnPADIBusManager::has_unique_labels(requested_allocations)) {
		return std::nullopt;
	}

	if (requested_allocations.empty()) {
		return std::vector<Allocation>{};
	}

	// early abort if not all requested allocations fit available number of synapse drivers
	if (!detail::SynapseDriverOnPADIBusManager::allocations_fit_available_size(
	        m_unavailable_synapse_drivers, requested_allocations)) {
		return std::nullopt;
	}

	auto const isolating_masks =
	    detail::SynapseDriverOnPADIBusManager::generate_isolating_masks(requested_allocations);

	// early abort if not all requested allocations can be isolated
	if (!detail::SynapseDriverOnPADIBusManager::allocations_can_be_isolated(
	        isolating_masks, requested_allocations)) {
		return std::nullopt;
	}

	// calculate synapse drivers which allow isolated placement of an allocation
	auto const isolated_synapse_drivers =
	    detail::SynapseDriverOnPADIBusManager::generate_isolated_synapse_drivers(
	        m_unavailable_synapse_drivers, isolating_masks, requested_allocations);

	// early abort if not all requested allocations can be placed individually
	if (!detail::SynapseDriverOnPADIBusManager::allocations_can_be_placed_individually(
	        isolated_synapse_drivers, requested_allocations)) {
		return std::nullopt;
	}

	// allocate
	auto const allocations = std::visit(
	    hate::overloaded{
	        [requested_allocations,
	         isolated_synapse_drivers](AllocationPolicyGreedy const& policy) {
		        return detail::SynapseDriverOnPADIBusManager::allocate_greedy(
		            requested_allocations, isolated_synapse_drivers, policy.enable_exclusive_first);
	        },
	        [requested_allocations,
	         isolated_synapse_drivers](AllocationPolicyBacktracking const& policy) {
		        return detail::SynapseDriverOnPADIBusManager::allocate_backtracking(
		            requested_allocations, isolated_synapse_drivers, policy.max_duration);
	        },
	        [](auto const&) { throw std::logic_error("Allocation policy not implemented."); },
	    },
	    allocation_policy);

	// check whether generated allocations are valid
	if (!detail::SynapseDriverOnPADIBusManager::valid(allocations, requested_allocations)) {
		return std::nullopt;
	}

	return allocations;
}

std::ostream& operator<<(
    std::ostream& os, SynapseDriverOnPADIBusManager::AllocationPolicy const& value)
{
	std::visit(
	    hate::overloaded{
	        [&os](SynapseDriverOnPADIBusManager::AllocationPolicyGreedy const& policy) {
		        os << "Greedy(enable_exclusive_first: " << std::boolalpha
		           << policy.enable_exclusive_first << ")";
	        },
	        [&os](SynapseDriverOnPADIBusManager::AllocationPolicyBacktracking const& policy) {
		        os << "Backtracking(max_duration: "
		           << (policy.max_duration ? hate::to_string(*policy.max_duration) : "infinite")
		           << ")";
	        },
	        [](auto const&) {
		        throw std::logic_error("AllocationPolicy ostream operator not implemented.");
	        },
	    },
	    value);
	return os;
}

} // namespace grenade::vx::network
