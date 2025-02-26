#pragma once
#include "grenade/vx/common/entity_on_chip.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/population_on_execution_instance.h"
#include "grenade/vx/network/receptor.h"
#include "halco/common/geometry.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "hate/visibility.h"
#include <vector>
#if defined(__GENPYBIND__) || defined(__GENPYBIND_GENERATED__)
#include <pybind11/numpy.h>
#endif

namespace grenade::vx { namespace network GENPYBIND_TAG_GRENADE_VX_NETWORK {

/**
 * Projection between populations.
 */
struct GENPYBIND(visible) Projection : public common::EntityOnChip
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
	PopulationOnExecutionInstance population_pre{};
	/** Descriptor to post-synaptic population. */
	PopulationOnExecutionInstance population_post{};

	Projection() = default;
	Projection(
	    Receptor const& receptor,
	    Connections const& connections,
	    PopulationOnExecutionInstance population_pre,
	    PopulationOnExecutionInstance population_post,
	    common::EntityOnChip::ChipCoordinate chip_coordinate =
	        common::EntityOnChip::ChipCoordinate()) SYMBOL_VISIBLE;
	Projection(
	    Receptor const& receptor,
	    Connections&& connections,
	    PopulationOnExecutionInstance population_pre,
	    PopulationOnExecutionInstance population_post,
	    common::EntityOnChip::ChipCoordinate chip_coordinate =
	        common::EntityOnChip::ChipCoordinate()) SYMBOL_VISIBLE;

	GENPYBIND_MANUAL({
		using namespace grenade::vx::network;

		auto const from_numpy = [](GENPYBIND_PARENT_TYPE& self, Receptor const& receptor,
		                           pybind11::array_t<size_t> const& pyconnections,
		                           PopulationOnExecutionInstance const population_pre,
		                           PopulationOnExecutionInstance const population_post,
		                           grenade::vx::common::EntityOnChip::ChipCoordinate
		                               chip_coordinate =
		                                   grenade::vx::common::EntityOnChip::ChipCoordinate()) {
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
			self.chip_coordinate = chip_coordinate;
		};

		parent.def(
		    "from_numpy", from_numpy, parent->py::arg("receptor"), parent->py::arg("connections"),
		    parent->py::arg("population_pre"), parent->py::arg("population_post"),
		    parent->py::arg("chip_coordinate") =
		        grenade::vx::common::EntityOnChip::ChipCoordinate());
	})

	bool operator==(Projection const& other) const SYMBOL_VISIBLE;
	bool operator!=(Projection const& other) const SYMBOL_VISIBLE;

	GENPYBIND(stringstream)
	friend std::ostream& operator<<(std::ostream& os, Projection const& projection) SYMBOL_VISIBLE;
};

std::ostream& operator<<(std::ostream& os, Projection::Connection::Index const& index)
    SYMBOL_VISIBLE;

} // namespace network
} // namespace grenade::vx
