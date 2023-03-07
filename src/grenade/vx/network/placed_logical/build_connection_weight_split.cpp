#include "grenade/vx/network/placed_logical/build_connection_weight_split.h"

#include <stdexcept>

namespace grenade::vx::network::placed_logical {

std::vector<placed_atomic::Projection::Connection::Weight> build_connection_weight_split(
    Projection::Connection::Weight const& weight, size_t num)
{
	constexpr placed_atomic::Projection::Connection::Weight max_split_weight(
	    placed_atomic::Projection::Connection::Weight::max);
	size_t const required_num_full = weight.value() / max_split_weight;
	size_t const required_partial = weight.value() % max_split_weight;

	if (required_num_full >= num && required_partial != 0) {
		throw std::overflow_error("Weight too large to be split into requested number.");
	}

	std::vector<placed_atomic::Projection::Connection::Weight> ret;
	ret.reserve(num);
	ret.insert(ret.end(), required_num_full, max_split_weight);
	if (required_partial != 0) {
		ret.insert(ret.end(), placed_atomic::Projection::Connection::Weight(required_partial));
	}
	ret.resize(num, placed_atomic::Projection::Connection::Weight(0));
	return ret;
}

} // namespace grenade::vx::network::placed_logical
