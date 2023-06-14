#include "grenade/vx/signal_flow/transformation/mac_spiketrain_generator.h"

#include "halco/hicann-dls/vx/v3/event.h"
#include "halco/hicann-dls/vx/v3/padi.h"
#include "halco/hicann-dls/vx/v3/synapse_driver.h"

namespace grenade::vx::signal_flow::transformation {

MACSpikeTrainGenerator::~MACSpikeTrainGenerator() {}

MACSpikeTrainGenerator::MACSpikeTrainGenerator(
    halco::common::typed_array<size_t, halco::hicann_dls::vx::v3::HemisphereOnDLS> const&
        hemisphere_sizes,
    size_t num_sends,
    common::Time wait_between_events) :
    m_hemisphere_sizes(hemisphere_sizes),
    m_num_sends(num_sends),
    m_wait_between_events(wait_between_events)
{}

std::vector<Port> MACSpikeTrainGenerator::inputs() const
{
	std::vector<Port> ports;
	for (auto const s : m_hemisphere_sizes) {
		if (s != 0) {
			ports.push_back(Port(s, ConnectionType::UInt5));
		}
	}
	return ports;
}

Port MACSpikeTrainGenerator::output() const
{
	return Port(1, ConnectionType::TimedSpikeToChipSequence);
}

MACSpikeTrainGenerator::Function::Value MACSpikeTrainGenerator::apply(
    std::vector<Function::Value> const& value) const
{
	using namespace halco::hicann_dls::vx::v3;
	using namespace haldls::vx::v3;

	size_t batch_size = 0;
	if (!value.empty()) {
		batch_size = std::visit([](auto const& v) { return v.size(); }, value.at(0));
	}

	halco::common::typed_array<std::vector<halco::hicann_dls::vx::v3::SpikeLabel>, HemisphereOnDLS>
	    labels;

	std::vector<std::reference_wrapper<
	    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>> const>>
	    uvalue;
	for (auto const& v : value) {
		uvalue.push_back(
		    std::get<std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>>(v));
	}

	std::vector<signal_flow::TimedSpikeToChipSequence> events(batch_size);

	for (size_t batch = 0; batch < batch_size; ++batch) {
		// reserve labels
		for (auto& l : labels) {
			l.reserve(SynapseDriverOnSynapseDriverBlock::size);
		}

		// generate spike labels
		assert(uvalue.size() <= labels.size());
		size_t i = 0;
		for (auto const& hvalue : uvalue) {
			auto const& lvalue = hvalue.get().at(batch);
			// only one logical input event
			assert(lvalue.size() == 1);
			auto const input_size = lvalue.at(0).data.size();
			auto const hemisphere = HemisphereOnDLS(i);
			auto& local_labels = labels[hemisphere];

			// pack all events from one hemisphere after one another
			for (size_t j = 0; j < input_size; ++j) {
				auto const label = get_spike_label(
				    SynapseDriverOnDLS(
				        SynapseDriverOnSynapseDriverBlock(j),
				        hemisphere.toSynapseDriverBlockOnDLS()),
				    lvalue.at(0).data[j]);
				if (label) {
					local_labels.push_back(*label);
				}
			}
			i++;
		}

		auto const [labels_min_it, labels_max_it] = std::minmax_element(
		    labels.begin(), labels.end(),
		    [](auto const& a, auto const& b) { return a.size() < b.size(); });
		auto const& labels_min = *labels_min_it;
		auto const& labels_max = *labels_max_it;

		auto& local_events = events.at(batch);
		local_events.reserve(labels_max.size() * m_num_sends);
		// add events from back to unsure equal time between last event and readout
		// for both hemispheres
		common::Time time(labels_max.size() * m_wait_between_events * m_num_sends);
		// add 2-packed events (both hemispheres)
		size_t const labels_min_size = labels_min.size();
		size_t const labels_max_size = labels_max.size();
		size_t const both_hemispheres = labels_min_size * m_num_sends;
		for (size_t n = 0; n < both_hemispheres; ++n) {
			auto const label_min = labels_min[n % labels_min_size];
			auto const label_max = labels_max[n % labels_max_size];
			SpikePack2ToChip const payload(SpikePack2ToChip::labels_type{label_min, label_max});
			local_events.push_back(signal_flow::TimedSpikeToChip{time, payload});
			time = common::Time(time - m_wait_between_events);
		}
		// add 1-packed left events (hemisphere with more events)
		size_t const one_hemisphere = labels_max_size * m_num_sends;
		for (size_t n = both_hemispheres; n < one_hemisphere; ++n) {
			auto const label = labels_max[n % labels_max_size];
			SpikePack1ToChip const payload(SpikePack1ToChip::labels_type{label});
			local_events.push_back(signal_flow::TimedSpikeToChip{time, payload});
			time = common::Time(time - m_wait_between_events);
		}

		// clear labels
		for (auto& l : labels) {
			l.clear();
		}

		std::sort(local_events.begin(), local_events.end(), [](auto const& a, auto const& b) {
			return a.time < b.time;
		});
	}

	return events;
}

std::optional<halco::hicann_dls::vx::v3::SpikeLabel> MACSpikeTrainGenerator::get_spike_label(
    halco::hicann_dls::vx::v3::SynapseDriverOnDLS const& driver, signal_flow::UInt5 const value)
{
	if (value == 0) {
		return std::nullopt;
	}
	using namespace halco::hicann_dls::vx::v3;
	auto const synapse_driver = driver.toSynapseDriverOnSynapseDriverBlock();
	auto const spl1_address = synapse_driver % PADIBusOnPADIBusBlock::size;
	auto const synapse_label = ((signal_flow::UInt5::max - value.value()) + 1);
	auto const row_select = synapse_driver / PADIBusOnPADIBusBlock::size;
	auto const h = (static_cast<size_t>(driver.toSynapseDriverBlockOnDLS()) << 13);
	return halco::hicann_dls::vx::v3::SpikeLabel(
	    h | (spl1_address << 14) | (row_select) << 6 | synapse_label);
}

bool MACSpikeTrainGenerator::equal(vertex::Transformation::Function const& other) const
{
	MACSpikeTrainGenerator const* o = dynamic_cast<MACSpikeTrainGenerator const*>(&other);
	if (o == nullptr) {
		return false;
	}
	return (m_hemisphere_sizes == o->m_hemisphere_sizes) && (m_num_sends == o->m_num_sends) &&
	       (m_wait_between_events == o->m_wait_between_events);
}

} // namespace grenade::vx::signal_flow::transformation
