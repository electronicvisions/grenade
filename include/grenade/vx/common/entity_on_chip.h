#pragma once
#include "grenade/common/connection_on_executor.h"
#include "grenade/vx/common/chip_on_connection.h"
#include "grenade/vx/genpybind.h"
#include "hate/visibility.h"
#include <iosfwd>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx {
namespace common GENPYBIND_TAG_GRENADE_VX_COMMON {

/**
 * Entity on chip mixin carrying chip id information.
 */
struct GENPYBIND(visible) EntityOnChip
{
	typedef std::pair<ChipOnConnection, grenade::common::ConnectionOnExecutor> ChipOnExecutor;

	explicit EntityOnChip(ChipOnExecutor const& chip_on_executor = ChipOnExecutor()) SYMBOL_VISIBLE;

	ChipOnExecutor chip_on_executor;

	bool operator==(EntityOnChip const& other) const SYMBOL_VISIBLE;
	bool operator!=(EntityOnChip const& other) const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, EntityOnChip const& entity) SYMBOL_VISIBLE;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace common
} // namespace grenade::vx
