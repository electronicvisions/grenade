#pragma once
#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "hate/visibility.h"
#include <functional>
#include <memory>
#include <ostream>
#include <stddef.h>
#include <variant>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex::transformation {

/**
 * Identity operation.
 */
struct SYMBOL_VISIBLE Identity : public Transformation::Function
{
	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, grenade::common::PortData const& data) const override;

	Identity() = default;

	/**
	 * Construct identity.
	 * @param output_ports Output ports to use
	 */
	Identity(std::vector<Port> output_ports);

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	/**
	 * Apply transformation.
	 * @param data Input data
	 * @return Output data
	 */
	std::vector<Transformation::Results> apply(
	    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data)
	    const override;

	virtual std::unique_ptr<Function> copy() const override;
	virtual std::unique_ptr<Function> move() override;

protected:
	std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Function const& other) const override;

private:
	std::vector<Port> m_output_ports;
};

} // grenade::vx::signal_flow::vertex::transformation
