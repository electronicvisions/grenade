#pragma once

namespace grenade::vx::network::abstract {

struct TestResult
{
	size_t num_compartments;
	bool success;
	double time_total;
	double time_generation;
	double time_placement;
	double space_efficiency;
};


} // namespace grenade::vx::network::abstract