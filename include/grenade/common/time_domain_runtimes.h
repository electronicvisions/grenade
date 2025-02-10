#pragma once
#include "dapr/property.h"
#include "grenade/common/genpybind.h"
#include <cstddef>

namespace grenade {
namespace common GENPYBIND_TAG_GRENADE_COMMON {

/**
 * Runtimes of vertices on a time domain.
 */
struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) TimeDomainRuntimes
    : public dapr::Property<TimeDomainRuntimes>
{
	virtual ~TimeDomainRuntimes();

	/**
	 * Number of batch entries for which runtime is given.
	 */
	virtual size_t batch_size() const = 0;
};

} // namespace common
} // namespace grenade
