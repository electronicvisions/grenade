#include "grenade/vx/execution/backend/detail/stateful_chip_config.h"

#include "haldls/vx/v3/omnibus_constants.h"
#include "lola/vx/v3/ppu.h"
#include "stadls/visitors.h"

namespace grenade::vx::execution::backend::detail {

StatefulChipConfig::StatefulChipConfig(System system) : m_last_system(std::move(system)) {}


bool StatefulChipConfig::get_enable_differential_config() const
{
	return m_enable_differential_config;
}

void StatefulChipConfig::set_enable_differential_config(bool const value)
{
	m_enable_differential_config = value;
}

void StatefulChipConfig::reset()
{
	if (std::holds_alternative<lola::vx::v3::ChipAndMultichipJboaLeafFPGA>(m_last_system)) {
		m_last_system = lola::vx::v3::ChipAndMultichipJboaLeafFPGA();
	} else if (std::holds_alternative<lola::vx::v3::ChipAndSinglechipFPGA>(m_last_system)) {
		m_last_system = lola::vx::v3::ChipAndSinglechipFPGA();
	} else {
		throw std::logic_error("Invalid system tpye.");
	}

	m_system_words.clear();
	m_last_system_words.clear();
	m_system_base_words.clear();
	m_system_differential_addresses.clear();
	m_system_base_addresses.clear();
	m_system_differential_addresses.clear();

	m_external_ppu_dram_memory_words.clear();
	m_external_ppu_dram_memory_addresses.clear();
	m_last_external_ppu_dram_memory_words.clear();
	m_external_ppu_dram_memory_base_words.clear();
	m_external_ppu_dram_memory_differential_words.clear();
	m_external_ppu_dram_memory_base_addresses.clear();
	m_external_ppu_dram_memory_differential_addresses.clear();
}

namespace {

/**
 * Helper to only encode the constant addresses of the lola single chip system object once.
 */
static const std::vector<halco::hicann_dls::vx::OmnibusAddress> cube_system_addresses = []() {
	using namespace halco::hicann_dls::vx::v3;
	typedef std::vector<OmnibusAddress> addresses_type;
	addresses_type storage;
	hate::Empty<lola::vx::v3::ChipAndSinglechipFPGA> config;
	haldls::vx::visit_preorder(
	    config, ChipAndSinglechipFPGAOnSystem(),
	    stadls::WriteAddressVisitor<addresses_type>{storage});
	return storage;
}();

/**
 * Helper to only encode the constant addresses of the lola multi chip system object once.
 */
static const std::vector<halco::hicann_dls::vx::OmnibusAddress> jboa_system_addresses = []() {
	using namespace halco::hicann_dls::vx::v3;
	typedef std::vector<OmnibusAddress> addresses_type;
	addresses_type storage;
	hate::Empty<lola::vx::v3::ChipAndMultichipJboaLeafFPGA> config;
	haldls::vx::visit_preorder(
	    config, ChipAndMultichipJboaLeafFPGAOnSystem(),
	    stadls::WriteAddressVisitor<addresses_type>{storage});
	return storage;
}();

} // namespace

void StatefulChipConfig::set_system(System const& value, bool split_base_differential)
{
	std::vector<halco::hicann_dls::vx::OmnibusAddress> const& system_addresses =
	    [this, &value]() -> std::vector<halco::hicann_dls::vx::OmnibusAddress> const& {
		if (std::holds_alternative<lola::vx::v3::ChipAndMultichipJboaLeafFPGA>(value)) {
			if (!std::holds_alternative<lola::vx::v3::ChipAndMultichipJboaLeafFPGA>(
			        m_last_system)) {
				throw std::logic_error("Last system and new system are of different type.");
			}
			return jboa_system_addresses;
		} else if (std::holds_alternative<lola::vx::v3::ChipAndSinglechipFPGA>(value)) {
			if (!std::holds_alternative<lola::vx::v3::ChipAndSinglechipFPGA>(m_last_system)) {
				throw std::logic_error("Last system and new system are of different type.");
			}
			return cube_system_addresses;
		} else {
			throw std::logic_error("Invalid system type.");
		}
	}();


	auto const encode_value = [&value, this]() {
		std::swap(m_last_system_words, m_system_words);

		m_system_words.clear();
		m_system_words.reserve(jboa_system_addresses.size());

		if (std::holds_alternative<lola::vx::v3::ChipAndMultichipJboaLeafFPGA>(value)) {
			haldls::vx::visit_preorder(
			    std::get<lola::vx::v3::ChipAndMultichipJboaLeafFPGA>(value),
			    hate::Empty<lola::vx::v3::ChipAndMultichipJboaLeafFPGA::coordinate_type>(),
			    stadls::EncodeVisitor<Words>{m_system_words});
		} else if (std::holds_alternative<lola::vx::v3::ChipAndSinglechipFPGA>(value)) {
			haldls::vx::visit_preorder(
			    std::get<lola::vx::v3::ChipAndSinglechipFPGA>(value),
			    hate::Empty<lola::vx::v3::ChipAndSinglechipFPGA::coordinate_type>(),
			    stadls::EncodeVisitor<Words>{m_system_words});
		} else {
			throw std::logic_error("Invalid system type.");
		}
	};

	if (split_base_differential) {
		if (!m_enable_differential_config) {
			encode_value();
			m_system_differential_words.clear();
			m_system_differential_addresses.clear();

			m_system_base_addresses = system_addresses;
			m_system_base_words = m_system_words;
		} else if (get_is_fresh()) {
			encode_value();
			assert(m_system_base_words.empty());
			assert(m_system_differential_words.empty());
			assert(m_system_base_addresses.empty());
			assert(m_system_differential_addresses.empty());

			m_system_base_addresses = system_addresses;
			m_system_base_words = m_system_words;
			m_last_system = value;
		} else {
			if (m_last_system != value) {
				encode_value();
				assert(m_system_words.size() == system_addresses.size());
				assert(m_last_system_words.size() == system_addresses.size());

				m_system_base_words.clear();
				m_system_differential_words.clear();
				m_system_base_addresses.clear();
				m_system_differential_addresses.clear();
				for (size_t i = 0; i < system_addresses.size(); ++i) {
					if (m_last_system_words[i] != m_system_words[i]) {
						m_system_differential_addresses.push_back(system_addresses[i]);
						m_system_differential_words.push_back(m_system_words[i]);
					} else {
						m_system_base_addresses.push_back(system_addresses[i]);
						m_system_base_words.push_back(m_system_words[i]);
					}
				}
				m_last_system = value;
			} else {
				m_last_system_words = m_system_words;
			}
		}
	} else {
		if (m_last_system != value) {
			encode_value();
			m_last_system = value;
		} else {
			m_last_system_words = m_system_words;
		}
	}
}

void StatefulChipConfig::set_external_ppu_dram_memory(
    std::optional<lola::vx::v3::ExternalPPUDRAMMemoryBlock> const& value)
{
	m_last_external_ppu_dram_memory = m_external_ppu_dram_memory;
	m_external_ppu_dram_memory = value;
	m_last_external_ppu_dram_memory_words.clear();
	for (size_t i = 0; i < m_external_ppu_dram_memory_addresses.size(); ++i) {
		m_last_external_ppu_dram_memory_words.emplace(
		    m_external_ppu_dram_memory_addresses[i], m_external_ppu_dram_memory_words[i]);
	}

	m_external_ppu_dram_memory_words.clear();
	m_external_ppu_dram_memory_addresses.clear();
	if (m_external_ppu_dram_memory) {
		halco::hicann_dls::vx::v3::ExternalPPUDRAMMemoryBlockOnFPGA const coord(
		    halco::hicann_dls::vx::v3::ExternalPPUDRAMMemoryByteOnFPGA(0),
		    halco::hicann_dls::vx::v3::ExternalPPUDRAMMemoryByteOnFPGA(
		        m_external_ppu_dram_memory->size() - 1));
		haldls::vx::visit_preorder(
		    *m_external_ppu_dram_memory, coord,
		    stadls::EncodeVisitor<Words>{m_external_ppu_dram_memory_words});

		hate::Empty<lola::vx::v3::ExternalPPUDRAMMemoryBlock> empty_config;
		haldls::vx::visit_preorder(
		    empty_config, coord,
		    stadls::WriteAddressVisitor<Addresses>{m_external_ppu_dram_memory_addresses});
	}

	if (!m_enable_differential_config) {
		m_external_ppu_dram_memory_differential_words.clear();
		m_external_ppu_dram_memory_differential_addresses.clear();

		m_external_ppu_dram_memory_base_addresses = m_external_ppu_dram_memory_addresses;
		m_external_ppu_dram_memory_base_words = m_external_ppu_dram_memory_words;
	} else if (get_is_fresh()) {
		assert(m_external_ppu_dram_memory_base_words.empty());
		assert(m_external_ppu_dram_memory_differential_words.empty());
		assert(m_external_ppu_dram_memory_base_addresses.empty());
		assert(m_external_ppu_dram_memory_differential_addresses.empty());

		m_external_ppu_dram_memory_base_addresses = m_external_ppu_dram_memory_addresses;
		m_external_ppu_dram_memory_base_words = m_external_ppu_dram_memory_words;
	} else {
		assert(
		    m_external_ppu_dram_memory_words.size() == m_external_ppu_dram_memory_addresses.size());

		m_external_ppu_dram_memory_base_words.clear();
		m_external_ppu_dram_memory_differential_words.clear();
		m_external_ppu_dram_memory_base_addresses.clear();
		m_external_ppu_dram_memory_differential_addresses.clear();
		for (size_t i = 0; i < m_external_ppu_dram_memory_addresses.size(); ++i) {
			if (auto it = m_last_external_ppu_dram_memory_words.find(
			        m_external_ppu_dram_memory_addresses[i]);
			    it != m_last_external_ppu_dram_memory_words.end() &&
			    it->second != m_external_ppu_dram_memory_words.at(i)) {
				m_external_ppu_dram_memory_differential_addresses.push_back(
				    m_external_ppu_dram_memory_addresses[i]);
				m_external_ppu_dram_memory_differential_words.push_back(
				    m_external_ppu_dram_memory_words[i]);
			} else {
				m_external_ppu_dram_memory_base_addresses.push_back(
				    m_external_ppu_dram_memory_addresses[i]);
				m_external_ppu_dram_memory_base_words.push_back(
				    m_external_ppu_dram_memory_words[i]);
			}
		}
	}
}

bool StatefulChipConfig::get_is_fresh() const
{
	return m_system_words.empty();
}

bool StatefulChipConfig::get_has_differential() const
{
	return !m_system_differential_words.empty() ||
	       !m_external_ppu_dram_memory_differential_words.empty();
}


bool StatefulChipConfig::get_differential_changes_capmem() const
{
	// iterate over all addresses and check whether the base matches one of the CapMem base
	// addresses.
	for (auto const& address : m_system_differential_addresses) {
		// select only the upper 16 bit and compare to the CapMem base address.
		auto const base = (address.value() & 0xffff0000);
		// north-west and south-west base addresses suffice because east base addresses only
		// have another bit set in the lower 16 bit.
		if ((base == haldls::vx::v3::capmem_nw_sram_base_address) ||
		    (base == haldls::vx::v3::capmem_sw_sram_base_address)) {
			return true;
		}
	}
	return false;
}

haldls::vx::Encodable::BackendCocoListVariant StatefulChipConfig::get_base()
{
	Addresses addresses(m_system_base_addresses);
	addresses.insert(
	    addresses.end(), m_external_ppu_dram_memory_base_addresses.begin(),
	    m_external_ppu_dram_memory_base_addresses.end());
	Words words(m_system_base_words);
	words.insert(
	    words.end(), m_external_ppu_dram_memory_base_words.begin(),
	    m_external_ppu_dram_memory_base_words.end());
	return std::pair{addresses, words};
}

haldls::vx::Encodable::BackendCocoListVariant StatefulChipConfig::get_differential()
{
	Addresses addresses(m_system_differential_addresses);
	addresses.insert(
	    addresses.end(), m_external_ppu_dram_memory_differential_addresses.begin(),
	    m_external_ppu_dram_memory_differential_addresses.end());
	Words words(m_system_differential_words);
	words.insert(
	    words.end(), m_external_ppu_dram_memory_differential_words.begin(),
	    m_external_ppu_dram_memory_differential_words.end());
	return std::pair{addresses, words};
}


StatefulChipConfigBase::StatefulChipConfigBase(StatefulChipConfig& config) : m_config(config) {}

StatefulChipConfigBase::BackendCoordinateListVariant StatefulChipConfigBase::encode_read(
    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& /* backend */) const
{
	if (typeid(coordinate) != typeid(StatefulChipConfigBaseCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
	return StatefulChipConfig::Addresses{};
}

void StatefulChipConfigBase::decode_read(
    BackendContainerListVariant const& /* data */, Coordinate const& coordinate)
{
	if (typeid(coordinate) != typeid(StatefulChipConfigBaseCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
}

std::initializer_list<hxcomm::vx::Target> StatefulChipConfigBase::get_unsupported_read_targets()
    const
{
	return {};
}

std::unique_ptr<StatefulChipConfigBase::Container> StatefulChipConfigBase::clone_container() const
{
	return std::make_unique<StatefulChipConfigBase>(*this);
}

StatefulChipConfigBase::BackendCocoListVariant StatefulChipConfigBase::encode_write(
    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& /* backend */) const
{
	if (typeid(coordinate) != typeid(StatefulChipConfigBaseCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
	return m_config.get_base();
}

std::initializer_list<hxcomm::vx::Target> StatefulChipConfigBase::get_unsupported_write_targets()
    const
{
	return {};
}

bool StatefulChipConfigBase::get_supports_differential_write() const
{
	return false;
}

std::unique_ptr<StatefulChipConfigBase::Encodable> StatefulChipConfigBase::clone_encodable() const
{
	return std::make_unique<StatefulChipConfigBase>(*this);
}

bool StatefulChipConfigBase::get_is_valid_backend(haldls::vx::Backend backend) const
{
	return backend == haldls::vx::Backend::Omnibus;
}

std::ostream& StatefulChipConfigBase::print(std::ostream& os) const
{
	return os;
}


bool StatefulChipConfigBase::operator==(Encodable const& other) const
{
	if (auto const ptr = dynamic_cast<StatefulChipConfigBase const*>(&other); ptr) {
		return m_config == ptr->m_config;
	}
	return false;
}

bool StatefulChipConfigBase::operator!=(Encodable const& other) const
{
	return !(*this == other);
}


StatefulChipConfigDifferential::StatefulChipConfigDifferential(StatefulChipConfig& config) :
    m_config(config)
{
}

StatefulChipConfigDifferential::BackendCoordinateListVariant
StatefulChipConfigDifferential::encode_read(
    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& /* backend */) const
{
	if (typeid(coordinate) != typeid(StatefulChipConfigDifferentialCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
	return StatefulChipConfig::Addresses{};
}

void StatefulChipConfigDifferential::decode_read(
    BackendContainerListVariant const& /* data */, Coordinate const& coordinate)
{
	if (typeid(coordinate) != typeid(StatefulChipConfigDifferentialCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
}

std::initializer_list<hxcomm::vx::Target>
StatefulChipConfigDifferential::get_unsupported_read_targets() const
{
	return {};
}

std::unique_ptr<StatefulChipConfigDifferential::Container>
StatefulChipConfigDifferential::clone_container() const
{
	return std::make_unique<StatefulChipConfigDifferential>(*this);
}

StatefulChipConfigDifferential::BackendCocoListVariant StatefulChipConfigDifferential::encode_write(
    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& /* backend */) const
{
	if (typeid(coordinate) != typeid(StatefulChipConfigDifferentialCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
	return m_config.get_differential();
}

std::initializer_list<hxcomm::vx::Target>
StatefulChipConfigDifferential::get_unsupported_write_targets() const
{
	return {};
}

bool StatefulChipConfigDifferential::get_supports_differential_write() const
{
	return false;
}

std::unique_ptr<StatefulChipConfigDifferential::Encodable>
StatefulChipConfigDifferential::clone_encodable() const
{
	return std::make_unique<StatefulChipConfigDifferential>(*this);
}

bool StatefulChipConfigDifferential::get_is_valid_backend(haldls::vx::Backend backend) const
{
	return backend == haldls::vx::Backend::Omnibus;
}

std::ostream& StatefulChipConfigDifferential::print(std::ostream& os) const
{
	return os;
}


bool StatefulChipConfigDifferential::operator==(Encodable const& other) const
{
	if (auto const ptr = dynamic_cast<StatefulChipConfigDifferential const*>(&other); ptr) {
		return m_config == ptr->m_config;
	}
	return false;
}

bool StatefulChipConfigDifferential::operator!=(Encodable const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::execution::backend::detail
