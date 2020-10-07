#pragma once
#include "grenade/cerealization.h"
#include "grenade/vx/compute/addition.h"
#include "grenade/vx/compute/argmax.h"
#include "grenade/vx/compute/conv1d.h"
#include "grenade/vx/compute/converting_relu.h"
#include "grenade/vx/compute/mac.h"
#include "grenade/vx/compute/relu.h"
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

namespace compute {

struct Sequence
{
	typedef std::variant<Addition, ArgMax, Conv1d, MAC, ReLU, ConvertingReLU> Entry;

	std::list<Entry> data;

	Sequence() = default;

	IODataList::Entry run(
	    IODataList::Entry const& input,
	    ChipConfig const& config,
	    hxcomm::vx::ConnectionVariant& connection) SYMBOL_VISIBLE;

private:
	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace compute

} // namespace grenade::vx

EXTERN_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::Sequence)
