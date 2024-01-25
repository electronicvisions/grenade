#pragma once
#include "grenade/vx/signal_flow/port.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

/**
 * Transformation packing single spikes into double spikes when times match.
 */
struct SYMBOL_VISIBLE SpikePacking : public signal_flow::vertex::Transformation::Function
{
	~SpikePacking() SYMBOL_VISIBLE;

	SpikePacking() = default;

	std::vector<signal_flow::Port> inputs() const SYMBOL_VISIBLE;
	signal_flow::Port output() const SYMBOL_VISIBLE;

	bool equal(signal_flow::vertex::Transformation::Function const& other) const SYMBOL_VISIBLE;

	Value apply(std::vector<Value> const& value) const SYMBOL_VISIBLE;

	std::unique_ptr<Transformation::Function> clone() const;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
