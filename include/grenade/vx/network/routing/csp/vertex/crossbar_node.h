#pragma once
#include "grenade/vx/network/routing/csp/vertex/filter.h"
#include "hate/visibility.h"
#include <gecode/int.hh>

namespace grenade::vx::network::routing::csp {

/**
 * Crossbar node.
 *
 * Applies a filter to incoming events by applying a mask to there label and comparing to a target
 * label.
 */
struct SYMBOL_VISIBLE CrossbarNode : public Filter
{
	/**
	 * Construct crossbar node.
	 * @param bit_count Number of bits used for mask and target of the crossbar node.
	 * @param space Gecode space in which mask and target are managed.
	 */
	CrossbarNode(size_t bit_count, Gecode::Space& space);

	~CrossbarNode();

	/**
	 * Get name of crossbar-node.
	 */
	std::string get_name() const;

	/**
	 * Update crossbar-node for another search space.
	 * This function is used in copying a search space.
	 */
	void update(Gecode::Space& space, RoutingVertex& other);

	/**
	 * Get all parameters of crossbar-node.
	 * This function is used in gathering parameters for branching.
	 */
	Gecode::IntVarArgs get_parameters() const;

	/**
	 * Get names of parameters.
	 */
	std::vector<std::string> get_parameter_names() const;

	/**
	 * Applies a filter to a input value.
	 * @param space Space in which return value is contained.
	 * @param value Value on which filter is applied.
	 * @return Boolean value, describing if event passes through filter.
	 */
	Gecode::BoolVar apply_filter(Gecode::Home space, Gecode::IntVar value);

	std::unique_ptr<RoutingVertex> copy() const;
	std::unique_ptr<RoutingVertex> move();

	Gecode::IntVar mask;
	Gecode::IntVar target;

protected:
	// How many bits should be used for printing the parameters in binary form
	std::vector<size_t> get_parameter_bitcounts() const;

private:
	bool is_equal_to(RoutingVertex const& other) const;

	size_t m_bit_count;
};

} // namespace grenade::vx::network::routing::csp
