#include "grenade/vx/network/placed_atomic/detail/synapse_driver_on_padi_bus_manager.h"

#include "halco/common/iter_all.h"
#include "hate/timer.h"
#include <algorithm>
#include <set>

namespace grenade::vx::network::placed_atomic::detail {

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;

bool SynapseDriverOnPADIBusManager::forwards(
    Label const& label, Mask const& mask, SynapseDriver const& synapse_driver)
{
	return ((label.value() & mask.value()) == (synapse_driver.value() & mask.value()));
}

bool SynapseDriverOnPADIBusManager::has_unique_labels(
    std::vector<AllocationRequest> const& requested_allocations)
{
	std::set<Label> labels;
	for (auto const& requested_allocation : requested_allocations) {
		labels.insert(requested_allocation.label);
	}
	return (labels.size() == requested_allocations.size());
}

bool SynapseDriverOnPADIBusManager::allocations_fit_available_size(
    std::set<SynapseDriver> const& unavailable_synapse_drivers,
    std::vector<AllocationRequest> const& requested_allocations)
{
	size_t requested_size = 0;
	for (auto const& request : requested_allocations) {
		requested_size += request.size();
	}
	return requested_size <= (SynapseDriver::size - unavailable_synapse_drivers.size());
}

SynapseDriverOnPADIBusManager::IsolatingMasks
SynapseDriverOnPADIBusManager::generate_isolating_masks(
    std::vector<AllocationRequest> const& requested_allocations)
{
	IsolatingMasks isolating_masks;
	for (auto const mask : iter_all<Mask>()) {
		for (size_t i = 0; i < requested_allocations.size(); ++i) {
			bool is_isolated = true;
			auto const label_i = requested_allocations.at(i).label;
			auto const masked_i = mask & label_i;
			for (size_t j = 0; j < requested_allocations.size(); ++j) {
				if (j != i) {
					auto const label_j = requested_allocations.at(j).label;
					is_isolated = is_isolated && ((mask & label_j) != masked_i);
				}
			}
			if (is_isolated) {
				isolating_masks[i].push_back(mask);
			}
		}
	}
	return isolating_masks;
}

bool SynapseDriverOnPADIBusManager::allocations_can_be_isolated(
    IsolatingMasks const& isolating_masks,
    std::vector<AllocationRequest> const& requested_allocations)
{
	return (isolating_masks.size() == requested_allocations.size());
}

SynapseDriverOnPADIBusManager::IsolatedSynapseDrivers
SynapseDriverOnPADIBusManager::generate_isolated_synapse_drivers(
    std::set<SynapseDriver> const& unavailable_synapse_drivers,
    IsolatingMasks const& isolating_masks,
    std::vector<AllocationRequest> const& requested_allocations)
{
	IsolatedSynapseDrivers isolated_synapse_drivers;
	for (auto const synapse_driver : iter_all<SynapseDriverOnPADIBus>()) {
		if (unavailable_synapse_drivers.contains(synapse_driver)) {
			continue;
		}
		for (auto const& [i, masks] : isolating_masks) {
			for (auto const mask : masks) {
				if (forwards(requested_allocations.at(i).label, mask, synapse_driver)) {
					isolated_synapse_drivers[synapse_driver].push_back(std::make_pair(i, mask));
					// we only need one matching mask
					break;
				}
			}
		}
	}
	return isolated_synapse_drivers;
}

bool SynapseDriverOnPADIBusManager::allocations_can_be_placed_individually(
    IsolatedSynapseDrivers const& isolated_synapse_drivers,
    std::vector<AllocationRequest> const& requested_allocations)
{
	size_t j = 0;
	for (auto const& requested_allocation : requested_allocations) {
		for (auto const& shape : requested_allocation.shapes) {
			if (shape.contiguous) {
				bool found = false;
				size_t possible_size = 0;
				std::optional<SynapseDriver> last;
				for (auto const& [synapse_driver, indices] : isolated_synapse_drivers) {
					for (auto const& [i, _] : indices) {
						if (i == j) {
							if (last) {
								if (*last + 1 != synapse_driver) {
									possible_size = 0;
								}
							}
							last.emplace(synapse_driver);
							possible_size++;
							break;
						}
					}
					if (possible_size >= shape.size) {
						found = true;
						break;
					}
				}
				if (!found) {
					return false;
				}
			} else {
				size_t possible_size = 0;
				for (auto const& [synapse_driver, indices] : isolated_synapse_drivers) {
					for (auto const& [i, _] : indices) {
						if (i == j) {
							possible_size++;
							break;
						}
					}
				}
				if (possible_size < shape.size) {
					return false;
				}
			}
		}
		j++;
	}
	return true;
}

std::vector<SynapseDriverOnPADIBusManager::Allocation>
SynapseDriverOnPADIBusManager::allocate_greedy(
    std::vector<AllocationRequest> const& requested_allocations,
    IsolatedSynapseDrivers const& isolated_synapse_drivers,
    bool const exclusive_first)
{
	std::vector<Allocation> allocations(requested_allocations.size());
	std::set<SynapseDriverOnPADIBus> unavailable_synapse_drivers;
	auto const impl = [&](bool const exclusive) {
		for (size_t i = 0; i < requested_allocations.size(); ++i) {
			auto& allocation = allocations.at(i);
			allocation.synapse_drivers.resize(requested_allocations.at(i).shapes.size());
			size_t j = 0;
			for (auto const& shape : requested_allocations.at(i).shapes) {
				for (auto const& [synapse_driver, indices] : isolated_synapse_drivers) {
					if (allocation.synapse_drivers.at(j).size() == shape.size) {
						break;
					}
					if (unavailable_synapse_drivers.contains(synapse_driver)) {
						continue;
					}
					if (exclusive && (indices.size() != 1)) {
						continue;
					}
					auto const match = std::find_if(
					    indices.begin(), indices.end(),
					    [i](auto const& v) { return v.first == i; });
					if (match == indices.end()) {
						continue;
					}
					allocation.synapse_drivers.at(j).push_back(
					    std::pair{synapse_driver, match->second});
					unavailable_synapse_drivers.insert(synapse_driver);
				}
				j++;
			}
		}
	};
	if (exclusive_first) {
		impl(true);
	}
	impl(false);
	return allocations;
}

std::vector<SynapseDriverOnPADIBusManager::Allocation>
SynapseDriverOnPADIBusManager::allocate_backtracking(
    std::vector<AllocationRequest> const& requested_allocations,
    IsolatedSynapseDrivers const& isolated_synapse_drivers,
    std::optional<std::chrono::milliseconds> const& max_duration)
{
	typedef std::pair<size_t, std::array<std::optional<size_t>, SynapseDriver::size>> Candidate;

	auto const root = []() { return Candidate(); };

	std::map<size_t, std::vector<size_t>> reduced_isolated_synapse_drivers;
	std::vector<std::pair<size_t, size_t>> flat_isolated_synapse_drivers;
	for (auto const& [synapse_driver, indices] : isolated_synapse_drivers) {
		for (auto const& [i, _] : indices) {
			reduced_isolated_synapse_drivers[synapse_driver].push_back(i);
			flat_isolated_synapse_drivers.push_back(
			    std::make_pair(static_cast<size_t>(synapse_driver), i));
		}
	}

	std::vector<size_t> requested_size(requested_allocations.size());
	for (size_t i = 0; i < requested_allocations.size(); ++i) {
		requested_size[i] = requested_allocations[i].size();
	}
	std::vector<size_t> accept_synapse_drivers(requested_allocations.size());

	auto const accept = [requested_size, &accept_synapse_drivers](Candidate const& candidate) {
		auto& synapse_drivers = accept_synapse_drivers;
		std::fill(synapse_drivers.begin(), synapse_drivers.end(), 0);
		for (size_t s = 0; s < candidate.first; ++s) {
			if (candidate.second[s]) {
				auto const i = *(candidate.second[s]);
				synapse_drivers[i]++;
			}
		}
		for (size_t i = 0; i < requested_size.size(); ++i) {
			if (synapse_drivers[i] != requested_size[i]) {
				return false;
			}
		}
		return true;
	};

	std::vector<std::vector<size_t>> reject_synapse_drivers(requested_allocations.size());
	for (auto& v : reject_synapse_drivers) {
		v.reserve(SynapseDriver::size);
	}
	std::vector<std::vector<std::vector<size_t>>> reject_synapse_drivers_per_shape(
	    requested_allocations.size());
	for (size_t i = 0; i < requested_allocations.size(); ++i) {
		reject_synapse_drivers_per_shape[i].resize(requested_allocations[i].shapes.size());
		for (size_t j = 0; j < requested_allocations[i].shapes.size(); ++j) {
			reject_synapse_drivers_per_shape[i][j].reserve(requested_allocations[i].shapes[j].size);
		}
	}
	std::vector<std::vector<size_t>> reject_open_positions(SynapseDriver::size);
	for (auto& op : reject_open_positions) {
		op.resize(requested_allocations.size());
	}
	for (size_t i = 0; i < SynapseDriver::size; ++i) {
		for (auto const& [synapse_driver, index] : flat_isolated_synapse_drivers) {
			if (synapse_driver < i) {
				continue;
			}
			reject_open_positions[i][index]++;
		}
	}

	auto const reject = [reject_open_positions, &reject_synapse_drivers,
	                     &reject_synapse_drivers_per_shape, requested_allocations,
	                     flat_isolated_synapse_drivers](Candidate const& candidate) {
		for (auto& v : reject_synapse_drivers) {
			v.clear();
		}
		for (size_t s = 0; s < candidate.first; ++s) {
			if (candidate.second[s]) {
				auto const i = *(candidate.second[s]);
				reject_synapse_drivers[i].push_back(s);
			}
		}
		// check if we allocated too much for any requested allocation
		for (size_t i = 0; i < reject_synapse_drivers.size(); ++i) {
			if (reject_synapse_drivers[i].size() > requested_allocations[i].size()) {
				return true;
			}
		}
		// check if an allocation requested to be contiguous is not contiguous
		for (size_t i = 0; i < reject_synapse_drivers.size(); ++i) {
			auto const is_contiguous = [](auto const& synapse_drivers) {
				return synapse_drivers.empty() ||
				       (*synapse_drivers.rbegin() - *synapse_drivers.begin()) ==
				           synapse_drivers.size() - 1;
			};
			size_t k = 0;
			size_t k_size = 0;
			for (auto const& shape : requested_allocations[i].shapes) {
				if (shape.contiguous) {
					auto& local_synapse_drivers = reject_synapse_drivers_per_shape[i][k];
					local_synapse_drivers.clear();
					if (reject_synapse_drivers[i].size() < k_size) {
						break;
					}
					for (size_t kk = 0;
					     kk < std::min(shape.size, reject_synapse_drivers[i].size() - k_size);
					     ++kk) {
						local_synapse_drivers.push_back(reject_synapse_drivers[i][k_size + kk]);
					}
					if (!is_contiguous(local_synapse_drivers)) {
						return true;
					}
				}
				k++;
				k_size += shape.size;
			}
		}
		// check whether the current candidate has any change of being accepted in the future
		if (candidate.first < SynapseDriver::size) {
			auto const& open_positions = reject_open_positions[candidate.first];
			for (size_t i = 0; i < reject_synapse_drivers.size(); ++i) {
				if (requested_allocations[i].size() >
				    reject_synapse_drivers[i].size() + open_positions[i]) {
					return true;
				}
			}
		}
		return false;
	};

	std::vector<Allocation> allocations;
	bool run = true;

	auto const output = [&](Candidate const& candidate) {
		allocations.resize(requested_allocations.size());
		for (size_t i = 0; i < allocations.size(); ++i) {
			allocations.at(i).synapse_drivers.resize(requested_allocations.at(i).shapes.size());
		}
		std::vector<size_t> ii(allocations.size());
		for (size_t s = 0; s < candidate.first; ++s) {
			SynapseDriver synapse_driver(s);
			if (candidate.second[s]) {
				auto const i = *(candidate.second[s]);
				auto const& isd = isolated_synapse_drivers.at(synapse_driver);
				allocations[i].synapse_drivers.at(ii.at(i)).push_back(std::pair{
				    synapse_driver, std::find_if(isd.begin(), isd.end(), [i](auto const& v) {
					                    return i == v.first;
				                    })->second});
				if (allocations[i].synapse_drivers.at(ii.at(i)).size() ==
				    requested_allocations[i].shapes.at(ii.at(i)).size) {
					ii.at(i)++;
				}
			}
		}
		run = false;
	};

	auto const first = [reduced_isolated_synapse_drivers](Candidate& candidate) {
		if (candidate.first == SynapseDriver::size) {
			return false;
		}
		while (!reduced_isolated_synapse_drivers.contains(candidate.first)) {
			candidate.first++;
			if (candidate.first == SynapseDriver::size) {
				return false;
			}
		}
		auto const& isd = reduced_isolated_synapse_drivers.at(candidate.first);
		if (isd.empty()) {
			candidate.second[candidate.first] = std::nullopt;
		} else {
			candidate.second[candidate.first].emplace(isd.front());
		}
		candidate.first++;
		return true;
	};

	auto const next = [reduced_isolated_synapse_drivers](Candidate& candidate) {
		assert(candidate.first != 0);
		auto const synapse_driver = candidate.first - 1;
		auto const& isd = reduced_isolated_synapse_drivers.at(synapse_driver);
		auto& local_candidate = candidate.second[synapse_driver];
		if (!local_candidate) {
			return false;
		}
		auto c = std::find_if(isd.begin(), isd.end(), [local_candidate](auto const& v) {
			return v == *local_candidate;
		});
		if (std::distance(c, isd.end()) == 1) {
			local_candidate = std::nullopt;
		} else {
			assert(c != isd.end());
			*local_candidate = *(c + 1);
		}
		return true;
	};

	// hand-tuned magic value large enough as to minimize impact of time measurements
	constexpr size_t max_duration_check_divisor = 10000;
	size_t iteration_count = 0;
	hate::Timer timer;

	auto const backtrack = [&](auto&& backtrack, Candidate const& candidate) {
		if (max_duration && ((iteration_count % max_duration_check_divisor) == 0) &&
		    (timer.get_ms() >= max_duration->count())) {
			run = false;
		}
		if (!run) {
			return;
		}
		if (reject(candidate)) {
			return;
		}
		if (accept(candidate)) {
			output(candidate);
		}
		Candidate s = candidate;
		bool r = first(s);
		while (r) {
			backtrack(backtrack, s);
			r = next(s);
		}
	};

	backtrack(backtrack, root());

	return allocations;
}

bool SynapseDriverOnPADIBusManager::valid(
    std::vector<Allocation> const& allocations,
    std::vector<AllocationRequest> const& requested_allocations)
{
	if (allocations.size() != requested_allocations.size()) {
		return false;
	}
	std::set<SynapseDriver> used_synapse_drivers;
	for (size_t i = 0; i < requested_allocations.size(); ++i) {
		auto const& allocation = allocations.at(i);
		if (allocation.synapse_drivers.size() != requested_allocations.at(i).shapes.size()) {
			return false;
		}
		for (size_t j = 0; j < requested_allocations.at(i).shapes.size(); ++j) {
			std::set<SynapseDriver> synapse_drivers;
			for (auto const& [synapse_driver, _] : allocation.synapse_drivers.at(j)) {
				synapse_drivers.insert(synapse_driver);
			}
			// non-unique synapse drivers for one shape
			if (synapse_drivers.size() != allocation.synapse_drivers.at(j).size()) {
				return false;
			}
			// contiguous requested, but not contiguous
			if (requested_allocations.at(i).shapes.at(j).contiguous &&
			    !is_contiguous(synapse_drivers)) {
				return false;
			}
			// size different than requested
			if (requested_allocations.at(i).shapes.at(j).size != synapse_drivers.size()) {
				return false;
			}
			// non-unique synapse drivers for different shapes or requested allocations
			auto const size = used_synapse_drivers.size();
			used_synapse_drivers.insert(synapse_drivers.begin(), synapse_drivers.end());
			if (used_synapse_drivers.size() - size != allocation.synapse_drivers.at(j).size()) {
				return false;
			}
		}
	}
	return true;
}

bool SynapseDriverOnPADIBusManager::is_contiguous(std::set<SynapseDriver> const& synapse_drivers)
{
	if (synapse_drivers.empty()) {
		return true;
	}
	return (*synapse_drivers.rbegin() - *synapse_drivers.begin()) == synapse_drivers.size() - 1;
}

} // namespace grenade::vx::network::placed_atomic::detail
