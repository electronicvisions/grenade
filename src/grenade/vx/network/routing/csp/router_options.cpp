#include "grenade/vx/network/routing/csp/router_options.h"

#include "hate/indent.h"
#include <sstream>

namespace grenade::vx::network::routing::csp {


std::ostream& operator<<(std::ostream& os, RouterOptions::Search const& options)
{
	return os << "Search(commit_distance: " << options.commit_distance
	          << ", adaptive_distance: " << options.adaptive_distance << ")";
}

RouterOptions::RouterOptions() : search() {}

std::ostream& operator<<(std::ostream& os, RouterOptions const& options)
{
	std::stringstream search;
	search << options.search;


	hate::IndentingOstream ios(os);
	ios << "RouterOptions(\n" << hate::Indentation("\t") << search.str() << "\n";
	ios << hate::Indentation("\t") << ")";
	return os;
}

} // namespace grenade::vx::network::routing::csp