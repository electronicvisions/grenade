#pragma once

#include "grenade/vx/network/routing/csp/routing_vertex.h"
#include "halco/hicann-dls/vx/v3/event.h"

namespace grenade::vx::network::routing::csp {

/**
 * Source vertex.
 *
 * Defines the number of labels available for routing. Each label is represented by an integer.
 */
struct SYMBOL_VISIBLE Source : public RoutingVertex
{
	Source(size_t number_label_bits, size_t num_labels, Gecode::Space& space);
	~Source();

	/**
	 * Get name of source.
	 */
	std::string get_name() const;
	/**
	 * Update source for another search space.
	 * This function is used in copying a search space.
	 */
	void update(Gecode::Space& space, RoutingVertex& other);

	/**
	 * Get all parameters of source.
	 * This function is used in gathering parameters for branching.
	 */
	Gecode::IntVarArgs get_parameters() const;

	/**
	 * Get names of parameters.
	 */
	std::vector<std::string> get_parameter_names() const;

	/**
	 * Number of source labels.
	 */
	size_t size() const;

	/**
	 * Get source label at specified index.
	 * @param index Index to get label for
	 * @throws std::out_of_range On index out of range [0, size())
	 */
	Gecode::IntVar const& get_label(size_t index) const;

	/**
	 * Get assigned source label at specified index.
	 * This function only works when the specified label is assigned, i.e. contains exactly a single
	 * value.
	 * @param index Index to get label for
	 * @throws std::out_of_range On index out of range [0, size())
	 */
	halco::hicann_dls::vx::v3::SpikeLabel get_assigned_label(size_t index) const;

	std::unique_ptr<RoutingVertex> copy() const;
	std::unique_ptr<RoutingVertex> move();

protected:
	// How many bits should be used for printing the parameters in binary form
	std::vector<size_t> get_parameter_bitcounts() const;

private:
	bool is_equal_to(RoutingVertex const& other) const;

	std::vector<Gecode::IntVar> m_labels;

	size_t m_number_label_bits;
};

} // namespace grenade::vx::network::routing::csp
