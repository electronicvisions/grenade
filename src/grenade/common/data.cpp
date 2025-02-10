#include "grenade/common/data.h"

#include "grenade/common/port_data/batched.h"
#include "grenade/common/topology.h"
#include "hate/indent.h"
#include "hate/join.h"
#include <stdexcept>
#include <log4cxx/logger.h>

namespace grenade::common {

bool Data::has_homogeneous_batch_size() const
{
	if (!ports.empty()) {
		std::set<size_t> batch_sizes;
		for (auto const& [_, port] : ports) {
			if (auto const batched_port_data = dynamic_cast<BatchedPortData const*>(&port);
			    batched_port_data) {
				batch_sizes.insert(batched_port_data->batch_size());
			}
		}
		if (batch_sizes.size() > 1) {
			return false;
		}
	}
	return true;
}


size_t Data::batch_size() const
{
	if (!has_homogeneous_batch_size()) {
		throw std::runtime_error("Batch size not homogeneous.");
	}
	for (auto const& [_, port] : ports) {
		if (auto const batched_port_data = dynamic_cast<BatchedPortData const*>(&port);
		    batched_port_data) {
			return batched_port_data->batch_size();
		}
	}
	return 0;
}

std::ostream& operator<<(std::ostream& os, Data const& data)
{
	hate::IndentingOstream ios(os);
	ios << "Data(\n";
	ios << hate::Indentation("\t");
	for (auto const& [p, d] : data.ports) {
		ios << p.first << ", " << p.second << ": " << d << "\n";
	}
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::common
