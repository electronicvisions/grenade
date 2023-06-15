#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

struct SYMBOL_VISIBLE Concatenation : public Transformation::Function
{
	~Concatenation() SYMBOL_VISIBLE;

	Concatenation() = default;

	/**
	 * Construct concatenation transformation with data type and sizes to concatenate.
	 * @param type Data type
	 * @param sizes Sizes
	 */
	Concatenation(ConnectionType type, std::vector<size_t> const& sizes);

	std::vector<Port> inputs() const;
	Port output() const;

	bool equal(Transformation::Function const& other) const;

	Value apply(std::vector<Value> const& value) const;

private:
	ConnectionType m_type{};
	std::vector<size_t> m_sizes{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
