#include "grenade/vx/network/placed_logical/requires_routing.h"

#include "grenade/vx/network/placed_logical/build_atomic_network.h"
#include "grenade/vx/network/placed_logical/build_connection_routing.h"
#include "grenade/vx/network/placed_logical/network.h"
#include "grenade/vx/network/placed_logical/network_graph.h"

namespace grenade::vx::network::placed_logical {

using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

bool requires_routing(std::shared_ptr<Network> const& current, NetworkGraph const& old_graph)
{
	auto const& old = old_graph.get_network();
	assert(current);
	assert(old_graph.get_network());

	// check if populations changed
	if (current->populations != old->populations) {
		return true;
	}
	// check if projection count changed
	if (current->projections.size() != old->projections.size()) {
		return true;
	}
	// check if projection topology changed
	auto const projection_translation = old_graph.get_projection_translation();
	for (auto const& [descriptor, projection] : current->projections) {
		auto const& old_projection = old->projections.at(descriptor);
		if ((projection.population_pre != old_projection.population_pre) ||
		    (projection.population_post != old_projection.population_post) ||
		    (projection.receptor != old_projection.receptor) ||
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
			// check if weight changed such that it no longer fits into the routed number of
			// hardware synapse circuits
			auto const local_connection_translation_range =
			    projection_translation.equal_range(std::pair{descriptor, i});
			size_t const local_num_hw_synapses = std::distance(
			    local_connection_translation_range.first,
			    local_connection_translation_range.second);
			if (hate::math::round_up_integer_division(
			        connection.weight.value(), lola::vx::v3::SynapseMatrix::Weight::max) >
			    local_num_hw_synapses) {
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
	if ((static_cast<bool>(current->cadc_recording) != static_cast<bool>(old->cadc_recording)) ||
	    (static_cast<bool>(current->cadc_recording) &&
	     (current->cadc_recording != old->cadc_recording))) {
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

} // namespace grenade::vx::network::placed_logical
