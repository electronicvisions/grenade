#pragma once
#include "grenade/common/port_data.h"
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
	Addition() = default;

	/**
	 * Construct addition transformation with specified number of inputs and their size.
	 * @param num_inputs Number of inputs
	 * @param size Number of data values per input
	 */
	Addition(size_t num_inputs, size_t size);

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::vector<Transformation::Results> apply(
	    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& value)
	    const override;

	virtual std::unique_ptr<Transformation::Function> copy() const override;
	virtual std::unique_ptr<Transformation::Function> move() override;

protected:
	virtual bool is_equal_to(Transformation::Function const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	size_t m_num_inputs{};
	size_t m_size{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
