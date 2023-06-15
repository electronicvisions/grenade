#pragma once
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

/**
 * Subtraction of multiple inputs of signal_flow::Int8 data type.
 */
struct SYMBOL_VISIBLE Subtraction : public Transformation::Function
{
	~Subtraction();

	Subtraction() = default;

	/**
	 * Construct subtraction transformation with specified number of inputs and their size.
	 * @param num_inputs Number of inputs
	 * @param size Number of data values per input
	 */
	Subtraction(size_t num_inputs, size_t size);

	std::vector<Port> inputs() const;
	Port output() const;

	bool equal(Transformation::Function const& other) const;

	Value apply(std::vector<Value> const& value) const;

private:
	size_t m_num_inputs{};
	size_t m_size{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
