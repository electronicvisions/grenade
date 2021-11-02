#pragma once
#include "grenade/vx/connection_type.h"
#include "grenade/vx/port.h"
#include "halco/common/geometry.h"
#include "hate/visibility.h"
#include <array>
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <vector>

namespace cereal {
class access;
} // namespace cereal

namespace grenade::vx {

struct PortRestriction;

namespace vertex {

struct SynapseArrayView;

/**
 * A plasticity rule to operate on synapse array views.
 */
struct PlasticityRule
{
	constexpr static bool can_connect_different_execution_instances = false;

	/**
	 * Timing information for execution of the rule.
	 */
	struct Timer
	{
		/** PPU clock cycles. */
		struct GENPYBIND(inline_base("*")) Value
		    : public halco::common::detail::RantWrapper<Value, uintmax_t, 0xffffffff, 0>
		{
			constexpr explicit Value(uintmax_t const value = 0) : rant_t(value) {}
		};

		Value start;
		Value period;
		size_t num_periods;

		bool operator==(Timer const& other) const SYMBOL_VISIBLE;
		bool operator!=(Timer const& other) const SYMBOL_VISIBLE;

		GENPYBIND(stringstream)
		friend std::ostream& operator<<(std::ostream& os, Timer const& timer) SYMBOL_VISIBLE;

	private:
		friend class cereal::access;
		template <typename Archive>
		void serialize(Archive& ar, std::uint32_t version);
	};

	PlasticityRule() = default;

	/**
	 * Construct PlasticityRule with specified kernel, timer and synapse information.
	 * @param kernel Kernel to apply
	 * @param timer Timer to use
	 * @param synapse_column_size Column-size of synapse views to alter
	 */
	PlasticityRule(
	    std::string kernel,
	    Timer const& timer,
	    std::vector<size_t> const& synapse_column_size) SYMBOL_VISIBLE;

	std::string const& get_kernel() const SYMBOL_VISIBLE;
	Timer const& get_timer() const SYMBOL_VISIBLE;

	constexpr static bool variadic_input = false;
	std::vector<Port> inputs() const SYMBOL_VISIBLE;

	Port output() const SYMBOL_VISIBLE;

	friend std::ostream& operator<<(std::ostream& os, PlasticityRule const& config) SYMBOL_VISIBLE;

	bool supports_input_from(
	    SynapseArrayView const& input,
	    std::optional<PortRestriction> const& restriction) const SYMBOL_VISIBLE;

	bool operator==(PlasticityRule const& other) const SYMBOL_VISIBLE;
	bool operator!=(PlasticityRule const& other) const SYMBOL_VISIBLE;

private:
	std::string m_kernel;
	Timer m_timer;
	std::vector<size_t> m_synapse_column_size;

	friend class cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex

} // grenade::vx
