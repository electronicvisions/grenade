#include "grenade/vx/network/placed_atomic/requires_routing.h"

#include "grenade/vx/network/placed_atomic/network.h"


namespace grenade::vx::network::placed_atomic {

using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

bool requires_routing(std::shared_ptr<Network> const& current, std::shared_ptr<Network> const& old)
{
	assert(current);
	assert(old);

	// check if populations changed
	if (current->populations != old->populations) {
		return true;
	}
	// check if projection count changed
	if (current->projections.size() != old->projections.size()) {
		return true;
	}
	// check if projection topology changed
	for (auto const& [descriptor, projection] : current->projections) {
		auto const& old_projection = old->projections.at(descriptor);
		if ((projection.population_pre != old_projection.population_pre) ||
		    (projection.population_post != old_projection.population_post) ||
		    (projection.receptor_type != old_projection.receptor_type) ||
		    (projection.connections.size() != old_projection.connections.size())) {
			return true;
		}
		for (size_t i = 0; i < projection.connections.size(); ++i) {
			auto const& connection = projection.connections.at(i);
			auto const& old_connection = old_projection.connections.at(i);
			if ((connection.index_pre != old_connection.index_pre) ||
			    (connection.index_post != old_connection.index_post)) {
				return true;
			}
		}
	}
	// check if requires one source per row and in order enable value changed
	for (auto const& [descriptor, plasticity_rule] : current->plasticity_rules) {
		auto const& old_plasticity_rule = old->plasticity_rules.at(descriptor);
		if (plasticity_rule.enable_requires_one_source_per_row_in_order !=
		    old_plasticity_rule.enable_requires_one_source_per_row_in_order) {
			return true;
		}
	}
	// check if MADC recording was added or removed
	if (static_cast<bool>(current->madc_recording) != static_cast<bool>(old->madc_recording)) {
		return true;
	}
	// check if CADC recording was changed
	// TODO: Support updating in cases where at least as many neurons are recorded as before
	if (current->cadc_recording != old->cadc_recording) {
		return true;
	}
	// check if plasticity rule count or the recording of a plasticity rule
	// changed. This is sufficient, because the only other thing which can change is the content of
	// the kernel or the timer, which both doesn't require new routing. However when the count
	// changes a new one exists (or an old one was dropped), in both cases the constraints to the
	// routing might have changed or at least we need to regenerate the NetworkGraph later on.
	if (current->plasticity_rules.size() != old->plasticity_rules.size()) {
		return true;
	}
	for (auto const& [descriptor, new_rule] : current->plasticity_rules) {
		if (new_rule.recording != old->plasticity_rules.at(descriptor).recording) {
			return true;
		}
	}
	return false;
}

} // namespace grenade::vx::network::placed_atomic
