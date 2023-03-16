#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/placed_atomic/projection.h"
#include "halco/common/geometry.h"
#include "hate/visibility.h"
#include <iosfwd>

namespace grenade::vx::network::placed_logical GENPYBIND_TAG_GRENADE_VX_NETWORK_PLACED_LOGICAL {

struct Receptor;
size_t hash_value(Receptor const& receptor) SYMBOL_VISIBLE;

/**
 * Receptor description of a neuron (compartment).
 */
struct GENPYBIND(visible) Receptor
{
	/** ID of receptor. */
	struct GENPYBIND(inline_base("*")) ID : public halco::common::detail::BaseType<ID, size_t>
	{
		constexpr explicit ID(value_type const value = 0) : base_t(value) {}
	};
	ID id;

	/** Receptor type. */
	typedef network::placed_atomic::Projection::ReceptorType Type GENPYBIND(visible);
	Type type;

	Receptor() = default;

	Receptor(ID id, Type type) SYMBOL_VISIBLE;

	bool operator==(Receptor const& other) const SYMBOL_VISIBLE;
	bool operator!=(Receptor const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, Receptor const& receptor) SYMBOL_VISIBLE;

	friend size_t hash_value(Receptor const& receptor) SYMBOL_VISIBLE;

	GENPYBIND(expose_as(__hash__))
	size_t hash() const SYMBOL_VISIBLE;
};

} // namespace grenade::vx::network::placed_logical

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::placed_logical::Receptor::ID)

template <>
struct hash<grenade::vx::network::placed_logical::Receptor>
{
	size_t operator()(grenade::vx::network::placed_logical::Receptor const& t) const
	{
		return grenade::vx::network::placed_logical::hash_value(t);
	}
};

} // namespace std
