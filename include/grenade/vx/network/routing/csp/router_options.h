#pragma once
#include "hate/visibility.h"
#include <iosfwd>

namespace grenade::vx::network::routing::csp {

/**
 * Options for CSP router.
 */
struct SYMBOL_VISIBLE RouterOptions
{
	/**
	 * Options for search algorithm.
	 */
	struct Search
	{
		Search() = default;

		/**
		 * DFS search creates a copy after this number of commits.
		 * Lower number increases memory consumption but decreases required computation during
		 * backtracking.
		 */
		unsigned int commit_distance{200};

		/**
		 * DFS search creates a copy during recomputation if distance is larger than this
		 * number. Setting this to the same or a larger number than `commit_distance` disables
		 * recomputation.
		 */
		unsigned int adaptive_distance{200};

		friend std::ostream& operator<<(std::ostream& os, Search const& options) SYMBOL_VISIBLE;
	} search;

	RouterOptions();

	friend std::ostream& operator<<(std::ostream& os, RouterOptions const& options);
};

} // namespace grenade::vx::network::routing::csp