#pragma once
#include "grenade/vx/genpybind.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "hate/visibility.h"
#include <iosfwd>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::common GENPYBIND_TAG_GRENADE_VX_COMMON {

/**
 * Entity on chip mixin carrying chip id information.
 */
struct GENPYBIND(visible) EntityOnChip
{
	typedef halco::hicann_dls::vx::v3::DLSGlobal ChipCoordinate;

	explicit EntityOnChip(ChipCoordinate const& chip_coordinate = ChipCoordinate()) SYMBOL_VISIBLE;

	ChipCoordinate chip_coordinate;

	bool operator==(EntityOnChip const& other) const SYMBOL_VISIBLE;
	bool operator!=(EntityOnChip const& other) const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, EntityOnChip const& entity) SYMBOL_VISIBLE;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::common
