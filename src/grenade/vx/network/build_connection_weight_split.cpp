#include "grenade/vx/network/build_connection_weight_split.h"

#include <stdexcept>

namespace grenade::vx::network {

std::vector<lola::vx::v3::SynapseMatrix::Weight> build_connection_weight_split(
    Projection::Connection::Weight const& weight, size_t num)
{
	constexpr lola::vx::v3::SynapseMatrix::Weight max_split_weight(
	    lola::vx::v3::SynapseMatrix::Weight::max);
	size_t const required_num_full = weight.value() / max_split_weight;
	size_t const required_partial = weight.value() % max_split_weight;

	if (required_num_full >= num && required_partial != 0) {
		throw std::overflow_error("Weight too large to be split into requested number.");
	}

	std::vector<lola::vx::v3::SynapseMatrix::Weight> ret;
	ret.reserve(num);
	ret.insert(ret.end(), required_num_full, max_split_weight);
	if (required_partial != 0) {
		ret.insert(ret.end(), lola::vx::v3::SynapseMatrix::Weight(required_partial));
	}
	ret.resize(num, lola::vx::v3::SynapseMatrix::Weight(0));
	return ret;
}

} // namespace grenade::vx::network
