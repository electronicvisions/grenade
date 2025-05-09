#include "grenade/vx/execution/backend/detail/connection_config.h"

#include "haldls/vx/v3/omnibus_constants.h"
#include "lola/vx/v3/ppu.h"
#include "stadls/visitors.h"

namespace grenade::vx::execution::backend::detail {

bool ConnectionConfig::get_enable_differential_config() const
{
	return m_enable_differential_config;
}

void ConnectionConfig::set_enable_differential_config(bool const value)
{
	m_enable_differential_config = value;
}

namespace {

/**
 * Helper to only encode the constant addresses of the lola Chip object once.
 */
static const std::vector<halco::hicann_dls::vx::OmnibusAddress> chip_addresses = []() {
	using namespace halco::hicann_dls::vx::v3;
	typedef std::vector<OmnibusAddress> addresses_type;
	addresses_type storage;
	hate::Empty<lola::vx::v3::Chip> config;
	haldls::vx::visit_preorder(
	    config, ChipOnDLS(), stadls::WriteAddressVisitor<addresses_type>{storage});
	return storage;
}();

} // namespace

void ConnectionConfig::set_chip(lola::vx::v3::Chip const& value, bool const split_base_differential)
{
	auto const encode_value = [value, this]() {
		std::swap(m_last_chip_words, m_chip_words);

		m_chip_words.clear();
		m_chip_words.reserve(chip_addresses.size());
		haldls::vx::visit_preorder(
		    value, hate::Empty<halco::hicann_dls::vx::v3::ChipOnDLS>(),
		    stadls::EncodeVisitor<Words>{m_chip_words});
	};

	if (split_base_differential) {
		if (!m_enable_differential_config) {
			encode_value();
			m_chip_differential_words.clear();
			m_chip_differential_addresses.clear();

			m_chip_base_addresses = chip_addresses;
			m_chip_base_words = m_chip_words;
		} else if (get_is_fresh()) {
			encode_value();
			assert(m_chip_base_words.empty());
			assert(m_chip_differential_words.empty());
			assert(m_chip_base_addresses.empty());
			assert(m_chip_differential_addresses.empty());

			m_chip_base_addresses = chip_addresses;
			m_chip_base_words = m_chip_words;
			m_last_chip = value;
		} else {
			if (m_last_chip != value) {
				encode_value();
				assert(m_chip_words.size() == chip_addresses.size());
				assert(m_last_chip_words.size() == chip_addresses.size());

				m_chip_base_words.clear();
				m_chip_differential_words.clear();
				m_chip_base_addresses.clear();
				m_chip_differential_addresses.clear();
				for (size_t i = 0; i < chip_addresses.size(); ++i) {
					if (m_last_chip_words[i] != m_chip_words[i]) {
						m_chip_differential_addresses.push_back(chip_addresses[i]);
						m_chip_differential_words.push_back(m_chip_words[i]);
					} else {
						m_chip_base_addresses.push_back(chip_addresses[i]);
						m_chip_base_words.push_back(m_chip_words[i]);
					}
				}
				m_last_chip = value;
			} else {
				m_last_chip_words = m_chip_words;
			}
		}
	} else {
		if (m_last_chip != value) {
			encode_value();
			m_last_chip = value;
		} else {
			m_last_chip_words = m_chip_words;
		}
	}
}

void ConnectionConfig::set_external_ppu_dram_memory(
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

bool ConnectionConfig::get_is_fresh() const
{
	return m_chip_words.empty();
}

bool ConnectionConfig::get_has_differential() const
{
	return !m_chip_differential_words.empty() ||
	       !m_external_ppu_dram_memory_differential_words.empty();
}


bool ConnectionConfig::get_differential_changes_capmem() const
{
	// iterate over all addresses and check whether the base matches one of the CapMem base
	// addresses.
	for (auto const& address : m_chip_differential_addresses) {
		// select only the upper 16 bit and compare to the CapMem base address.
		auto const base = (address.value() & 0xffff0000);
		// north-west and south-west base addresses suffice because east base addresses only have
		// another bit set in the lower 16 bit.
		if ((base == haldls::vx::v3::capmem_nw_sram_base_address) ||
		    (base == haldls::vx::v3::capmem_sw_sram_base_address)) {
			return true;
		}
	}
	return false;
}

haldls::vx::Encodable::BackendCocoListVariant ConnectionConfig::get_base()
{
	Addresses addresses(m_chip_base_addresses);
	addresses.insert(
	    addresses.end(), m_external_ppu_dram_memory_base_addresses.begin(),
	    m_external_ppu_dram_memory_base_addresses.end());
	Words words(m_chip_base_words);
	words.insert(
	    words.end(), m_external_ppu_dram_memory_base_words.begin(),
	    m_external_ppu_dram_memory_base_words.end());
	return std::pair{addresses, words};
}

haldls::vx::Encodable::BackendCocoListVariant ConnectionConfig::get_differential()
{
	Addresses addresses(m_chip_differential_addresses);
	addresses.insert(
	    addresses.end(), m_external_ppu_dram_memory_differential_addresses.begin(),
	    m_external_ppu_dram_memory_differential_addresses.end());
	Words words(m_chip_differential_words);
	words.insert(
	    words.end(), m_external_ppu_dram_memory_differential_words.begin(),
	    m_external_ppu_dram_memory_differential_words.end());
	return std::pair{addresses, words};
}


ConnectionConfigBase::ConnectionConfigBase(ConnectionConfig& config) : m_config(config) {}

ConnectionConfigBase::BackendCoordinateListVariant ConnectionConfigBase::encode_read(
    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& /* backend */) const
{
	if (typeid(coordinate) != typeid(ConnectionConfigBaseCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
	return ConnectionConfig::Addresses{};
}

void ConnectionConfigBase::decode_read(
    BackendContainerListVariant const& /* data */, Coordinate const& coordinate)
{
	if (typeid(coordinate) != typeid(ConnectionConfigBaseCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
}

std::initializer_list<hxcomm::vx::Target> ConnectionConfigBase::get_unsupported_read_targets() const
{
	return {};
}

std::unique_ptr<ConnectionConfigBase::Container> ConnectionConfigBase::clone_container() const
{
	return std::make_unique<ConnectionConfigBase>(*this);
}

ConnectionConfigBase::BackendCocoListVariant ConnectionConfigBase::encode_write(
    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& /* backend */) const
{
	if (typeid(coordinate) != typeid(ConnectionConfigBaseCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
	return m_config.get_base();
}

std::initializer_list<hxcomm::vx::Target> ConnectionConfigBase::get_unsupported_write_targets()
    const
{
	return {};
}

bool ConnectionConfigBase::get_supports_differential_write() const
{
	return false;
}

std::unique_ptr<ConnectionConfigBase::Encodable> ConnectionConfigBase::clone_encodable() const
{
	return std::make_unique<ConnectionConfigBase>(*this);
}

bool ConnectionConfigBase::get_is_valid_backend(haldls::vx::Backend backend) const
{
	return backend == haldls::vx::Backend::Omnibus;
}

std::ostream& ConnectionConfigBase::print(std::ostream& os) const
{
	return os;
}


bool ConnectionConfigBase::operator==(Encodable const& other) const
{
	if (auto const ptr = dynamic_cast<ConnectionConfigBase const*>(&other); ptr) {
		return m_config == ptr->m_config;
	}
	return false;
}

bool ConnectionConfigBase::operator!=(Encodable const& other) const
{
	return !(*this == other);
}


ConnectionConfigDifferential::ConnectionConfigDifferential(ConnectionConfig& config) :
    m_config(config)
{
}

ConnectionConfigDifferential::BackendCoordinateListVariant
ConnectionConfigDifferential::encode_read(
    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& /* backend */) const
{
	if (typeid(coordinate) != typeid(ConnectionConfigDifferentialCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
	return ConnectionConfig::Addresses{};
}

void ConnectionConfigDifferential::decode_read(
    BackendContainerListVariant const& /* data */, Coordinate const& coordinate)
{
	if (typeid(coordinate) != typeid(ConnectionConfigDifferentialCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
}

std::initializer_list<hxcomm::vx::Target>
ConnectionConfigDifferential::get_unsupported_read_targets() const
{
	return {};
}

std::unique_ptr<ConnectionConfigDifferential::Container>
ConnectionConfigDifferential::clone_container() const
{
	return std::make_unique<ConnectionConfigDifferential>(*this);
}

ConnectionConfigDifferential::BackendCocoListVariant ConnectionConfigDifferential::encode_write(
    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& /* backend */) const
{
	if (typeid(coordinate) != typeid(ConnectionConfigDifferentialCoordinate)) {
		throw std::runtime_error("Coordinate type doesn't match container.");
	}
	return m_config.get_differential();
}

std::initializer_list<hxcomm::vx::Target>
ConnectionConfigDifferential::get_unsupported_write_targets() const
{
	return {};
}

bool ConnectionConfigDifferential::get_supports_differential_write() const
{
	return false;
}

std::unique_ptr<ConnectionConfigDifferential::Encodable>
ConnectionConfigDifferential::clone_encodable() const
{
	return std::make_unique<ConnectionConfigDifferential>(*this);
}

bool ConnectionConfigDifferential::get_is_valid_backend(haldls::vx::Backend backend) const
{
	return backend == haldls::vx::Backend::Omnibus;
}

std::ostream& ConnectionConfigDifferential::print(std::ostream& os) const
{
	return os;
}


bool ConnectionConfigDifferential::operator==(Encodable const& other) const
{
	if (auto const ptr = dynamic_cast<ConnectionConfigDifferential const*>(&other); ptr) {
		return m_config == ptr->m_config;
	}
	return false;
}

bool ConnectionConfigDifferential::operator!=(Encodable const& other) const
{
	return !(*this == other);
}

} // namespace grenade::vx::execution::backend::detail
