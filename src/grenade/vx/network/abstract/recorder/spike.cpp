#include "grenade/vx/network/abstract/recorder/spike.h"

#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/vx/network/abstract/vertex_port_type/spike.h"
#include "hate/indent.h"

namespace grenade::vx::network::abstract {

SpikeRecorder::Results::Results(size_t batch_size, size_t size) : spikes(batch_size)
{
	for (auto& batch_entry : spikes) {
		batch_entry.resize(size);
	}
}

SpikeRecorder::Results::Results(Spikes spikes) : spikes(std::move(spikes)) {}

size_t SpikeRecorder::Results::batch_size() const
{
	return spikes.size();
}

size_t SpikeRecorder::Results::size() const
{
	if (spikes.empty()) {
		return 0;
	}
	size_t const ret = spikes.at(0).size();
	for (size_t i = 1; i < spikes.size(); ++i) {
		if (ret != spikes.at(i).size()) {
			throw std::runtime_error("Size inhomogeneous across batch entries.");
		}
	}
	return ret;
}

void SpikeRecorder::Results::set_section(
    Recorder::Results const& other_results, grenade::common::MultiIndexSequence const& sequence)
{
	if (auto const other_results_ptr = dynamic_cast<Results const*>(&other_results);
	    other_results_ptr) {
		if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
			throw std::invalid_argument("Given sequence not part of results to set section into.");
		}
		if (batch_size() != other_results.batch_size()) {
			throw std::invalid_argument("Batch sizes don't match between results and section.");
		}
		for (size_t i = 0; auto const& element : sequence.get_elements()) {
			for (size_t b = 0; b < batch_size(); ++b) {
				spikes.at(b).at(element.value.at(0)) = other_results_ptr->spikes.at(b).at(i);
			}
			i++;
		}
	} else {
		throw std::invalid_argument(
		    "Given results not of same type as results to set section into.");
	}
}

std::unique_ptr<grenade::common::PortData> SpikeRecorder::Results::copy() const
{
	return std::make_unique<SpikeRecorder::Results>(*this);
}

std::unique_ptr<grenade::common::PortData> SpikeRecorder::Results::move()
{
	return std::make_unique<SpikeRecorder::Results>(std::move(*this));
}

bool SpikeRecorder::Results::is_equal_to(grenade::common::PortData const& other) const
{
	return spikes == static_cast<Results const&>(other).spikes;
}

std::ostream& SpikeRecorder::Results::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Recording(\n";
	for (size_t b = 0; auto const& batch_entry : spikes) {
		ios << hate::Indentation("\t");
		ios << "batch entry " << b << ":\n";
		for (size_t n = 0; auto const& neuron : batch_entry) {
			ios << hate::Indentation("\t\t");
			ios << "channel " << n << ":\n";
			ios << hate::Indentation("\t\t\t");
			for (auto const& spike : neuron) {
				ios << spike << "\n";
			}
			n++;
		}
		b++;
	}
	ios << ")";
	return os;
}


SpikeRecorder::SpikeRecorder(
    grenade::common::MultiIndexSequence const& shape,
    grenade::common::TimeDomainOnTopology const& time_domain) :
    Recorder(shape, time_domain)
{
}

bool SpikeRecorder::valid_output_port_data(
    size_t output_port_on_vertex, grenade::common::PortData const& results) const
{
	return output_port_on_vertex == 0 &&
	       dynamic_cast<SpikeRecorder::Results const*>(&results) != nullptr;
}

std::unique_ptr<grenade::common::Recorder::Results> SpikeRecorder::create_empty_results(
    size_t const batch_size_value) const
{
	return std::make_unique<Results>(batch_size_value, get_shape().size());
}

std::vector<grenade::common::Vertex::Port> SpikeRecorder::get_input_ports() const
{
	return {Port(
	    Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    Port::ExecutionInstanceTransitionConstraint::not_supported,
	    Port::RequiresOrGeneratesData::no, get_shape())};
}

std::vector<grenade::common::Vertex::Port> SpikeRecorder::get_output_ports() const
{
	return {Port(
	    Spike(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    Port::ExecutionInstanceTransitionConstraint::required, Port::RequiresOrGeneratesData::yes,
	    get_shape())};
}

std::unique_ptr<grenade::common::Vertex> SpikeRecorder::copy() const
{
	return std::make_unique<SpikeRecorder>(*this);
}

std::unique_ptr<grenade::common::Vertex> SpikeRecorder::move()
{
	return std::make_unique<SpikeRecorder>(std::move(*this));
}

std::ostream& SpikeRecorder::print(std::ostream& os) const
{
	os << "Spike";
	Recorder::print(os);
	return os;
}

} // namespace grenade::vx::network::abstract
