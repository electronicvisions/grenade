#include "helper.h"

#include "hxcomm/common/hwdb_entry.h"


using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;
using namespace lola::vx::v3;
using namespace haldls::vx::v3;


bool is_jboa_setup_of_size(grenade::vx::execution::JITGraphExecutor const& executor, size_t size)
{
	auto hwdb_entries = executor.get_hwdb_entry();
	bool all_jboa =
	    std::all_of(hwdb_entries.begin(), hwdb_entries.end(), [](auto const& key_value) {
		    for (auto const& hwdb_entry : key_value.second) {
			    if (!std::holds_alternative<hwdb4cpp::JboaSetupEntry>(hwdb_entry)) {
				    return false;
			    }
		    }
		    return true;
	    });
	if (size != 0) {
		for (auto const& [_, connection_size] : executor.connection_sizes()) {
			if (connection_size != size) {
				return false;
			}
		}
	}
	return all_jboa;
}
