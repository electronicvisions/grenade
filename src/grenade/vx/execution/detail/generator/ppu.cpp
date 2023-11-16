#include "grenade/vx/execution/detail/generator/ppu.h"

#include "halco/common/iter_all.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "haldls/vx/v3/block.h"
#include "haldls/vx/v3/ppu.h"

namespace grenade::vx::execution::detail::generator {

using namespace haldls::vx::v3;
using namespace stadls::vx::v3;
using namespace halco::hicann_dls::vx::v3;
using namespace halco::common;

stadls::vx::PlaybackGeneratorReturn<PPUCommand::Builder, PPUCommand::Result> PPUCommand::generate()
    const
{
	Builder builder;
	Result current_time = Result(0);
	// write command to be executed
	for (auto const ppu : iter_all<PPUOnDLS>()) {
		PPUMemoryWord config(PPUMemoryWord::Value(static_cast<uint32_t>(m_status)));
		builder.write(current_time, PPUMemoryWordOnDLS(m_coord, ppu), config);
		current_time += Timer::Value(2);
	}
	return {std::move(builder), current_time};
}

} // namespace grenade::vx::execution::detail::generator
