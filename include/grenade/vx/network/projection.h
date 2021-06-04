#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/network/population.h"
#include "halco/common/geometry.h"
#include "hate/visibility.h"
#include "lola/vx/v2/synapse.h"
#include <vector>

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace network {

/**
 * Projection between populations.
 */
struct GENPYBIND(visible) Projection
{
	/** Receptor type of connections. */
	enum class ReceptorType
	{
		excitatory,
		inhibitory
	};
	/** Receptor type. */
	ReceptorType receptor_type;

	/** Single neuron connection. */
	struct Connection
	{
		/** (Possibly virtualized) weight of a connection. */
		struct GENPYBIND(inline_base("*")) Weight
		    : public halco::common::detail::BaseType<Weight, size_t>
		{
			constexpr explicit Weight(value_type const value = 0) GENPYBIND(implicit_conversion) :
			    base_t(value)
			{}
		};

		/** Index of neuron in pre-synaptic population. */
		size_t index_pre;
		/** Index of neuron in post-synaptic population. */
		size_t index_post;
		/** Weight of connection. */
		Weight weight;

		Connection() = default;
		Connection(size_t index_pre, size_t index_post, Weight weight) SYMBOL_VISIBLE;
	};
	/** Point-to-point neuron connections type. */
	typedef std::vector<Connection> Connections;
	/** Point-to-point neuron connections. */
	Connections connections{};

	/** Descriptor to pre-synaptic population. */
	PopulationDescriptor population_pre{};
	/** Descriptor to post-synaptic population. */
	PopulationDescriptor population_post{};

	Projection() = default;
	Projection(
	    ReceptorType receptor_type,
	    Connections const& connections,
	    PopulationDescriptor population_pre,
	    PopulationDescriptor population_post) SYMBOL_VISIBLE;
	Projection(
	    ReceptorType receptor_type,
	    Connections&& connections,
	    PopulationDescriptor population_pre,
	    PopulationDescriptor population_post) SYMBOL_VISIBLE;
};

/** Descriptor to be used to identify a projection. */
struct GENPYBIND(inline_base("*")) ProjectionDescriptor
    : public halco::common::detail::BaseType<ProjectionDescriptor, size_t>
{
	constexpr explicit ProjectionDescriptor(value_type const value = 0) : base_t(value) {}
};

} // namespace network

} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::network::ProjectionDescriptor)

} // namespace std
