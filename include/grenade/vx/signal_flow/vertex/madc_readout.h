#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/port_data.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "halco/hicann-dls/vx/v3/neuron.h"
#include "haldls/vx/v3/event.h"
#include "haldls/vx/v3/madc.h"
#include "hate/visibility.h"
#include "lola/vx/v3/neuron.h"
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <tuple>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow {
namespace vertex {

/**
 * Readout of neuron voltages via the MADC.
 */
struct SYMBOL_VISIBLE MADCReadoutView : public EntityOnChip
{
	struct Results : public grenade::common::BatchedPortData
	{
		typedef std::vector<TimedMADCSampleFromChipSequence> Samples;
		Samples samples;

		Results(Samples samples);

		virtual size_t batch_size() const override;

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(PortData const& other) const override;
	};

	virtual bool valid_output_port_data(
	    size_t output_port_on_vertex, grenade::common::PortData const& data) const override;

	struct Parameterization : public grenade::common::PortData
	{
		/** Initially selected channel. */
		typedef halco::hicann_dls::vx::SourceMultiplexerOnReadoutSourceSelection Initial;
		Initial initial{};

		/** Period with which to switch between channels. */
		typedef haldls::vx::v3::MADCConfig::ActiveMuxInputSelectLength Period;
		Period period{};

		Parameterization();

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(PortData const& other) const override;
	};

	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex,
	    grenade::common::PortData const& parameterization) const override;

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	typedef halco::hicann_dls::vx::v3::AtomicNeuronOnDLS Source;

	/**
	 * Construct MADCReadoutView.
	 * @param first_source Neuron source and location to read out
	 * @param second_source Optional second neuron source and location to read out
	 * @param chip_on_connection Coordinate of chip to use
	 * @param time_domain Time domain to use
	 * @param execution_instance_on_executor Execution instance on executor to use
	 */
	explicit MADCReadoutView(
	    Source const& first_source,
	    std::optional<Source> const& second_source,
	    common::ChipOnConnection const& chip_on_connection,
	    grenade::common::TimeDomainOnTopology const& time_domain,
	    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor)
	    SYMBOL_VISIBLE;

	Source const& get_first_source() const;
	std::optional<Source> const& get_second_source() const;

	virtual bool valid_edge_from(
	    Vertex const& source, grenade::common::Edge const& edge) const override;

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual bool is_equal_to(Vertex const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	Source m_first_source;
	std::optional<Source> m_second_source;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // vertex
} // grenade::vx::signal_flow
