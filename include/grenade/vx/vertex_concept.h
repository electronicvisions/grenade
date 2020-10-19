#pragma once
#include <array>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace grenade::vx {

struct Port;

namespace detail {

template <typename Inputs>
struct IsInputsReturn : std::false_type
{};

template <size_t N>
struct IsInputsReturn<std::array<Port, N>> : std::true_type
{};

template <>
struct IsInputsReturn<std::vector<Port>> : std::true_type
{};

} // namespace detail

/**
 * A vertex is an entity which has a defined number of input ports and one output port with defined
 * type and size.
 */
template <typename Vertex>
struct VertexConcept
{
	/* Each vertex has a single output port. */
	static_assert(
	    std::is_same_v<decltype(&Vertex::output), Port (Vertex::*)() const>,
	    "Vertex missing output method.");

	/*
	 * The inputs are to be iterable (`std::array` or `std::vector`) of type `Port`.
	 */
	static_assert(
	    detail::IsInputsReturn<decltype(std::declval<Vertex const>().inputs())>::value,
	    "Vertex missing inputs method.");

	/*
	 * The last element of the input ports can be variadic meaning it can be extended to an
	 * arbitrary number (including zero) of distinct ports. For this to be the case `variadic_input`
	 * has to be set to true.
	 */
	static_assert(
	    std::is_same_v<decltype(Vertex::variadic_input), bool const>,
	    "Vertex missing variadic_input specifier.");

	/*
	 * Vertices which are allowed connections between different execution instances are to set
	 * `can_connect_different_execution_instances` to true.
	 * TODO: this is rather a ConnectionType property, right?
	 */
	static_assert(
	    std::is_same_v<decltype(Vertex::can_connect_different_execution_instances), bool const>,
	    "Vertex missing can_connect_different_execution_instances specifier.");

	/**
	 * In addition connections between vertices can be restricted via:
	 *     supports_input_from(InputVertexT const& input, std::optional<PortRestriction> const&)
	 * This default to true if it is not specialized. TODO: find out how to test this.
	 */
};

namespace detail {

template <typename VertexVariant>
struct CheckVertexConcept;

template <typename... Vertex>
struct CheckVertexConcept<std::variant<Vertex...>> : VertexConcept<Vertex>...
{};

} // namespace detail

} // namespace grenade::vx
