#pragma once

#include "dapr/property.h"
#include "grenade/common/vertex.h"
#include "grenade/vx/network/routing/csp/tracer.h"

namespace grenade::vx::network::routing::csp {

struct RoutingGraph;

/**
 * Abstract routing vertex.
 */
struct SYMBOL_VISIBLE RoutingVertex : public dapr::Property<RoutingVertex>
{
	/**
	 * Destruct a vertex.
	 */
	virtual ~RoutingVertex();

	/**
	 * Get name of vertex.
	 */
	virtual std::string get_name() const = 0;

	/**
	 * Get all parameters of vertex.
	 * This function is used in gathering parameters for branching.
	 */
	virtual Gecode::IntVarArgs get_parameters() const = 0;

	/**
	 * Get names of parameters.
	 */
	virtual std::vector<std::string> get_parameter_names() const = 0;

	/**
	 * Add tracing for vertex parameters.
	 */
	void trace(Gecode::Space& space);

	/**
	 * Printing method required by property and used in ostream call.
	 */
	std::ostream& print(std::ostream& os) const;

protected:
	// How many bits should be used for printing the parameters in binary form
	virtual std::vector<size_t> get_parameter_bitcounts() const = 0;

private:
	friend RoutingGraph;
	/**
	 * Update vertex for another search space.
	 * This function is used in copying a search space.
	 */
	virtual void update(Gecode::Space& space, RoutingVertex& other) = 0;

private:
	std::vector<std::shared_ptr<IntTracer>> m_tracers;
};

} // namespace grenade::vx::network::routing::csp