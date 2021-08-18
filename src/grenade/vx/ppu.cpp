#include "grenade/vx/ppu.h"

#include "halco/hicann-dls/vx/v2/neuron.h"
#include "halco/hicann-dls/vx/v2/quad.h"
#include "halco/hicann-dls/vx/v2/synapse.h"

#include <filesystem>

namespace grenade::vx {

haldls::vx::v2::PPUMemoryBlock to_vector_unit_row(
    halco::common::typed_array<int8_t, halco::hicann_dls::vx::v2::NeuronColumnOnDLS> const& values)
{
	using namespace haldls::vx::v2;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;

	PPUMemoryBlock block(
	    PPUMemoryBlockSize(NeuronColumnOnDLS::size / sizeof(PPUMemoryWord::raw_type)));

	for (auto coord : iter_all<SynapseOnSynapseRow>()) {
		auto const quad = coord.toSynapseQuadColumnOnDLS();
		auto const entry = coord.toEntryOnQuad();
		PPUMemoryWord::raw_type word = block.at(quad).get_value();
		int8_t* byte = reinterpret_cast<int8_t*>(&word);
		*(byte + entry) = values.at(coord.toNeuronColumnOnDLS());
		block.at(quad) = PPUMemoryWord(PPUMemoryWord::Value(word));
	}
	return block;
}

halco::common::typed_array<int8_t, halco::hicann_dls::vx::v2::NeuronColumnOnDLS>
from_vector_unit_row(haldls::vx::v2::PPUMemoryBlock const& values)
{
	using namespace haldls::vx::v2;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v2;

	if (values.size() != NeuronColumnOnDLS::size / sizeof(PPUMemoryWord::raw_type)) {
		throw std::runtime_error("Trying to convert values to vector-unit row of wrong size.");
	}

	halco::common::typed_array<int8_t, NeuronColumnOnDLS> bytes;

	for (auto quad : iter_all<SynapseQuadColumnOnDLS>()) {
		PPUMemoryWord::raw_type word = values.at(quad).get_value();
		int8_t* byte = reinterpret_cast<int8_t*>(&word);
		auto const neurons = quad.toNeuronColumnOnDLS();
		for (auto entry : iter_all<EntryOnQuad>()) {
			bytes.at(neurons.at(entry)) = *(byte + (EntryOnQuad::max - entry));
		}
	}

	return bytes;
}

std::string get_program_path(std::string const& name)
{
	char const* env_path = std::getenv("PATH");
	if (!env_path) {
		throw std::runtime_error("No PATH variable found.");
	}
	std::istringstream path(env_path);
	std::string p;
	while (std::getline(path, p, ':')) {
		std::filesystem::path fp(p);
		fp /= name;
		if (std::filesystem::exists(fp)) {
			return fp;
		}
	}
	throw std::runtime_error(std::string("Specified PPU program (") + name + ") not found.");
}

} // namespace grenade::vx
