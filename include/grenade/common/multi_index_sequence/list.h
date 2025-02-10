#pragma once
#include "dapr/property_holder.h"
#include "grenade/common/genpybind.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence.h"
#include "hate/visibility.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#if defined(__GENPYBIND__) or defined(__GENPYBIND_GENERATED__)
#include <pybind11/numpy.h>
#endif

namespace cereal {
struct access;
} // namespace cereal

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Multi-index sequence defined by a list of multi-indices.
 */
struct SYMBOL_VISIBLE GENPYBIND(visible) ListMultiIndexSequence : public MultiIndexSequence
{
	ListMultiIndexSequence() = default;

	/**
	 * Construct sequence.
	 * @param elements Multi-index elements of sequence
	 * @throws std::invalid_argument On elements not featuring homogeneous dimensionality
	 */
	explicit ListMultiIndexSequence(std::vector<MultiIndex> elements);

	/**
	 * Construct sequence.
	 * Constructor present for Python wrapping to work.
	 * @param elements Multi-index elements of sequence
	 * @param dimension_units Dimension units of multi indices, if none are given, dimensions are
	 * unitless
	 * @throws std::invalid_argument On elements not featuring homogeneous dimensionality or number
	 * of dimension units not matching dimensionality of elements
	 */
	ListMultiIndexSequence(
	    std::vector<MultiIndex> elements,
	    std::vector<std::optional<std::reference_wrapper<DimensionUnit const>>> const&
	        dimension_units);

	/**
	 * Construct sequence.
	 * @param elements Multi-index elements of sequence
	 * @param dimension_units Dimension units of multi indices, if none are given, dimensions are
	 * unitless
	 * @throws std::invalid_argument On elements not featuring homogeneous dimensionality or number
	 * of dimension units not matching dimensionality of elements
	 */
	ListMultiIndexSequence(std::vector<MultiIndex> elements, DimensionUnits dimension_units)
	    GENPYBIND(hidden);

	/**
	 * Get dimensionality of multi-indices in sequence.
	 */
	virtual size_t dimensionality() const override;

	/**
	 * Get number of elements in sequence.
	 */
	virtual size_t size() const override;

	/**
	 * Get elements in sequence.
	 */
	virtual std::vector<MultiIndex> get_elements() const override;

	/**
	 * Set elements in sequence.
	 * @param value Elements to set
	 * @throws std::invalid_argument On elements not featuring homogeneous dimensionality or
	 * dimensionality not matching number of dimension units in sequence
	 */
	void set_elements(std::vector<MultiIndex> const& value);

	/**
	 * Set elements in sequence.
	 * @param value Elements to set
	 * @throws std::invalid_argument On elements not featuring homogeneous dimensionality or
	 * dimensionality not matching number of dimension units in sequence
	 */
	void set_elements(std::vector<MultiIndex>&& value) GENPYBIND(hidden);

	/**
	 * Get dimension units.
	 */
	virtual DimensionUnits get_dimension_units() const override;

	/**
	 * Set dimension units.
	 * @param value Dimension units to set
	 * @throws std::invalid_argument On number of dimension units not matching dimensionality of
	 * elements in sequence
	 */
	void set_dimension_units(DimensionUnits value);

	virtual std::unique_ptr<MultiIndexSequence> copy() const override;
	virtual std::unique_ptr<MultiIndexSequence> move() override;

	GENPYBIND_MANUAL({
		using namespace grenade::common;

		auto const from_numpy = [](GENPYBIND_PARENT_TYPE& self,
		                           pybind11::array_t<size_t> const& pypoints) {
			if (pypoints.ndim() != 2) {
				throw std::runtime_error("Expected points array to be of dimension 2.");
			}
			auto const shape =
			    std::vector<size_t>{pypoints.shape(), pypoints.shape() + pypoints.ndim()};
			std::vector<MultiIndex> elements(shape.at(0));

			auto const data = pypoints.unchecked<2>();
			for (size_t i = 0; i < elements.size(); ++i) {
				auto& lpoint = elements.at(i);
				lpoint.value.resize(shape.at(1));
				for (size_t j = 0; j < lpoint.value.size(); ++j) {
					lpoint.value.at(j) = data(i, j);
				}
			}
			self.set_elements({});
			self.set_dimension_units(MultiIndexSequence::DimensionUnits(shape.at(1)));
			self.set_elements(std::move(elements));
		};

		parent.def("from_numpy", from_numpy, parent->py::arg("points"));

		auto const from_numpy_with_dimension_units =
		    [](GENPYBIND_PARENT_TYPE& self, pybind11::array_t<size_t> const& pypoints,
		       std::vector<std::optional<std::reference_wrapper<
		           grenade::common::MultiIndexSequence::DimensionUnit const>>> const&
		           dimension_units) {
			    if (pypoints.ndim() != 2) {
				    throw std::runtime_error("Expected points array to be of dimension 2.");
			    }
			    auto const shape =
			        std::vector<size_t>{pypoints.shape(), pypoints.shape() + pypoints.ndim()};
			    std::vector<MultiIndex> elements(shape.at(0));

			    auto const data = pypoints.unchecked<2>();
			    for (size_t i = 0; i < elements.size(); ++i) {
				    auto& lpoint = elements.at(i);
				    lpoint.value.resize(shape.at(1));
				    for (size_t j = 0; j < lpoint.value.size(); ++j) {
					    lpoint.value.at(j) = data(i, j);
				    }
			    }
			    grenade::common::MultiIndexSequence::DimensionUnits ldimension_units;
			    for (auto const& dimension_unit : dimension_units) {
				    if (dimension_unit) {
					    ldimension_units.push_back(dimension_unit->get());
				    } else {
					    ldimension_units.push_back({});
				    }
			    }
			    self.set_elements({});
			    self.set_dimension_units(std::move(ldimension_units));
			    self.set_elements(std::move(elements));
		    };

		parent.def(
		    "from_numpy", from_numpy_with_dimension_units, parent->py::arg("points"),
		    parent->py::arg("dimension_units"));
	})

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(MultiIndexSequence const& other) const override;

private:
	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive&, std::uint32_t);

	std::vector<MultiIndex> m_elements;
	std::vector<dapr::PropertyHolder<DimensionUnit>> m_dimension_units;
};

} // namespace common
} // namespace grenade
