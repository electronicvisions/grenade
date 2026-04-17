#pragma once
#include "grenade/common/execution_instance_id.h"
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
	ConvertingReLU() = default;

	/**
	 * Construct converting relu transformation with data size and configuration.
	 * @param size Number of data values per input
	 * @param shift Number of bits to shift after relu before clamping
	 */
	ConvertingReLU(size_t size, uint32_t shift);

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::vector<Transformation::Results> apply(
	    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data)
	    const override;

	virtual std::unique_ptr<Function> copy() const override;
	virtual std::unique_ptr<Function> move() override;

protected:
	virtual bool is_equal_to(Function const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	size_t m_size{};
	uint32_t m_shift{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
