#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/logical_network/population.h"
#include "grenade/vx/logical_network/receptor.h"
#include "halco/common/geometry.h"
#include "hate/visibility.h"
#include <vector>
#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/numpy.h>
#endif

namespace grenade::vx GENPYBIND_TAG_GRENADE_VX {

namespace logical_network GENPYBIND_MODULE {

/**
 * Projection between populations.
 */
struct GENPYBIND(visible) Projection
{
	/** Receptor type. */
	Receptor receptor;

	/** Single neuron connection. */
	struct Connection
	{
		struct GENPYBIND(inline_base("*")) Weight
		    : public halco::common::detail::BaseType<Weight, size_t>
		{
			constexpr explicit Weight(value_type const value = 0) : base_t(value) {}
		};

		/** Index of neuron in pre-synaptic population. */
		size_t index_pre;
		/** Index of neuron in post-synaptic population. */
		size_t index_post;
		/** Weight of connection. */
		Weight weight;

		Connection() = default;
		Connection(size_t index_pre, size_t index_post, Weight weight) SYMBOL_VISIBLE;

		bool operator==(Connection const& other) const SYMBOL_VISIBLE;
		bool operator!=(Connection const& other) const SYMBOL_VISIBLE;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Connection const& connection)
		    SYMBOL_VISIBLE;
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
	    Receptor const& receptor,
	    Connections const& connections,
	    PopulationDescriptor population_pre,
	    PopulationDescriptor population_post) SYMBOL_VISIBLE;
	Projection(
	    Receptor const& receptor,
	    Connections&& connections,
	    PopulationDescriptor population_pre,
	    PopulationDescriptor population_post) SYMBOL_VISIBLE;

	GENPYBIND_MANUAL({
		using namespace grenade::vx::logical_network;

		auto const from_numpy = [](GENPYBIND_PARENT_TYPE& self, Receptor const& receptor,
		                           pybind11::array_t<size_t> const& pyconnections,
		                           PopulationDescriptor const population_pre,
		                           PopulationDescriptor const population_post) {
			if (pyconnections.ndim() != 2) {
				throw std::runtime_error("Expected connections array to be of dimension 2.");
			}
			auto const shape = std::vector<size_t>{
			    pyconnections.shape(), pyconnections.shape() + pyconnections.ndim()};
			if (shape.at(1) != 3) {
				throw std::runtime_error("Expected connections array second dimension to be of "
				                         "size 3 (index_pre, index_post, weight).");
			}
			self.connections.resize(shape.at(0));
			auto const data = pyconnections.unchecked();
			for (size_t i = 0; i < self.connections.size(); ++i) {
				auto& lconn = self.connections.at(i);
				lconn.index_pre = data(i, 0);
				lconn.index_post = data(i, 1);
				lconn.weight = Projection::Connection::Weight(data(i, 2));
			}
			self.receptor = receptor;
			self.population_pre = population_pre;
			self.population_post = population_post;
		};

		parent.def("from_numpy", from_numpy);
	})

	bool operator==(Projection const& other) const SYMBOL_VISIBLE;
	bool operator!=(Projection const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, Projection const& projection) SYMBOL_VISIBLE;
};

/** Descriptor to be used to identify a projection. */
struct GENPYBIND(inline_base("*")) ProjectionDescriptor
    : public halco::common::detail::BaseType<ProjectionDescriptor, size_t>
{
	constexpr explicit ProjectionDescriptor(value_type const value = 0) : base_t(value) {}
};

} // namespace logical_network

} // namespace grenade::vx

namespace std {

HALCO_GEOMETRY_HASH_CLASS(grenade::vx::logical_network::ProjectionDescriptor)

} // namespace std
