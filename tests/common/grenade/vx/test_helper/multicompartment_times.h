#pragma once

namespace grenade::vx::network::abstract {
struct Times
{
	double validation;
	double empty;
	double construction;
	double num_compartments;
	double num_connections;
	double isomorphism;
	double resources;
};
} // namespace grenade::vx::network::abstract