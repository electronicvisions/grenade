#include "grenade/vx/ppu/detail/status.h"

#ifndef __ppu__
#include <ostream>

namespace grenade::vx::ppu::detail {

std::ostream& operator<<(std::ostream& os, Status const& value)
{
	switch (value) {
		case Status::initial:
			return os << "initial";
		case Status::idle:
			return os << "idle";
		case Status::reset_neurons:
			return os << "reset_neurons";
		case Status::baseline_read:
			return os << "baseline_read";
		case Status::read:
			return os << "read";
		case Status::periodic_read:
			return os << "periodic_read";
		case Status::inside_periodic_read:
			return os << "inside_periodic_read";
		case Status::stop_periodic_read:
			return os << "stop_periodic_read";
		case Status::stop:
			return os << "stop";
		case Status::scheduler:
			return os << "scheduler";
		default:
			break;
	}
	throw std::logic_error("Status not implemented.");
}

} // namespace grenade::vx::ppu::detail
#endif
