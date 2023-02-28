#include <gtest/gtest.h>

#include "grenade/vx/signal_flow/port.h"
#include <stdexcept>
#include <vector>

/**
 * Drop-in replacement for `static_assert` performing checks at runtime.
 */
struct exceptionalized_static_assert
{
	exceptionalized_static_assert(bool const cond, char const* what)
	{
		if (cond) {
			static_cast<void>(what);
		} else {
			throw std::logic_error(what);
		}
	}

	exceptionalized_static_assert(bool const cond) :
	    exceptionalized_static_assert(cond, "static_assert failed.")
	{}
};

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
/**
 * Replace `static_assert` by runtime checks in order to be able to unittest failures.
 */
#define static_assert(cond, ...)                                                                   \
	exceptionalized_static_assert TOKENPASTE2(_test_, __COUNTER__)                                 \
	{                                                                                              \
		cond, __VA_ARGS__                                                                          \
	}

#include "grenade/vx/signal_flow/detail/vertex_concept.h"

using namespace grenade::vx::signal_flow;

struct IncorrectOutput
{
	void output() const;
};

struct IncorrectInputs
{
	Port output() const;

	std::vector<int> inputs() const;
};

struct IncorrectVariadicInput
{
	Port output() const;
	std::vector<Port> inputs() const;

	int const variadic_input;
};

struct IncorrectCanConnectDifferentExecutionInstances
{
	Port output() const;
	std::vector<Port> inputs() const;
	bool const variadic_input;

	int const can_connect_different_execution_instances;
};

struct Correct
{
	Port output() const;
	std::vector<Port> inputs() const;
	bool const variadic_input;
	bool const can_connect_different_execution_instances;
};

TEST(VertexConcept, General)
{
	EXPECT_THROW(detail::VertexConcept<IncorrectOutput>{}, std::logic_error);
	EXPECT_THROW(detail::VertexConcept<IncorrectInputs>{}, std::logic_error);
	EXPECT_THROW(detail::VertexConcept<IncorrectVariadicInput>{}, std::logic_error);
	EXPECT_THROW(
	    detail::VertexConcept<IncorrectCanConnectDifferentExecutionInstances>{}, std::logic_error);
	EXPECT_NO_THROW(detail::VertexConcept<Correct>{});
}
