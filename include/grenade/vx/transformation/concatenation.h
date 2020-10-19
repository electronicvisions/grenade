#pragma once
#include "grenade/vx/connection_type.h"
#include "grenade/vx/vertex/transformation.h"
#include <vector>

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx::transformation {

struct Concatenation : public vertex::Transformation::Function
{
	Concatenation() = default;

	/**
	 * Construct concatenation transformation with data type and sizes to concatenate.
	 * @param type Data type
	 * @param sizes Sizes
	 */
	Concatenation(ConnectionType type, std::vector<size_t> const& sizes) SYMBOL_VISIBLE;

	std::vector<Port> inputs() const SYMBOL_VISIBLE;
	Port output() const SYMBOL_VISIBLE;

	bool equal(vertex::Transformation::Function const& other) const SYMBOL_VISIBLE;

	Value apply(std::vector<Value> const& value) const SYMBOL_VISIBLE;

private:
	ConnectionType m_type{};
	std::vector<size_t> m_sizes{};

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::transformation
