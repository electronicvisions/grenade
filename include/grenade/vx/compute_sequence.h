#pragma once
#include "grenade/cerealization.h"
#include "grenade/vx/compute_single_addition.h"
#include "grenade/vx/compute_single_argmax.h"
#include "grenade/vx/compute_single_converting_relu.h"
#include "grenade/vx/compute_single_mac.h"
#include "grenade/vx/compute_single_relu.h"
#include "grenade/vx/io_data_list.h"
#include "hate/visibility.h"
#include "hxcomm/vx/connection_variant.h"
#include <list>
#include <variant>

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx {

struct ChipConfig;

struct ComputeSequence
{
	typedef std::variant<
	    ComputeSingleAddition,
	    ComputeSingleArgMax,
	    ComputeSingleMAC,
	    ComputeSingleReLU,
	    ComputeSingleConvertingReLU>
	    Entry;

	std::list<Entry> data;

	ComputeSequence() = default;

	IODataList::Entry run(
	    IODataList::Entry const& input,
	    ChipConfig const& config,
	    hxcomm::vx::ConnectionVariant& connection) SYMBOL_VISIBLE;

private:
	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::ComputeSequence)
