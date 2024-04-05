#include "grenade/vx/network/routing/greedy/synapse_driver_on_dls_manager.h"

#include "grenade/vx/network/routing/greedy/detail/synapse_driver_on_dls_manager.h"
#include "halco/common/iter_all.h"
#include "hate/indent.h"
#include "hate/join.h"
#include "hate/multidim_iterator.h"
#include "hate/timer.h"
#include <numeric>
#include <sstream>
#include <unordered_set>
#include <log4cxx/logger.h>

namespace grenade::vx::network::routing::greedy {

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;

bool SynapseDriverOnDLSManager::AllocationRequest::valid() const
{
	return !labels.empty() && !shapes.empty();
}

bool SynapseDriverOnDLSManager::AllocationRequest::operator==(AllocationRequest const& other) const
{
	return shapes == other.shapes && labels == other.labels &&
	       dependent_label_group == other.dependent_label_group;
}

bool SynapseDriverOnDLSManager::AllocationRequest::operator!=(AllocationRequest const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(
    std::ostream& os, SynapseDriverOnDLSManager::AllocationRequest const& config)
{
	os << "AllocationRequest(\n";
	os << "\tshapes:\n";
	std::stringstream ss;
	for (auto const& [padi_bus, shapes] : config.shapes) {
		for (auto const& shape : shapes) {
			ss << "\t\t" << padi_bus << ": " << shape << "\n";
		}
	}
	os << ss.str();
	os << "\tlabels:\n";
	for (auto const& label : config.labels) {
		os << "\t\t" << label << "\n";
	}
	if (config.dependent_label_group) {
		os << "\tdependent_label_group: " << *(config.dependent_label_group) << "\n";
	} else {
		os << "\tdependent_label_group: none\n";
	}
	os << ")";
	return os;
}


std::ostream& operator<<(std::ostream& os, SynapseDriverOnDLSManager::Allocation const& config)
{
	os << "Allocation(\n";
	os << "\tsynapse_drivers:\n";
	std::stringstream ss;
	for (auto const& [padi_bus, allocation] : config.synapse_drivers) {
		ss << padi_bus << ": " << allocation << "\n";
	}
	os << hate::indent(ss.str(), "\t\t");
	os << "\tlabel: " << config.label << "\n";
	os << ")";
	return os;
}


SynapseDriverOnDLSManager::SynapseDriverOnDLSManager(
    std::set<SynapseDriverOnDLS> const& unavailable_synapse_drivers) :
    m_synapse_driver_on_padi_bus_manager(),
    m_logger(log4cxx::Logger::getLogger("grenade.SynapseDriverOnDLSManager"))
{
	halco::common::typed_array<std::set<SynapseDriver>, PADIBusOnDLS>
	    unavailable_synapse_drivers_per_padi_bus;
	for (auto const& synapse_driver : unavailable_synapse_drivers) {
		unavailable_synapse_drivers_per_padi_bus
		    [PADIBusOnDLS(
		         synapse_driver.toSynapseDriverOnSynapseDriverBlock().toPADIBusOnPADIBusBlock(),
		         synapse_driver.toSynapseDriverBlockOnDLS().toPADIBusBlockOnDLS())]
		        .insert(synapse_driver.toSynapseDriverOnSynapseDriverBlock()
		                    .toSynapseDriverOnPADIBus());
	}
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		m_synapse_driver_on_padi_bus_manager[padi_bus] =
		    SynapseDriverOnPADIBusManager(unavailable_synapse_drivers_per_padi_bus[padi_bus]);
	}
}

std::optional<std::vector<SynapseDriverOnDLSManager::Allocation>> SynapseDriverOnDLSManager::solve(
    std::vector<AllocationRequest> const& requested_allocations,
    SynapseDriverOnPADIBusManager::AllocationPolicy const& allocation_policy,
    std::optional<std::chrono::milliseconds> const& timeout)
{
	// log allocation policy used
	LOG4CXX_DEBUG(
	    m_logger,
	    "solve(): Trying to solve synapse driver allocation requests using AllocationPolicy("
	        << allocation_policy << ").");

	// check that all requested allocations are valid
	for (auto const& request : requested_allocations) {
		if (!request.valid()) {
			throw std::runtime_error(
			    "AllocationRequest with empty labels or shapes not supported.");
		}
	}

	// check that dependent label groups are of homogeneous label size
	{
		std::map<AllocationRequest::DependentLabelGroup, std::set<size_t>> homogeneous;
		for (auto const& request : requested_allocations) {
			if (request.dependent_label_group) {
				homogeneous[*request.dependent_label_group].insert(request.labels.size());
			}
		}
		for (auto const& [group, sizes] : homogeneous) {
			if (sizes.size() != 1) {
				std::stringstream ss;
				ss << group
				   << " is present in allocation requests with different numbers of possible "
				      "labels.";
				throw std::runtime_error(ss.str());
			}
		}
	}

	// get collections of interdependent PADI-bus collections
	auto const dependent_padi_busses =
	    detail::SynapseDriverOnDLSManager::get_interdependent_padi_busses(requested_allocations);

	std::vector<SynapseDriverOnDLSManager::Allocation> solution(requested_allocations.size());

	auto requested_allocations_per_padi_bus =
	    detail::SynapseDriverOnDLSManager::get_requested_allocations_per_padi_bus(
	        requested_allocations);

	// for every collection of interdependent PADI-busses solve synapse driver allocation
	bool solved = true;
	for (auto const& padi_busses : dependent_padi_busses) {
		// log collection of interdependent PADI-busses
		LOG4CXX_DEBUG(
		    m_logger,
		    "solve(): Trying to solve synapse driver allocation requests of interdependent "
		    "PADI-bus(ses):\n"
		        << hate::indent(
		               hate::join_string(padi_busses.begin(), padi_busses.end(), "\n"), "\t")
		        << ".");

		// get unique dependent label groups present on PADI-bus collection
		auto const unique_dependent_label_groups =
		    detail::SynapseDriverOnDLSManager::get_unique_dependent_label_groups(
		        requested_allocations, padi_busses);

		// get requested allocations  on PADI-bus collection without dependent label group
		auto const independent_allocation_requests =
		    detail::SynapseDriverOnDLSManager::get_independent_allocation_requests(
		        requested_allocations, padi_busses);

		// create label space shape by concatenation of the label space of the independent requests
		// and the dependent label groups
		auto const label_space = detail::SynapseDriverOnDLSManager::get_label_space(
		    independent_allocation_requests, unique_dependent_label_groups, requested_allocations);

		// log label space to explore
		LOG4CXX_DEBUG(
		    m_logger, "solve(): Label space to explore:\n\t["
		                  << hate::join(label_space, ", ") << "] (" << label_space.empty() << " "
		                  << (label_space.empty() ? 0
		                                          : std::accumulate(
		                                                label_space.begin(), label_space.end(),
		                                                static_cast<size_t>(1),
		                                                [](auto const& a, auto const& b) {
			                                                return a * static_cast<size_t>(b);
		                                                }))
		                  << " combinations).");

		// for every label combination try to solve synapse driver allocation
		bool solved_for_padi_busses = false;
		hate::MultidimIterator label_space_iterator(label_space);
		auto const label_space_end = label_space_iterator.end();
		size_t explored_label_combinations = 0;
		hate::Timer timer;
		for (; label_space_iterator != label_space_end; ++label_space_iterator) {
			if (std::popcount(explored_label_combinations) == 1) { // pow(2)
				LOG4CXX_DEBUG(
				    m_logger, "solve(): Explored " << explored_label_combinations
				                                   << " label combination(s).");
			}
			if (timeout && (timer.get_ms() >= timeout->count())) {
				break;
			}
			// for every PADI-bus in interdependent collection try to solve synapse driver
			// allocation given the label combination
			auto const labels = *label_space_iterator;
			bool solved_for_padi_bus = true;
			for (auto const padi_bus : padi_busses) {
				// update labels to label combination
				detail::SynapseDriverOnDLSManager::update_labels(
				    requested_allocations_per_padi_bus.at(padi_bus), requested_allocations, labels,
				    independent_allocation_requests, unique_dependent_label_groups);

				// try to solve PADI-bus-local synapse driver allocation
				auto const local_allocations = m_synapse_driver_on_padi_bus_manager[padi_bus].solve(
				    requested_allocations_per_padi_bus.at(padi_bus).first, allocation_policy);
				// if allocation is not successful, it makes no sense to try the remaining
				// PADI-busses, therefore the next label combination is requested directly
				if (!local_allocations) {
					solved_for_padi_bus = false;
					break;
				}
				// update partial solution
				detail::SynapseDriverOnDLSManager::update_solution(
				    solution, *local_allocations, requested_allocations_per_padi_bus.at(padi_bus),
				    padi_bus, requested_allocations, labels, independent_allocation_requests,
				    unique_dependent_label_groups);
			}
			explored_label_combinations++;
			if (solved_for_padi_bus) {
				solved_for_padi_busses = true;
				break;
			}
		}

		// log number of explored label combinations and time spent doing so
		LOG4CXX_DEBUG(
		    m_logger, "solve(): Explored "
		                  << explored_label_combinations << " / "
		                  << (label_space.empty() ? 0
		                                          : std::accumulate(
		                                                label_space.begin(), label_space.end(),
		                                                static_cast<size_t>(1),
		                                                [](auto const& a, auto const& b) {
			                                                return a * static_cast<size_t>(b);
		                                                }))
		                  << " label combination(s) in " << timer.print() << ".");

		if (!solved_for_padi_busses) {
			solved = false;
			break;
		}
	}
	if (solved) {
		if (!detail::SynapseDriverOnDLSManager::valid_solution(solution, requested_allocations)) {
			throw std::logic_error("Allocations are not a valid solution to requests.");
		}
		LOG4CXX_DEBUG(
		    m_logger,
		    "solve(): Found solution:\n"
		        << hate::indent(hate::join_string(solution.begin(), solution.end(), "\n"), "\t")
		        << ".");
		return solution;
	}
	LOG4CXX_DEBUG(m_logger, "solve(): Found no solution.");
	return std::nullopt;
}

} // namespace grenade::vx::network::routing::greedy
