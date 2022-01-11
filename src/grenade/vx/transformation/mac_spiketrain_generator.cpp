#include "grenade/vx/transformation/mac_spiketrain_generator.h"

#include "grenade/cerealization.h"
#include "halco/common/cerealization_geometry.h"
#include "halco/common/cerealization_typed_array.h"
#include "halco/hicann-dls/vx/v2/padi.h"
#include "halco/hicann-dls/vx/v2/synapse_driver.h"
#include "haldls/vx/v2/event.h"
#include <cereal/types/polymorphic.hpp>

namespace grenade::vx::transformation {

MACSpikeTrainGenerator::~MACSpikeTrainGenerator() {}

MACSpikeTrainGenerator::MACSpikeTrainGenerator(
    halco::common::typed_array<size_t, halco::hicann_dls::vx::v2::HemisphereOnDLS> const&
        hemisphere_sizes,
    size_t num_sends,
    haldls::vx::v2::Timer::Value wait_between_events) :
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
	return Port(1, ConnectionType::TimedSpikeSequence);
}

MACSpikeTrainGenerator::Function::Value MACSpikeTrainGenerator::apply(
    std::vector<Function::Value> const& value) const
{
	using namespace halco::hicann_dls::vx::v2;
	using namespace haldls::vx::v2;

	size_t batch_size = 0;
	if (!value.empty()) {
		batch_size = std::visit([](auto const& v) { return v.size(); }, value.at(0));
	}

	halco::common::typed_array<std::vector<SpikeLabel>, HemisphereOnDLS> labels;

	std::vector<std::reference_wrapper<std::vector<TimedDataSequence<std::vector<UInt5>>> const>>
	    uvalue;
	for (auto const& v : value) {
		uvalue.push_back(std::get<std::vector<TimedDataSequence<std::vector<UInt5>>>>(v));
	}

	std::vector<TimedSpikeSequence> events(batch_size);

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
		TimedSpike::Time time(labels_max.size() * m_wait_between_events * m_num_sends);
		// add 2-packed events (both hemispheres)
		size_t const labels_min_size = labels_min.size();
		size_t const labels_max_size = labels_max.size();
		size_t const both_hemispheres = labels_min_size * m_num_sends;
		for (size_t n = 0; n < both_hemispheres; ++n) {
			auto const label_min = labels_min[n % labels_min_size];
			auto const label_max = labels_max[n % labels_max_size];
			SpikePack2ToChip const payload(SpikePack2ToChip::labels_type{label_min, label_max});
			local_events.push_back(TimedSpike{time, payload});
			time = TimedSpike::Time(time - m_wait_between_events);
		}
		// add 1-packed left events (hemisphere with more events)
		size_t const one_hemisphere = labels_max_size * m_num_sends;
		for (size_t n = both_hemispheres; n < one_hemisphere; ++n) {
			auto const label = labels_max[n % labels_max_size];
			SpikePack1ToChip const payload(SpikePack1ToChip::labels_type{label});
			local_events.push_back(TimedSpike{time, payload});
			time = TimedSpike::Time(time - m_wait_between_events);
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

std::optional<haldls::vx::v2::SpikeLabel> MACSpikeTrainGenerator::get_spike_label(
    halco::hicann_dls::vx::v2::SynapseDriverOnDLS const& driver, UInt5 const value)
{
	if (value == 0) {
		return std::nullopt;
	}
	using namespace halco::hicann_dls::vx::v2;
	auto const synapse_driver = driver.toSynapseDriverOnSynapseDriverBlock();
	auto const spl1_address = synapse_driver % PADIBusOnPADIBusBlock::size;
	auto const synapse_label = ((UInt5::max - value.value()) + 1);
	auto const row_select = synapse_driver / PADIBusOnPADIBusBlock::size;
	auto const h = (static_cast<size_t>(driver.toSynapseDriverBlockOnDLS()) << 13);
	return haldls::vx::v2::SpikeLabel(h | (spl1_address << 14) | (row_select) << 6 | synapse_label);
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

template <typename Archive>
void MACSpikeTrainGenerator::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_hemisphere_sizes);
	ar(m_num_sends);
	ar(m_wait_between_events);
}

} // namespace grenade::vx::transformation

CEREAL_REGISTER_TYPE(grenade::vx::transformation::MACSpikeTrainGenerator)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::vx::vertex::Transformation::Function,
    grenade::vx::transformation::MACSpikeTrainGenerator)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::transformation::MACSpikeTrainGenerator)
CEREAL_CLASS_VERSION(grenade::vx::transformation::MACSpikeTrainGenerator, 0)
