#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <functional>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

/**
 * Transformation packing single spikes into double spikes when times match.
 */
struct SYMBOL_VISIBLE SpikePacking : public Transformation::Function
{
	SpikePacking() = default;

	std::vector<Port> get_input_ports() const override;
	std::vector<Port> get_output_ports() const override;

	std::vector<Transformation::Results> apply(
	    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data)
	    const override;

	std::unique_ptr<Function> copy() const override;
	std::unique_ptr<Function> move() override;

protected:
	bool is_equal_to(Function const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
