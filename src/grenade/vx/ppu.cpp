#include "grenade/vx/ppu.h"

#include "halco/hicann-dls/vx/v3/neuron.h"
#include "halco/hicann-dls/vx/v3/quad.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "hate/join.h"
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <boost/compute/detail/sha1.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx {

haldls::vx::v3::PPUMemoryBlock to_vector_unit_row(
    halco::common::typed_array<int8_t, halco::hicann_dls::vx::v3::NeuronColumnOnDLS> const& values)
{
	using namespace haldls::vx::v3;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;

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

halco::common::typed_array<int8_t, halco::hicann_dls::vx::v3::NeuronColumnOnDLS>
from_vector_unit_row(haldls::vx::v3::PPUMemoryBlock const& values)
{
	using namespace haldls::vx::v3;
	using namespace halco::common;
	using namespace halco::hicann_dls::vx::v3;

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

TemporaryDirectory::TemporaryDirectory(std::string directory_template)
{
	std::string tmp = directory_template;
	auto ret = mkdtemp(tmp.data());
	if (ret == nullptr) {
		throw std::runtime_error("Temporary directory creation failed.");
	}
	m_path = tmp;
}

namespace {

std::string get_include_path(
    std::string const& name, std::filesystem::path const& location_during_build)
{
	// assume, that include paths are found under:
	// - after install: PATH/../<location_after_install>
	// - during build:  PATH/../<location_during_build>
	// TODO (Issue #3990): Implement properly without magic assumptions
	char const* env_path = std::getenv("PATH");
	if (!env_path) {
		throw std::runtime_error("No PATH variable found.");
	}
	std::istringstream path(env_path);
	std::string p;
	std::vector<std::string> include_paths;
	while (std::getline(path, p, ':')) {
		if (p == "") {
			continue;
		}
		{
			std::filesystem::path fp(p);
			if (fp.parent_path().filename() == "build") {
				fp = fp.parent_path().parent_path();
				fp /= name;
				fp /= location_during_build;
				if (std::filesystem::exists(fp / name)) {
					include_paths.push_back("-I" + static_cast<std::string>(fp));
				}
			}
		}
		{
			std::filesystem::path fp(p);
			fp = fp.parent_path();
			fp /= "include";
			if (std::filesystem::exists(fp / name)) {
				include_paths.push_back("-I" + static_cast<std::string>(fp));
			}
		}
	}
	if (include_paths.empty()) {
		throw std::runtime_error("Include path for " + name + " not found.");
	}
	return hate::join_string(include_paths, " ");
}

} // namespace

std::string get_include_paths()
{
	std::set<std::string> include_paths;
	include_paths.insert(get_include_path("libnux", "include"));
	include_paths.insert(get_include_path("grenade", "include"));
	include_paths.insert(get_include_path("hate", "include"));
	include_paths.insert(get_include_path("haldls", "include"));
	include_paths.insert(get_include_path("fisch", "include"));
	include_paths.insert(get_include_path("halco", "include"));
	include_paths.insert(get_include_path("rant", ""));
	include_paths.insert(get_include_path("pywrap", "src"));
	include_paths.insert(get_include_path("boost", ""));
	return hate::join_string(include_paths, " ");
}

namespace {

std::string get_library_path(
    std::string const& name,
    std::filesystem::path const& location_after_install,
    std::filesystem::path const& location_during_build)
{
	// assume, that include paths are found under:
	// - after install: PATH/../<location_after_install>
	// - during build:  PATH/../<location_during_build>
	// TODO (Issue #3990): Implement properly without magic assumptions
	char const* env_path = std::getenv("PATH");
	if (!env_path) {
		throw std::runtime_error("No PATH variable found.");
	}
	std::istringstream path(env_path);
	std::string p;
	while (std::getline(path, p, ':')) {
		if (p == "") {
			continue;
		}
		{
			std::filesystem::path fp(p);
			fp = fp.parent_path();
			fp /= location_after_install;
			if (std::filesystem::exists(fp / name)) {
				return "-L" + static_cast<std::string>(fp);
			}
		}
		{
			std::filesystem::path fp(p);
			fp = fp.parent_path();
			fp /= location_during_build;
			if (std::filesystem::exists(fp / name)) {
				return "-L" + static_cast<std::string>(fp);
			}
		}
	}
	throw std::runtime_error("Library path for " + name + " not found.");
}

} // namespace

std::string get_library_paths()
{
	std::set<std::string> library_paths;
	library_paths.insert(get_library_path("libnux_vx_v3.a", "lib", "libnux"));
	library_paths.insert(
	    get_library_path("libfisch_ppu_vx.a", std::filesystem::path("lib") / "ppu", "fisch"));
	library_paths.insert(
	    get_library_path("libhaldls_ppu_vx_v3.a", std::filesystem::path("lib") / "ppu", "haldls"));
	library_paths.insert(get_library_path(
	    "libhalco_hicann_dls_ppu_vx_v3.a", std::filesystem::path("lib") / "ppu", "halco"));
	return hate::join_string(library_paths, " ");
}

std::string get_linker_file(std::string const& name)
{
	// assume, that linker script is found under:
	// - after install: PATH/../share/libnux
	// - during build:  PATH/../../libnux/share/libnux
	// TODO (Issue #3990): Implement properly without magic assumptions
	char const* env_path = std::getenv("PATH");
	if (!env_path) {
		throw std::runtime_error("No PATH variable found.");
	}
	std::istringstream path(env_path);
	std::string p;
	while (std::getline(path, p, ':')) {
		if (p == "") {
			continue;
		}
		{
			std::filesystem::path fp(p);
			fp = fp.parent_path();
			fp /= "share";
			fp /= "libnux";
			fp /= name;
			if (std::filesystem::exists(fp)) {
				return fp;
			}
		}
		{
			std::filesystem::path fp(p);
			fp = fp.parent_path();
			fp = fp.parent_path();
			fp /= "libnux";
			fp /= "share";
			fp /= "libnux";
			fp /= name;
			if (std::filesystem::exists(fp)) {
				return fp;
			}
		}
	}
	throw std::runtime_error(std::string("Specified linker script (") + name + ") not found.");
}

std::string get_libnux_runtime(std::string const& name)
{
	// assume, that libnux runtime is found under:
	// - after install: PATH/../lib
	// - during build:  PATH/../libnux
	// TODO (Issue #3990): Implement properly without magic assumptions
	char const* env_path = std::getenv("PATH");
	if (!env_path) {
		throw std::runtime_error("No PATH variable found.");
	}
	std::istringstream path(env_path);
	std::string p;
	while (std::getline(path, p, ':')) {
		if (p == "") {
			continue;
		}
		{
			std::filesystem::path fp(p);
			fp = fp.parent_path();
			fp /= "lib";
			fp /= name;
			if (std::filesystem::exists(fp)) {
				return fp;
			}
		}
		{
			std::filesystem::path fp(p);
			fp = fp.parent_path();
			fp /= "libnux";
			fp /= name;
			if (std::filesystem::exists(fp)) {
				return fp;
			}
		}
	}
	throw std::runtime_error(std::string("Specified libnux runtime (") + name + ") not found.");
}

std::string get_program_base_source()
{
	// assume, that program source is found under:
	// - after install: PATH/../share/grenade/base.cpp
	// - during build:  PATH/../../grenade/src/grenade/vx/start.cpp
	// TODO (Issue #3990): Implement properly without magic assumptions
	char const* env_path = std::getenv("PATH");
	if (!env_path) {
		throw std::runtime_error("No PATH variable found.");
	}
	std::istringstream path(env_path);
	std::string p;
	while (std::getline(path, p, ':')) {
		if (p == "") {
			continue;
		}
		{
			std::filesystem::path fp(p);
			fp = fp.parent_path();
			fp /= "share";
			fp /= "grenade";
			fp /= "base.cpp";
			if (std::filesystem::exists(fp)) {
				return fp;
			}
		}
		{
			std::filesystem::path fp(p);
			fp = fp.parent_path();
			fp = fp.parent_path();
			fp /= "grenade";
			fp /= "src";
			fp /= "grenade";
			fp /= "vx";
			fp /= "ppu";
			fp /= "start.cpp";
			if (std::filesystem::exists(fp)) {
				return fp;
			}
		}
	}
	throw std::runtime_error("PPU program base source not found.");
}

std::filesystem::path TemporaryDirectory::get_path() const
{
	return m_path;
}

TemporaryDirectory::~TemporaryDirectory()
{
	std::filesystem::remove_all(m_path);
}

Compiler::Compiler() {}

std::pair<lola::vx::v3::PPUElfFile::symbols_type, lola::vx::v3::PPUElfFile::Memory>
Compiler::compile(std::vector<std::string> sources)
{
	auto logger = log4cxx::Logger::getLogger("grenade.Compiler");
	TemporaryDirectory temporary("grenade-compiler-XXXXXX");

	// save sources to files for compiler subprocess to use
	std::vector<std::string> source_paths;
	for (size_t i = 0; auto const& source : sources) {
		auto const path = temporary.get_path() / ("source_" + std::to_string(i) + ".cpp");
		{
			std::ofstream fs(path);
			fs << source;
		}
		i++;
		source_paths.push_back(path);
	}
	source_paths.push_back(get_program_base_source());

	std::stringstream ss;
	ss << "cd " << temporary.get_path() << " && ";
	ss << name << " "
	   << hate::join_string(options_before_source.begin(), options_before_source.end(), " ") << " ";
	for (auto const& source : source_paths) {
		std::filesystem::path path(source);
		if (path.is_absolute()) {
			ss << path << " ";
		} else {
			ss << "../" / path << " ";
		}
	}
	ss << hate::join_string(options_after_source.begin(), options_after_source.end(), " ") << " -o"
	   << "program.bin";
	ss << " > "
	   << "compile_log"
	   << " 2>&1";
	LOG4CXX_DEBUG(logger, "compile(): Command: " << ss.str());
	auto ret = std::system(ss.str().c_str());
	std::stringstream log;
	{
		log << std::ifstream(temporary.get_path() / "compile_log").rdbuf();
	}
	if (ret != 0) {
		throw std::runtime_error("Compilation failed:\n" + log.str());
	}
	LOG4CXX_DEBUG(logger, "compile(): Output:\n" << log.str());
	if (logger->isDebugEnabled()) {
		ret = std::system(("powerpc-ppu-readelf -a " +
		                   std::string(temporary.get_path() / "program.bin") + " > " +
		                   std::string(temporary.get_path() / "readelf_log") + " 2>&1")
		                      .c_str());
		{
			std::stringstream log;
			{
				log << std::ifstream(temporary.get_path() / "readelf_log").rdbuf();
			}
			if (ret != 0) {
				throw std::runtime_error("Readelf failed:\n" + log.str());
			}
			LOG4CXX_DEBUG(logger, "compile(): Readelf:\n" << log.str());
		}
		{
			std::stringstream log;
			for (auto const& source_path : source_paths) {
				auto path = std::filesystem::path(source_path).replace_extension("su");
				log << std::ifstream(path).rdbuf();
			}
			LOG4CXX_DEBUG(logger, "compile(): Stack usage:\n" << log.str());
		}
	}
	if (logger->isTraceEnabled()) {
		ret = std::system(("powerpc-ppu-objdump -Mnux -d " +
		                   std::string(temporary.get_path() / "program.bin") + " > " +
		                   std::string(temporary.get_path() / "objdump_log") + " 2>&1")
		                      .c_str());
		{
			std::stringstream log;
			{
				log << std::ifstream(temporary.get_path() / "objdump_log").rdbuf();
			}
			if (ret != 0) {
				throw std::runtime_error("Objdump failed:\n" + log.str());
			}
			LOG4CXX_TRACE(logger, "compile(): Objdump:\n" << log.str());
		}
	}
	lola::vx::v3::PPUElfFile elf_file(temporary.get_path() / "program.bin");
	return {elf_file.read_symbols(), elf_file.read_program()};
}


std::string CachingCompiler::ProgramCache::Source::sha1() const
{
	boost::compute::detail::sha1 digestor;
	for (auto const& option : options_before_source) {
		digestor.process(option);
	}
	for (auto const& option : options_after_source) {
		digestor.process(option);
	}
	for (auto const& source : source_codes) {
		digestor.process(source);
	}
	return static_cast<std::string>(digestor);
}

CachingCompiler::ProgramCache& CachingCompiler::get_program_cache()
{
	static ProgramCache data;
	return data;
}

std::pair<lola::vx::v3::PPUElfFile::symbols_type, lola::vx::v3::PPUElfFile::Memory>
CachingCompiler::compile(std::vector<std::string> sources)
{
	auto& program_cache = get_program_cache();
	std::lock_guard lock(program_cache.data_mutex);
	ProgramCache::Source source;
	source.options_before_source = options_before_source;
	source.options_after_source = options_after_source;
	source.source_codes = sources;
	auto const sha1 = source.sha1();
	if (program_cache.data.contains(sha1)) {
		return program_cache.data.at(sha1);
	} else {
		auto const program = static_cast<Compiler*>(this)->compile(sources);
		program_cache.data[sha1] = program;
		return program;
	}
}

} // namespace grenade::vx
