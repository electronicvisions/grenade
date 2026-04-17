#pragma once
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/common/time.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/chip.h"
#include "halco/hicann-dls/vx/v3/event.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"
#include "haldls/vx/v3/event.h"
#include "haldls/vx/v3/timer.h"
#include <functional>
#include <vector>
#include <gtest/gtest_prod.h>

namespace cereal {
struct access;
} // namespace cereal

class MACSpikeTrainGenerator_get_spike_label_Test;

namespace grenade::vx::signal_flow::vertex::transformation {

struct SYMBOL_VISIBLE MACSpikeTrainGenerator : public Transformation::Function
{
	MACSpikeTrainGenerator() = default;

	/**
	 * Construct spiketrain generator transformation.
	 * @param hemisphere_sizes Hemisphere sizes for which to generate spikeTrain for.
	 *        This setting corresponds to the number of inputs expected from the transformation,
	 *        where for each hemisphere size > 0 an input is expected.
	 * @param num_sends Number of times a input activation is sent to the specific row
	 * @param wait_between_events Wait time between input events in FPGA cycles
	 */
	MACSpikeTrainGenerator(
	    halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::HemisphereOnDLS> const&
	        hemisphere_sizes,
	    size_t num_sends,
	    common::Time wait_between_events);

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::vector<Transformation::Results> apply(
	    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data)
	    const override;

	virtual std::unique_ptr<Function> copy() const override;
	virtual std::unique_ptr<Function> move() override;

protected:
	virtual bool is_equal_to(Function const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	FRIEND_TEST(::MACSpikeTrainGenerator, get_spike_label);

	/**
	 * Get spike label value from location and activation value.
	 * @param row Synapse driver to send to
	 * @param value Activation value to send
	 * @return SpikeLabel value if activation value is larger than zero
	 */
	static std::optional<halco::hicann_dls::vx::v3::SpikeLabel> get_spike_label(
	    halco::hicann_dls::vx::v3::SynapseDriverOnDLS const& driver,
	    signal_flow::UInt5 const value) SYMBOL_VISIBLE;

	halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::HemisphereOnDLS>
	    m_hemisphere_sizes;
	size_t m_num_sends{};
	common::Time m_wait_between_events{};

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // namespace grenade::vx::signal_flow::vertex::transformation
