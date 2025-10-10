#pragma once
#include "dapr/property.h"
#include "dapr/property_holder.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/vertex.h"
#include "hate/visibility.h"
#include <optional>
#include <vector>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Edge property in topology possibly altering connectivity.
 * An edge connects two vertices in the topology and describes parallel independent signal paths
 * between channels of a single port on the source and target vertex.
 */
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) Edge : public dapr::Property<Edge>
{
	typedef std::vector<Vertex::Port>::size_type PortOnVertex;

	/**
	 * Port on source vertex.
	 * The value corresponds to the index of the port in the source vertex output ports.
	 */
	PortOnVertex port_on_source{0};

	/**
	 * Port on target vertex.
	 * The value corresponds to the index of the port in the target vertex input ports.
	 */
	PortOnVertex port_on_target{0};

	Edge() = default;
	Edge(
	    MultiIndexSequence const& channels_on_source, MultiIndexSequence const& channels_on_target);
	Edge(MultiIndexSequence&& channels_on_source, MultiIndexSequence&& channels_on_target)
	    GENPYBIND(hidden);
	Edge(
	    MultiIndexSequence const& channels_on_source,
	    MultiIndexSequence const& channels_on_target,
	    PortOnVertex port_on_source,
	    PortOnVertex port_on_target);
	Edge(
	    MultiIndexSequence&& channels_on_source,
	    MultiIndexSequence&& channels_on_target,
	    PortOnVertex port_on_source,
	    PortOnVertex port_on_target) GENPYBIND(hidden);

	MultiIndexSequence const& get_channels_on_source() const GENPYBIND(hidden);

	GENPYBIND_MANUAL({
		parent.def("get_channels_on_source", [](GENPYBIND_PARENT_TYPE const& self) {
			return self.get_channels_on_source().copy();
		});
	})

	void set_channels_on_source(MultiIndexSequence const& value);
	void set_channels_on_source(MultiIndexSequence&& value) GENPYBIND(hidden);

	MultiIndexSequence const& get_channels_on_target() const;
	void set_channels_on_target(MultiIndexSequence const& value);
	void set_channels_on_target(MultiIndexSequence&& value) GENPYBIND(hidden);

	/**
	 * Copy edge property.
	 */
	std::unique_ptr<Edge> copy() const GENPYBIND(hidden);

	/**
	 * Move edge property.
	 */
	std::unique_ptr<Edge> move() GENPYBIND(hidden);

protected:
	std::ostream& print(std::ostream& os) const;
	bool is_equal_to(Edge const& other) const;

private:
	dapr::PropertyHolder<MultiIndexSequence> m_channels_on_source;
	dapr::PropertyHolder<MultiIndexSequence> m_channels_on_target;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace common
} // namespace grenade
