#pragma once
#include "fisch/vx/word_access/type/omnibus.h"
#include "halco/common/coordinate.h"
#include "halco/common/geometry.h"
#include "haldls/vx/container.h"
#include "haldls/vx/encodable.h"
#include "hate/visibility.h"
#include "lola/vx/v3/chip.h"
#include "lola/vx/v3/ppu.h"
#include <optional>
#include <vector>

namespace grenade::vx::execution::backend::detail {

/**
 * Connection config tracking changes to the configuration of hardware accessible via one
 * connection.
 * Containers are eagerly encoded to only encode them once for both base and differential config
 * parts.
 */
struct StatefulConnectionConfig
{
	StatefulConnectionConfig() = default;

	bool get_enable_differential_config() const SYMBOL_VISIBLE;
	void set_enable_differential_config(bool value) SYMBOL_VISIBLE;

	/**
	 * Set chip which is to be applied next.
	 */
	void set_chip(lola::vx::v3::Chip const& value, bool split_base_differential) SYMBOL_VISIBLE;

	/**
	 * Set external PPU DRAM memory which is to be applied next.
	 */
	void set_external_ppu_dram_memory(
	    std::optional<lola::vx::v3::ExternalPPUDRAMMemoryBlock> const& value) SYMBOL_VISIBLE;

	/**
	 * Get whether applying current state differential changes CapMem.
	 */
	bool get_differential_changes_capmem() const SYMBOL_VISIBLE;

	/**
	 * Get whether connection config is fresh, i.e. never applied.
	 */
	bool get_is_fresh() const SYMBOL_VISIBLE;

	bool get_has_differential() const SYMBOL_VISIBLE;

	bool operator==(StatefulConnectionConfig const& other) const = default;
	bool operator!=(StatefulConnectionConfig const& other) const = default;

	haldls::vx::Encodable::BackendCocoListVariant get_base() SYMBOL_VISIBLE;
	haldls::vx::Encodable::BackendCocoListVariant get_differential() SYMBOL_VISIBLE;

	typedef std::vector<halco::hicann_dls::vx::OmnibusAddress> Addresses;
	typedef std::vector<fisch::vx::word_access_type::Omnibus> Words;

private:
	bool m_enable_differential_config;

	lola::vx::v3::Chip m_last_chip;
	Words m_chip_words;
	Words m_last_chip_words;
	Words m_chip_base_words;
	Words m_chip_differential_words;
	Addresses m_chip_base_addresses;
	Addresses m_chip_differential_addresses;

	std::optional<lola::vx::v3::ExternalPPUDRAMMemoryBlock> m_external_ppu_dram_memory;
	std::optional<lola::vx::v3::ExternalPPUDRAMMemoryBlock> m_last_external_ppu_dram_memory;

	Words m_external_ppu_dram_memory_words;
	Addresses m_external_ppu_dram_memory_addresses;
	std::map<Addresses::value_type, Words::value_type> m_last_external_ppu_dram_memory_words;
	Words m_external_ppu_dram_memory_base_words;
	Words m_external_ppu_dram_memory_differential_words;
	Addresses m_external_ppu_dram_memory_base_addresses;
	Addresses m_external_ppu_dram_memory_differential_addresses;
};


struct StatefulConnectionConfigBaseCoordinate
    : public halco::common::detail::
          RantWrapper<StatefulConnectionConfigBaseCoordinate, size_t, 0, 0>
    , public halco::common::CoordinateBase<StatefulConnectionConfigBaseCoordinate>
{};


struct StatefulConnectionConfigBase : public haldls::vx::Container
{
	StatefulConnectionConfigBase(StatefulConnectionConfig& config) SYMBOL_VISIBLE;

	virtual BackendCoordinateListVariant encode_read(
	    Coordinate const& coordinate,
	    std::optional<haldls::vx::Backend> const& backend) const SYMBOL_VISIBLE;

	virtual void decode_read(BackendContainerListVariant const& data, Coordinate const& coordinate)
	    SYMBOL_VISIBLE;

	virtual std::initializer_list<hxcomm::vx::Target> get_unsupported_read_targets() const
	    SYMBOL_VISIBLE;

	virtual std::unique_ptr<Container> clone_container() const SYMBOL_VISIBLE;

	virtual BackendCocoListVariant encode_write(
	    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& backend) const;

	virtual std::initializer_list<hxcomm::vx::Target> get_unsupported_write_targets() const;

	virtual bool get_supports_differential_write() const;

	virtual std::unique_ptr<Encodable> clone_encodable() const;

	virtual bool get_is_valid_backend(haldls::vx::Backend backend) const;

	virtual std::ostream& print(std::ostream& os) const;

	virtual bool operator==(Encodable const& other) const;
	virtual bool operator!=(Encodable const& other) const;

private:
	StatefulConnectionConfig& m_config;
};


struct StatefulConnectionConfigDifferentialCoordinate
    : public halco::common::detail::
          RantWrapper<StatefulConnectionConfigDifferentialCoordinate, size_t, 0, 0>
    , public halco::common::CoordinateBase<StatefulConnectionConfigDifferentialCoordinate>
{};

struct StatefulConnectionConfigDifferential : public haldls::vx::Container
{
	StatefulConnectionConfigDifferential(StatefulConnectionConfig& config) SYMBOL_VISIBLE;

	virtual BackendCoordinateListVariant encode_read(
	    Coordinate const& coordinate,
	    std::optional<haldls::vx::Backend> const& backend) const SYMBOL_VISIBLE;

	virtual void decode_read(BackendContainerListVariant const& data, Coordinate const& coordinate)
	    SYMBOL_VISIBLE;

	virtual std::initializer_list<hxcomm::vx::Target> get_unsupported_read_targets() const
	    SYMBOL_VISIBLE;

	virtual std::unique_ptr<Container> clone_container() const SYMBOL_VISIBLE;

	virtual BackendCocoListVariant encode_write(
	    Coordinate const& coordinate, std::optional<haldls::vx::Backend> const& backend) const;

	virtual std::initializer_list<hxcomm::vx::Target> get_unsupported_write_targets() const;

	virtual bool get_supports_differential_write() const;

	virtual std::unique_ptr<Encodable> clone_encodable() const;

	virtual bool get_is_valid_backend(haldls::vx::Backend backend) const;

	virtual std::ostream& print(std::ostream& os) const;

	virtual bool operator==(Encodable const& other) const;
	virtual bool operator!=(Encodable const& other) const;

private:
	StatefulConnectionConfig& m_config;
};

} // namespace grenade::vx::execution::backend::detail
