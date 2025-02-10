#pragma once
#include "dapr/property.h"
#include "dapr/property_holder.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/port_data.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/vertex_port_type.h"
#include "hate/visibility.h"
#include <memory>
#include <optional>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal


namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

struct Edge;

/**
 * Vertex property in topology describing computation.
 * It can have input and output ports, where other vertices can be connected via edges in the
 * topology.
 */
struct SYMBOL_VISIBLE GENPYBIND(
    inline_base("*"), holder_type("std::shared_ptr<grenade::common::Vertex>")) Vertex
    : public dapr::Property<Vertex>
    , public std::enable_shared_from_this<Vertex>
{
	/**
	 * Port of vertex describing parallel independent signal channels with homogeneous signal
	 * type to/from the vertex.
	 *
	 * Individual channels are referenced by elements in a multi-index sequence, which enables a
	 * representation of locations of channels in an index space.
	 */
	struct Port
	{
		/**
		 * Type of signal this port handles.
		 */
		typedef VertexPortType Type;

		/**
		 * Get signal type.
		 */
		Type const& get_type() const;

		/**
		 * Set signal type.
		 * @param value Type to set
		 */
		void set_type(Type const& value);

		/**
		 * Whether the port allows supplying/using signal channels to/from multiple target/sources.
		 * Signal channels can be supplied by multiple sources if the target vertex supports an
		 * abstract sum of the signals. Signal channels can be used by multiple targets if the
		 * source vertex supports an abstract split_support of the signals.
		 */
		enum class SumOrSplitSupport : bool
		{
			no,
			yes
		};
		SumOrSplitSupport sum_or_split_support;

		/**
		 * Whether the port supports or requires an execution instance transition.
		 * Execution instance transition constraint translates as implication to time domain
		 * transition constraint.
		 */
		enum class ExecutionInstanceTransitionConstraint
		{
			not_supported,
			supported,
			required
		};
		ExecutionInstanceTransitionConstraint execution_instance_transition_constraint;

		/**
		 * Whether the port requires (input) or generates (output) data.
		 * If an input port requires data, either an in-edge is required which supplies the data or
		 * it is to be supplied separately in the InputData.
		 * TODO: This is a property of the port signal type
		 */
		enum class RequiresOrGeneratesData
		{
			no,
			yes
		};
		RequiresOrGeneratesData requires_or_generates_data;

		Port() = default;

		/**
		 * Construct port with signal type, sequence of independent signal channels and whether it
		 * allows distribution/merge of multiple signals onto/from multiple targets/sources.
		 */
		Port(
		    Type const& type,
		    SumOrSplitSupport sum_or_split_support,
		    ExecutionInstanceTransitionConstraint execution_instance_transition_constraint,
		    RequiresOrGeneratesData requires_or_generates_data,
		    MultiIndexSequence const& channels);
		Port(
		    Type const& type,
		    SumOrSplitSupport sum_or_split_support,
		    ExecutionInstanceTransitionConstraint execution_instance_transition_constraint,
		    RequiresOrGeneratesData requires_or_generates_data,
		    MultiIndexSequence&& channels) GENPYBIND(hidden);

		/**
		 * Get multi-index sequence of independent signal channels.
		 */
		GENPYBIND(return_value_policy(reference_internal))
		MultiIndexSequence const& get_channels() const;

		/**
		 * Set multi-index sequence of independent signal channels.
		 * @param value Value to set
		 * @throws std::invalid_argument On sequence not being injective
		 */
		void set_channels(MultiIndexSequence const& value);

		/**
		 * Set multi-index sequence of independent signal channels.
		 * @param value Value to set
		 * @throws std::invalid_argument On sequence not being injective
		 */
		void set_channels(MultiIndexSequence&& value) GENPYBIND(hidden);

		bool operator==(Port const& other) const = default;
		bool operator!=(Port const& other) const = default;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Port const& value) SYMBOL_VISIBLE;

	private:
		dapr::PropertyHolder<Type> m_type;
		dapr::PropertyHolder<MultiIndexSequence> m_channels;

	private:
		friend struct cereal::access;
		template <typename Archive>
		void serialize(Archive& ar, std::uint32_t);
	};

	/**
	 * Invariant of vertex which is required to be equal across its strong component.
	 * The default implementation compares unequal to everything including itself if no time domain
	 * is specified and compares the equality of the time domain otherwise.
	 * For a vertex residing on a time domain, all vertices in a strong component are required to be
	 * in the same time domain.
	 */
	struct GENPYBIND(inline_base("*")) StrongComponentInvariant
	    : public dapr::Property<StrongComponentInvariant>
	{
		virtual ~StrongComponentInvariant();

		/**
		 * Construct strong component invariant with optional time domain.
		 */
		StrongComponentInvariant(std::optional<TimeDomainOnTopology> time_domain);

		/**
		 * Time domain on topology.
		 * In a strong component, all vertices are required to be part of the same time domain.
		 */
		std::optional<TimeDomainOnTopology> time_domain;

		virtual std::unique_ptr<StrongComponentInvariant> copy() const override;
		virtual std::unique_ptr<StrongComponentInvariant> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(StrongComponentInvariant const& other) const override;
	};

	virtual ~Vertex();

	/**
	 * Get the input ports of vertex.
	 * The position of a port in the return value is used to encode the connectivity of an
	 * edge to a port.
	 */
	virtual std::vector<Port> get_input_ports() const = 0;

	/**
	 * Get the output port of vertex.
	 * The position of a port in the return value is used to encode the connectivity of an
	 * edge from a port.
	 */
	virtual std::vector<Port> get_output_ports() const = 0;

	/**
	 * Get invariant of vertex which is required to be equal across its strong component.
	 */
	virtual std::unique_ptr<StrongComponentInvariant> get_strong_component_invariant() const;

	/**
	 * Get whether given data is valid for input port on vertex.
	 * @param data Data to check
	 * @returns Validity of data
	 */
	virtual bool valid_input_port_data(size_t input_port_on_vertex, PortData const& data) const;

	/**
	 * Get identifier of time domain of vertex.
	 * All vertices onto the same time domain are expected to share the time dimension and are
	 * allowed to interact instantaneously with each other.
	 * If no time domain is returned, the vertices is offline without time dimension.
	 */
	virtual std::optional<TimeDomainOnTopology> get_time_domain() const;

	/**
	 * Get whether the edge from the source vertex to this vertex is valid.
	 * @param source Source vertex
	 * @param edge Edge
	 */
	virtual bool valid_edge_from(Vertex const& source, Edge const& edge) const;

	/**
	 * Get whether the edge to the target vertex from this vertex is valid.
	 * @param target Target vertex
	 * @param edge Edge
	 */
	virtual bool valid_edge_to(Vertex const& target, Edge const& edge) const;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};


std::ostream& operator<<(
    std::ostream& os,
    Vertex::Port::ExecutionInstanceTransitionConstraint const& value) SYMBOL_VISIBLE;
std::ostream& operator<<(std::ostream& os, Vertex::Port::SumOrSplitSupport const& value)
    SYMBOL_VISIBLE;

} // namespace common
} // namespace grenade
