#pragma once
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <functional>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

struct SYMBOL_VISIBLE Concatenation : public Transformation::Function
{
	Concatenation() = default;

	/**
	 * Construct concatenation transformation with data type and sizes to concatenate.
	 * @param type Data type
	 * @param sizes Sizes
	 */
	Concatenation(ConnectionType type, std::vector<size_t> const& sizes);

	virtual std::vector<Function::Port> get_input_ports() const override;
	virtual std::vector<Function::Port> get_output_ports() const override;

	virtual std::vector<Transformation::Results> apply(
	    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data)
	    const override;

	virtual std::unique_ptr<Function> copy() const override;
	virtual std::unique_ptr<Function> move() override;

protected:
	virtual bool is_equal_to(Function const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	ConnectionType m_type{};
	std::vector<size_t> m_sizes{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
