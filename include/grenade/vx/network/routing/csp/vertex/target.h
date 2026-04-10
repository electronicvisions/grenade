#pragma once

#include "grenade/vx/network/routing/csp/routing_vertex.h"
#include "hate/visibility.h"

namespace grenade::vx::network::routing::csp {

/**
 * Target vertex.
 *
 * Does not constrain the number of possible target indices.
 */
struct SYMBOL_VISIBLE Target : public RoutingVertex
{
	~Target();

	/**
	 * Get name of target.
	 */
	std::string get_name() const;

	/**
	 * Update target for another search space.
	 * This function is used in copying a search space.
	 */
	void update(Gecode::Space& space, RoutingVertex& other);

	/**
	 * Get all parameters of target.
	 * This function is used in gathering parameters for branching.
	 */
	Gecode::IntVarArgs get_parameters() const;

	/**
	 * Get names of parameters.
	 */
	std::vector<std::string> get_parameter_names() const;


	std::unique_ptr<RoutingVertex> copy() const;
	std::unique_ptr<RoutingVertex> move();

protected:
	// How many bits should be used for printing the parameters in binary form
	std::vector<size_t> get_parameter_bitcounts() const;

private:
	bool is_equal_to(RoutingVertex const& other) const;
};

} // namespace grenade::vx::network::routing::csp
