#pragma once
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <functional>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

/**
 * ReLU of multiple inputs of signal_flow::Int8 data type.
 */
struct SYMBOL_VISIBLE ReLU : public Transformation::Function
{
	ReLU() = default;

	/**
	 * Construct reul transformation with data size.
	 * @param size Number of data values per input
	 */
	ReLU(size_t size);

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

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
