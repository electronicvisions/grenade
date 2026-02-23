#include "grenade/vx/network/abstract/recorder/madc.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/network/abstract/vertex_port_type/analog_observable.h"
#include "grenade/vx/network/abstract/vertex_port_type/madc_samples.h"
#include "hate/indent.h"
#include <memory>

namespace grenade::vx::network::abstract {

MADCRecorder::Results::Results(size_t batch_size, size_t size) : samples(batch_size)
{
	for (auto& batch_entry : samples) {
		batch_entry.resize(size);
	}
}

MADCRecorder::Results::Results(Samples samples) : samples(std::move(samples)) {}

size_t MADCRecorder::Results::batch_size() const
{
	return samples.size();
}

size_t MADCRecorder::Results::size() const
{
	if (samples.empty()) {
		return 0;
	}
	size_t const ret = samples.at(0).size();
	for (size_t i = 1; i < samples.size(); ++i) {
		if (ret != samples.at(i).size()) {
			throw std::runtime_error("Size inhomogeneous across batch entries.");
		}
	}
	return ret;
}

void MADCRecorder::Results::set_section(
    Recorder::Results const& results, grenade::common::MultiIndexSequence const& sequence)
{
	if (auto const results_ptr = dynamic_cast<Results const*>(&results); results_ptr) {
		if (!grenade::common::CuboidMultiIndexSequence({size()}).includes(sequence)) {
			throw std::invalid_argument("Given sequence not part of results to set section into.");
		}
		if (batch_size() != results.batch_size()) {
			throw std::invalid_argument("Batch sizes don't match between results and section.");
		}
		for (size_t i = 0; auto const& element : sequence.get_elements()) {
			for (size_t b = 0; b < batch_size(); ++b) {
				samples.at(b).at(element.value.at(0)) = results_ptr->samples.at(b).at(i);
			}
			i++;
		}
	} else {
		throw std::invalid_argument(
		    "Given results not of same type than results to set section into.");
	}
}

std::unique_ptr<grenade::common::PortData> MADCRecorder::Results::copy() const
{
	return std::make_unique<MADCRecorder::Results>(*this);
}

std::unique_ptr<grenade::common::PortData> MADCRecorder::Results::move()
{
	return std::make_unique<MADCRecorder::Results>(std::move(*this));
}

bool MADCRecorder::Results::is_equal_to(grenade::common::PortData const& other) const
{
	return samples == static_cast<Results const&>(other).samples;
}

std::ostream& MADCRecorder::Results::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Results(\n";
	for (size_t b = 0; auto const& batch_entry : samples) {
		ios << hate::Indentation("\t");
		ios << "batch entry " << b << ":\n";
		for (size_t n = 0; auto const& neuron : batch_entry) {
			ios << hate::Indentation("\t\t");
			ios << "channel " << n << ":\n";
			ios << hate::Indentation("\t\t\t");
			for (auto const& sample : neuron) {
				ios << sample.first << ": " << sample.second << "\n";
			}
			n++;
		}
		b++;
	}
	ios << ")";
	return os;
}


MADCRecorder::MADCRecorder(
    grenade::common::MultiIndexSequence const& shape,
    grenade::common::TimeDomainOnTopology const& time_domain) :
    Recorder(shape, time_domain)
{
}

std::unique_ptr<grenade::common::Recorder::Results> MADCRecorder::create_empty_results(
    size_t const batch_size_value) const
{
	return std::make_unique<Results>(batch_size_value, get_shape().size());
}

bool MADCRecorder::valid_output_port_data(
    size_t output_port_on_vertex, grenade::common::PortData const& recording) const
{
	return output_port_on_vertex == 0 &&
	       dynamic_cast<MADCRecorder::Results const*>(&recording) != nullptr;
}

std::vector<grenade::common::Vertex::Port> MADCRecorder::get_input_ports() const
{
	return {Port(
	    AnalogObservable(), Port::SumOrSplitSupport::no,
	    Port::ExecutionInstanceTransitionConstraint::not_supported,
	    Port::RequiresOrGeneratesData::no, get_shape())};
}

std::vector<grenade::common::Vertex::Port> MADCRecorder::get_output_ports() const
{
	return {Port(
	    MADCSamples(), Port::SumOrSplitSupport::no,
	    Port::ExecutionInstanceTransitionConstraint::required, Port::RequiresOrGeneratesData::yes,
	    get_shape())};
}

std::unique_ptr<grenade::common::Vertex> MADCRecorder::copy() const
{
	return std::make_unique<MADCRecorder>(*this);
}

std::unique_ptr<grenade::common::Vertex> MADCRecorder::move()
{
	return std::make_unique<MADCRecorder>(std::move(*this));
}

std::ostream& MADCRecorder::print(std::ostream& os) const
{
	os << "MADC";
	Recorder::print(os);
	return os;
}

} // namespace grenade::vx::network::abstract
