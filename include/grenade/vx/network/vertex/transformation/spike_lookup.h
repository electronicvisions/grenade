#pragma once
#include "dapr/empty_property.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "haldls/vx/v3/event.h"
#include <vector>

namespace grenade::vx::network::vertex::transformation {

/**
 * Transformation implementing lookup of spike label and time translations.
 */
struct SYMBOL_VISIBLE SpikeLookup : public signal_flow::vertex::Transformation::Function
{
	struct Parameterization : public grenade::common::PortData
	{
		/**
		 * Post labels for pre label with according delays.
		 */
		typedef std::map<
		    halco::hicann_dls::vx::v3::SpikeLabel,
		    std::vector<std::pair<halco::hicann_dls::vx::v3::SpikeLabel, common::Time>>>
		    Translation;
		Translation translation;

		Parameterization(Translation translation);

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual bool is_equal_to(PortData const& other) const override;
		virtual std::ostream& print(std::ostream& os) const override;
	};

	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, grenade::common::PortData const& data) const override;

	/**
	 * Parameterization port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) ParameterizationPortType
	    : public dapr::EmptyProperty<ParameterizationPortType, grenade::common::VertexPortType>
	{};

	/**
	 * Construct lookup transformation for spike-trains.
	 */
	SpikeLookup() = default;

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::vector<signal_flow::vertex::Transformation::Results> apply(
	    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& value)
	    const override;

	virtual std::unique_ptr<signal_flow::vertex::Transformation::Function> copy() const override;
	virtual std::unique_ptr<signal_flow::vertex::Transformation::Function> move() override;

protected:
	virtual bool is_equal_to(
	    signal_flow::vertex::Transformation::Function const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;
};

} // namespace grenade::vx::network::vertex::transformation
