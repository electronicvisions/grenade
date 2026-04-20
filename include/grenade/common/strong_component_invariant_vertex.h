#pragma once
#include "dapr/empty_property.h"
#include "dapr/property_holder.h"
#include "grenade/common/vertex.h"

namespace grenade::common {

/**
 * A vertex corresponding to a strong component invariant.
 */
struct SYMBOL_VISIBLE StrongComponentInvariantVertex : public grenade::common::Vertex
{
	/**
	 * Port type connecting strong component invariants.
	 */
	struct SYMBOL_VISIBLE PortType
	    : public dapr::EmptyProperty<PortType, grenade::common::VertexPortType>
	{};

	StrongComponentInvariantVertex(StrongComponentInvariant&& value);
	StrongComponentInvariantVertex(StrongComponentInvariant const& value);

	/**
	 * Get stored strong component invariant.
	 */
	virtual std::unique_ptr<StrongComponentInvariant> get_strong_component_invariant()
	    const override;

	/**
	 * Get input ports of vertex.
	 * The strong component invariant vertex has a single input port with one channel at position
	 * zero, which requires invariant changes.
	 */
	virtual std::vector<Port> get_input_ports() const override;

	/**
	 * Get output ports of vertex.
	 * The strong component invariant vertex has a single output port with one channel at position
	 * zero, which requires invariant changes.
	 */
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<grenade::common::Vertex> copy() const override;
	virtual std::unique_ptr<grenade::common::Vertex> move() override;

protected:
	virtual bool is_equal_to(Vertex const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	dapr::PropertyHolder<StrongComponentInvariant> m_value;
};


} // namespace grenade::common
