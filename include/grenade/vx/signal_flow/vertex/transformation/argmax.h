#pragma once
#include "grenade/common/port_data.h"
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
	ArgMax() = default;

	/**
	 * Construct argmax transformation with data type and size.
	 * @param size Number of data values
	 * @param type Type of data to compute argmax over
	 */
	ArgMax(size_t size, ConnectionType type);

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::vector<Transformation::Results> apply(
	    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data)
	    const override;

	virtual std::unique_ptr<Transformation::Function> copy() const override;
	virtual std::unique_ptr<Transformation::Function> move() override;

protected:
	virtual bool is_equal_to(Transformation::Function const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	size_t m_size{};
	ConnectionType m_type{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
