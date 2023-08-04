#pragma once
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population_on_network.h"
#include "grenade/vx/network/receptor.h"
#include "halco/common/geometry.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/visibility.h"
#include <vector>
#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/numpy.h>
#endif

namespace grenade::vx::network GENPYBIND_TAG_GRENADE_VX_NETWORK {

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

		typedef std::pair<size_t, halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron> Index;
		/** Index of neuron in pre-synaptic population. */
		Index index_pre;
		/** Index of neuron in post-synaptic population. */
		Index index_post;
		/** Weight of connection. */
		Weight weight;

		Connection() = default;
		Connection(Index const& index_pre, Index const& index_post, Weight weight) SYMBOL_VISIBLE;

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
	PopulationOnNetwork population_pre{};
	/** Descriptor to post-synaptic population. */
	PopulationOnNetwork population_post{};

	Projection() = default;
	Projection(
	    Receptor const& receptor,
	    Connections const& connections,
	    PopulationOnNetwork population_pre,
	    PopulationOnNetwork population_post) SYMBOL_VISIBLE;
	Projection(
	    Receptor const& receptor,
	    Connections&& connections,
	    PopulationOnNetwork population_pre,
	    PopulationOnNetwork population_post) SYMBOL_VISIBLE;

	GENPYBIND_MANUAL({
		using namespace grenade::vx::network;

		auto const from_numpy = [](GENPYBIND_PARENT_TYPE& self, Receptor const& receptor,
		                           pybind11::array_t<size_t> const& pyconnections,
		                           PopulationOnNetwork const population_pre,
		                           PopulationOnNetwork const population_post) {
			if (pyconnections.ndim() != 2) {
				throw std::runtime_error("Expected connections array to be of dimension 2.");
			}
			auto const shape = std::vector<size_t>{
			    pyconnections.shape(), pyconnections.shape() + pyconnections.ndim()};
			if (shape.at(1) != 5) {
				throw std::runtime_error("Expected connections array second dimension to be of "
				                         "size 5 (index_pre.first, index_pre.second, "
				                         "index_post.first, index_post.second, weight).");
			}
			self.connections.resize(shape.at(0));
			auto const data = pyconnections.unchecked<2>();
			for (size_t i = 0; i < self.connections.size(); ++i) {
				auto& lconn = self.connections.at(i);
				lconn.index_pre.first = data(i, 0);
				lconn.index_pre.second =
				    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron(data(i, 1));
				lconn.index_post.first = data(i, 2);
				lconn.index_post.second =
				    halco::hicann_dls::vx::v3::CompartmentOnLogicalNeuron(data(i, 3));
				lconn.weight = Projection::Connection::Weight(data(i, 4));
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

std::ostream& operator<<(std::ostream& os, Projection::Connection::Index const& index)
    SYMBOL_VISIBLE;

} // namespace grenade::vx::network
