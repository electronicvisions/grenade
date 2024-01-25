#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

/**
 * ArgMax of multiple inputs.
 */
struct SYMBOL_VISIBLE ArgMax : public Transformation::Function
{
	~ArgMax() SYMBOL_VISIBLE;

	ArgMax() = default;

	/**
	 * Construct argmax transformation with data type and size.
	 * @param size Number of data values
	 * @param type Type of data to compute argmax over
	 */
	ArgMax(size_t size, ConnectionType type);

	std::vector<Port> inputs() const;
	Port output() const;

	bool equal(Transformation::Function const& other) const;

	Value apply(std::vector<Value> const& value) const;

	std::unique_ptr<Transformation::Function> clone() const;

private:
	size_t m_size{};
	ConnectionType m_type{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
