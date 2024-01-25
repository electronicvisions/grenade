#pragma once
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

/**
 * Addition of multiple inputs of signal_flow::Int8 data type.
 */
struct SYMBOL_VISIBLE Addition : public Transformation::Function
{
	~Addition();

	Addition() = default;

	/**
	 * Construct addition transformation with specified number of inputs and their size.
	 * @param num_inputs Number of inputs
	 * @param size Number of data values per input
	 */
	Addition(size_t num_inputs, size_t size);

	std::vector<Port> inputs() const;
	Port output() const;

	bool equal(Transformation::Function const& other) const;

	Value apply(std::vector<Value> const& value) const;

	std::unique_ptr<Transformation::Function> clone() const;

private:
	size_t m_num_inputs{};
	size_t m_size{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
