#pragma once
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

/**
 * ConvertingReLU of multiple inputs from signal_flow::Int8 to signal_flow::UInt5.
 */
struct SYMBOL_VISIBLE ConvertingReLU : public Transformation::Function
{
	~ConvertingReLU() SYMBOL_VISIBLE;

	ConvertingReLU() = default;

	/**
	 * Construct converting relu transformation with data size and configuration.
	 * @param size Number of data values per input
	 * @param shift Number of bits to shift after relu before clamping
	 */
	ConvertingReLU(size_t size, uint32_t shift);

	std::vector<Port> inputs() const;
	Port output() const;

	bool equal(Transformation::Function const& other) const;

	Value apply(std::vector<Value> const& value) const;

	std::unique_ptr<Transformation::Function> clone() const;

private:
	size_t m_size;
	uint32_t m_shift;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
