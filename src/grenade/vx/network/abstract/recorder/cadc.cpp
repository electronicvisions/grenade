#include "grenade/vx/network/abstract/recorder/cadc.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/port_data.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/vertex_port_type/analog_observable.h"
#include "grenade/vx/network/abstract/vertex_port_type/cadc_samples.h"
#include "hate/indent.h"
#include <memory>

namespace grenade::vx::network::abstract {

CADCRecorder::Results::Results(size_t batch_size, size_t size) : samples(batch_size)
{
	for (auto& batch_entry : samples) {
		batch_entry.resize(size);
	}
}

CADCRecorder::Results::Results(Samples samples) : samples(std::move(samples)) {}

size_t CADCRecorder::Results::batch_size() const
{
	return samples.size();
}

size_t CADCRecorder::Results::size() const
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

void CADCRecorder::Results::set_section(
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

std::unique_ptr<grenade::common::PortData> CADCRecorder::Results::copy() const
{
	return std::make_unique<CADCRecorder::Results>(*this);
}

std::unique_ptr<grenade::common::PortData> CADCRecorder::Results::move()
{
	return std::make_unique<CADCRecorder::Results>(std::move(*this));
}

bool CADCRecorder::Results::is_equal_to(grenade::common::PortData const& other) const
{
	return samples == static_cast<Results const&>(other).samples;
}

std::ostream& CADCRecorder::Results::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Results(\n";
	for (size_t b = 0; auto const& batch_entry : samples) {
		ios << hate::Indentation("\t");
		ios << "batch entry " << b << ":\n";
		for (size_t n = 0; auto const& neuron : batch_entry) {
			ios << hate::Indentation("\t\t");
			ios << "neuron " << n << ":\n";
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


CADCRecorder::CADCRecorder(
    grenade::common::MultiIndexSequence const& shape,
    bool const enable_record_to_dram,
    grenade::common::TimeDomainOnTopology const& time_domain) :
    Recorder(shape, time_domain), enable_record_to_dram(enable_record_to_dram)
{
}

bool CADCRecorder::valid_output_port_data(
    size_t output_port_on_vertex, grenade::common::PortData const& recording) const
{
	return output_port_on_vertex == 0 &&
	       dynamic_cast<CADCRecorder::Results const*>(&recording) != nullptr;
}

std::unique_ptr<grenade::common::Recorder::Results> CADCRecorder::create_empty_results(
    size_t const batch_size_value) const
{
	return std::make_unique<Results>(batch_size_value, get_shape().size());
}

std::vector<grenade::common::Vertex::Port> CADCRecorder::get_input_ports() const
{
	return {Port(
	    AnalogObservable(), Port::SumOrSplitSupport::no,
	    Port::ExecutionInstanceTransitionConstraint::not_supported,
	    Port::RequiresOrGeneratesData::no, get_shape())};
}

std::vector<grenade::common::Vertex::Port> CADCRecorder::get_output_ports() const
{
	return {Port(
	    CADCSamples(), Port::SumOrSplitSupport::no,
	    Port::ExecutionInstanceTransitionConstraint::required, Port::RequiresOrGeneratesData::yes,
	    get_shape())};
}

std::unique_ptr<grenade::common::Vertex> CADCRecorder::copy() const
{
	return std::make_unique<CADCRecorder>(*this);
}

std::unique_ptr<grenade::common::Vertex> CADCRecorder::move()
{
	return std::make_unique<CADCRecorder>(std::move(*this));
}

bool CADCRecorder::is_equal_to(Vertex const& other) const
{
	auto const& other_recorder = static_cast<CADCRecorder const&>(other);
	return Recorder::is_equal_to(other) &&
	       enable_record_to_dram == other_recorder.enable_record_to_dram;
}

std::ostream& CADCRecorder::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "CADCRecorder(\n";
	ios << hate::Indentation("\t");
	Recorder::print(ios);
	ios << "\n";
	ios << "enable_record_to_dram: " << enable_record_to_dram << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::vx::network::abstract
