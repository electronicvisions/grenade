#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"

#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/entity_on_chip.h"
#include "grenade/vx/signal_flow/vertex/neuron_view.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view_sparse.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "hate/math.h"
#include "hate/timer.h"
#include "hate/variant.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <inja/inja.hpp>
#include <log4cxx/logger.h>

namespace grenade::vx::signal_flow::vertex {

PlasticityRule::Timer::Value PlasticityRule::Timer::get_last() const
{
	return Value(start + period * num_periods);
}

bool PlasticityRule::Timer::operator==(Timer const& other) const
{
	return (start == other.start) && (period == other.period) && (num_periods == other.num_periods);
}

bool PlasticityRule::Timer::operator!=(Timer const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::Timer const& timer)
{
	os << "Timer(start: " << timer.start << ", period: " << timer.period
	   << ", num_periods: " << timer.num_periods << ")";
	return os;
}

PlasticityRule::RawRecording::RawRecording() : scratchpad_memory_size(0), placement_in_dram(true) {}
PlasticityRule::RawRecording::RawRecording(size_t scratchpad_memory_size, bool placement_in_dram) :
    scratchpad_memory_size(scratchpad_memory_size), placement_in_dram(placement_in_dram)
{}

bool PlasticityRule::RawRecording::operator==(RawRecording const& other) const
{
	return scratchpad_memory_size == other.scratchpad_memory_size &&
	       placement_in_dram == other.placement_in_dram;
}

bool PlasticityRule::RawRecording::operator!=(RawRecording const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::RawRecording const& recording)
{
	os << "RawRecording(scratchpad_memory_size: " << recording.scratchpad_memory_size;
	os << ", placement in dram: " << recording.placement_in_dram;
	os << ")";
	return os;
}

PlasticityRule::TimedRecording::ObservablePerSynapse::ObservablePerSynapse(
    TypeVariant const& type, LayoutPerRow const& layout_per_row) :
    type(type), layout_per_row(layout_per_row)
{}

bool PlasticityRule::TimedRecording::ObservablePerSynapse::operator==(
    ObservablePerSynapse const& other) const
{
	return (type == other.type) && (layout_per_row == other.layout_per_row);
}

bool PlasticityRule::TimedRecording::ObservablePerSynapse::operator!=(
    ObservablePerSynapse const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(
    std::ostream& os, PlasticityRule::TimedRecording::ObservablePerSynapse const& observable)
{
	os << "ObservablePerSynapse(type: ";
	std::visit([&os](auto const& type) { os << type; }, observable.type);
	os << ", layout_per_row: " << observable.layout_per_row << ")";
	return os;
}

PlasticityRule::TimedRecording::ObservablePerNeuron::ObservablePerNeuron(
    TypeVariant const& type, Layout const& layout) :
    type(type), layout(layout)
{}

bool PlasticityRule::TimedRecording::ObservablePerNeuron::operator==(
    ObservablePerNeuron const& other) const
{
	return (type == other.type) && (layout == other.layout);
}

bool PlasticityRule::TimedRecording::ObservablePerNeuron::operator!=(
    ObservablePerNeuron const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(
    std::ostream& os, PlasticityRule::TimedRecording::ObservablePerNeuron const& observable)
{
	os << "ObservablePerNeuron(type: ";
	std::visit([&os](auto const& type) { os << type; }, observable.type);
	os << ", layout: " << observable.layout << ")";
	return os;
}

PlasticityRule::TimedRecording::ObservableArray::ObservableArray(
    TypeVariant const& type, size_t const size) :
    type(type), size(size)
{}

bool PlasticityRule::TimedRecording::ObservableArray::operator==(ObservableArray const& other) const
{
	return (type == other.type) && (size == other.size);
}

bool PlasticityRule::TimedRecording::ObservableArray::operator!=(ObservableArray const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(
    std::ostream& os, PlasticityRule::TimedRecording::ObservableArray const& observable)
{
	os << "ObservableArray(type: ";
	std::visit([&os](auto const& type) { os << type; }, observable.type);
	os << ", size: " << observable.size << ")";
	return os;
}

PlasticityRule::TimedRecording::TimedRecording() : observables(), placement_in_dram(true) {}
PlasticityRule::TimedRecording::TimedRecording(
    std::map<std::string, Observable> observables, bool placement_in_dram) :
    observables(std::move(observables)), placement_in_dram(placement_in_dram)
{}

bool PlasticityRule::TimedRecording::operator==(TimedRecording const& other) const
{
	return observables == other.observables && placement_in_dram == other.placement_in_dram;
}

bool PlasticityRule::TimedRecording::operator!=(TimedRecording const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::TimedRecording const& recording)
{
	os << "TimedRecording(observables:\n";
	for (auto const& [name, type] : recording.observables) {
		os << "\t" << name << ": ";
		std::visit([&](auto const& t) { os << t; }, type);
		os << "\n";
	}
	os << "\tplacement in dram: " << recording.placement_in_dram << "\n";
	os << ")";
	return os;
}

bool PlasticityRule::SynapseViewShape::operator==(SynapseViewShape const& other) const
{
	return (num_rows == other.num_rows) && (columns.size() == other.columns.size());
}

bool PlasticityRule::SynapseViewShape::operator!=(SynapseViewShape const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::SynapseViewShape const& shape)
{
	os << "SynapseViewShape(num_rows: " << shape.num_rows
	   << ", columns.size(): " << shape.columns.size() << ")";
	return os;
}

bool PlasticityRule::NeuronViewShape::operator==(NeuronViewShape const& other) const
{
	return (columns.size() == other.columns.size()) && row == other.row;
}

bool PlasticityRule::NeuronViewShape::operator!=(NeuronViewShape const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::NeuronViewShape const& shape)
{
	os << "NeuronViewShape(\n";
	os << "\tcolumns:\n";
	for (auto const& column : shape.columns) {
		os << "\t\t" << column << "\n";
	}
	os << "\trow: " << shape.row << "\n";
	os << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::RawRecordingData const& data)
{
	os << "RawRecordingData(data:\n";
	for (size_t i = 0; i < data.data.size(); ++i) {
		os << "\tbatch entry " << i << ":\n";
		os << "\t\t" << hate::join(data.data.at(i), ", ") << "\n";
	}
	os << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::TimedRecordingData const& data)
{
	os << "TimedRecordingData(\n";
	os << "\tdata_per_synapse:\n";
	for (auto const& [name, d] : data.data_per_synapse) {
		for (size_t p = 0; p < d.size(); ++p) {
			std::visit(
			    [&os, &name, p](auto const& dd) {
				    for (size_t b = 0; b < dd.size(); ++b) {
					    for (size_t s = 0; s < dd.at(b).size(); ++s) {
						    os << "\t\tobservable(" << name << "), synapse_view(" << p
						       << "), batch_entry(" << b << "), sample(" << s << "):\n";
						    os << "\t\t\t" << dd.at(b).at(s).time << "\n";
						    std::vector<intmax_t> copy(
						        dd.at(b).at(s).data.begin(), dd.at(b).at(s).data.end());
						    os << "\t\t\t" << hate::join(copy, ", ") << "\n";
					    }
				    }
			    },
			    d.at(p));
		}
	}
	os << "\tdata_per_neuron:\n";
	for (auto const& [name, d] : data.data_per_neuron) {
		for (size_t p = 0; p < d.size(); ++p) {
			std::visit(
			    [&os, &name, p](auto const& dd) {
				    for (size_t b = 0; b < dd.size(); ++b) {
					    for (size_t s = 0; s < dd.at(b).size(); ++s) {
						    os << "\t\tobservable(" << name << "), neuron_view(" << p
						       << "), batch_entry(" << b << "), sample(" << s << "):\n";
						    os << "\t\t\t" << dd.at(b).at(s).time << "\n";
						    std::vector<intmax_t> copy(
						        dd.at(b).at(s).data.begin(), dd.at(b).at(s).data.end());
						    os << "\t\t\t" << hate::join(copy, ", ") << "\n";
					    }
				    }
			    },
			    d.at(p));
		}
	}
	os << "\tdata_array:\n";
	for (auto const& [name, d] : data.data_array) {
		std::visit(
		    [&os, &name](auto const& dd) {
			    for (size_t b = 0; b < dd.size(); ++b) {
				    for (size_t s = 0; s < dd.at(b).size(); ++s) {
					    os << "\t\tobservable(" << name << "), batch_entry(" << b << "), sample("
					       << s << "):\n";
					    os << "\t\t\t" << dd.at(b).at(s).time << "\n";
					    std::vector<intmax_t> copy(
					        dd.at(b).at(s).data.begin(), dd.at(b).at(s).data.end());
					    os << "\t\t\t" << hate::join(copy, ", ") << "\n";
				    }
			    }
		    },
		    d);
	}
	os << ")";
	return os;
}


PlasticityRule::Results::Results(RecordingData data) : data(std::move(data)) {}

size_t PlasticityRule::Results::batch_size() const
{
	return std::visit(
	    hate::overloaded{
	        [](RawRecordingData const& d) { return d.data.size(); },
	        [](TimedRecordingData const& d) {
		        if (!d.data_per_synapse.empty()) {
			        return std::visit(
			            [](auto const& dd) { return dd.size(); },
			            d.data_per_synapse.begin()->second.at(0));
		        }
		        if (!d.data_per_neuron.empty()) {
			        return std::visit(
			            [](auto const& dd) { return dd.size(); },
			            d.data_per_neuron.begin()->second.at(0));
		        }
		        if (!d.data_array.empty()) {
			        return std::visit(
			            [](auto const& dd) { return dd.size(); }, d.data_array.begin()->second);
		        }
		        throw std::runtime_error("Empty timed recording doesn't define a batch size.");
	        }},
	    data);
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Results::copy() const
{
	return std::make_unique<Results>(*this);
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Results::move()
{
	return std::make_unique<Results>(std::move(*this));
}

std::ostream& PlasticityRule::Results::print(std::ostream& os) const
{
	std::visit(
	    [&os](auto const& d) {
		    hate::IndentingOstream ios(os);
		    ios << "Results(\n";
		    ios << hate::Indentation("\t");
		    ios << d << "\n";
		    ios << hate::Indentation();
		    ios << ")";
	    },
	    data);
	return os;
}

bool PlasticityRule::Results::is_equal_to(grenade::common::PortData const& other) const
{
	return data == static_cast<Results const&>(other).data;
}


PlasticityRule::Parameterization::Parameterization(std::string kernel) : kernel(std::move(kernel))
{
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

std::ostream& PlasticityRule::Parameterization::print(std::ostream& os) const
{
	return os << "Parameterization(kernel: " << kernel << ")";
}

bool PlasticityRule::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	auto const& other_parameterization = static_cast<Parameterization const&>(other);
	return kernel == other_parameterization.kernel;
}


PlasticityRule::Dynamics::Dynamics(size_t batch_size, Timer timer) :
    timer(std::move(timer)), m_batch_size(batch_size)
{
}

size_t PlasticityRule::Dynamics::batch_size() const
{
	return m_batch_size;
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Dynamics::copy() const
{
	return std::make_unique<Dynamics>(*this);
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Dynamics::move()
{
	return std::make_unique<Dynamics>(std::move(*this));
}

bool PlasticityRule::Dynamics::is_equal_to(grenade::common::PortData const& other) const
{
	return timer == static_cast<Dynamics const&>(other).timer;
}

std::ostream& PlasticityRule::Dynamics::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Dynamics(\n";
	ios << hate::Indentation("\t");
	ios << timer << "\n";
	ios << "batch size: " << m_batch_size;
	ios << hate::Indentation() << "\n)";
	return os;
}


PlasticityRule::PlasticityRule(
    std::vector<SynapseViewShape> const& synapse_view_shapes,
    std::vector<NeuronViewShape> const& neuron_view_shapes,
    std::optional<Recording> const& recording,
    ID const& id,
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor),
    m_synapse_view_shapes(synapse_view_shapes),
    m_neuron_view_shapes(neuron_view_shapes),
    m_recording(recording),
    m_id(id)
{}

size_t PlasticityRule::get_recorded_scratchpad_memory_size() const
{
	if (!m_recording) {
		return 0;
	}

	if (std::holds_alternative<RawRecording>(*m_recording)) {
		return std::get<RawRecording>(*m_recording).scratchpad_memory_size;
	} else if (std::holds_alternative<TimedRecording>(*m_recording)) {
		size_t size = 8;
		for (auto const& [_, type] : std::get<TimedRecording>(*m_recording).observables) {
			std::visit(
			    hate::overloaded{
			        [&](TimedRecording::ObservablePerSynapse const& observable) {
				        switch (observable.layout_per_row) {
					        case TimedRecording::ObservablePerSynapse::LayoutPerRow::
					            complete_rows: {
						        size_t num_synapse_rows = 0;
						        for (auto const& synapse_view_shape : m_synapse_view_shapes) {
							        num_synapse_rows += synapse_view_shape.num_rows;
						        }
						        size_t const element_size = std::visit(
						            [](auto t) {
							            return sizeof(typename decltype(t)::ElementType);
						            },
						            observable.type);
						        size = hate::math::round_up_to_multiple(size, 128);
						        size += num_synapse_rows * element_size *
						                halco::hicann_dls::vx::v3::SynapseOnSynapseRow::size;
						        break;
					        }
					        case TimedRecording::ObservablePerSynapse::LayoutPerRow::
					            packed_active_columns: {
						        size_t const element_size = std::visit(
						            [](auto t) {
							            return sizeof(typename decltype(t)::ElementType);
						            },
						            observable.type);
						        size = hate::math::round_up_to_multiple(size, element_size);
						        for (auto const& synapse_view_shape : m_synapse_view_shapes) {
							        size += synapse_view_shape.num_rows *
							                synapse_view_shape.columns.size() * element_size;
						        }
						        break;
					        }
					        default: {
						        throw std::logic_error(
						            "Observable per synapse layout per row not implemented.");
					        }
				        }
			        },
			        [&](TimedRecording::ObservablePerNeuron const& observable) {
				        switch (observable.layout) {
					        case TimedRecording::ObservablePerNeuron::Layout::complete_row: {
						        size_t const element_size = std::visit(
						            [](auto t) {
							            return sizeof(typename decltype(t)::ElementType);
						            },
						            observable.type);
						        size = hate::math::round_up_to_multiple(size, 128);
						        size += m_neuron_view_shapes.size() * element_size *
						                halco::hicann_dls::vx::v3::NeuronColumnOnDLS::size;
						        break;
					        }
					        case TimedRecording::ObservablePerNeuron::Layout::
					            packed_active_columns: {
						        size_t const element_size = std::visit(
						            [](auto t) {
							            return sizeof(typename decltype(t)::ElementType);
						            },
						            observable.type);
						        size = hate::math::round_up_to_multiple(size, element_size);
						        for (auto const& neuron_view_shape : m_neuron_view_shapes) {
							        size += neuron_view_shape.columns.size() * element_size;
						        }
						        break;
					        }
					        default: {
						        throw std::logic_error(
						            "Observable per neuron layout not implemented.");
					        }
				        }
			        },
			        [&](TimedRecording::ObservableArray const& observable) {
				        size_t const element_size = std::visit(
				            [](auto t) { return sizeof(typename decltype(t)::ElementType); },
				            observable.type);
				        size = hate::math::round_up_to_multiple(size, element_size);
				        size += observable.size * element_size;
			        },
			        [](auto const&) { throw std::logic_error("Observable type not implemented."); },
			    },
			    type);
		}
		size = hate::math::round_up_to_multiple(size, get_recorded_scratchpad_memory_alignment());
		return size;
	} else {
		throw std::logic_error("Recording type not implemented.");
	}
}

size_t PlasticityRule::get_recorded_scratchpad_memory_alignment() const
{
	if (!m_recording) {
		return 0;
	}
	if (std::holds_alternative<TimedRecording>(*m_recording)) {
		bool has_vector = false;
		for (auto const& [_, type] : std::get<TimedRecording>(*m_recording).observables) {
			has_vector = has_vector ||
			             (std::holds_alternative<TimedRecording::ObservablePerSynapse>(type) &&
			              (std::get<TimedRecording::ObservablePerSynapse>(type).layout_per_row ==
			               TimedRecording::ObservablePerSynapse::LayoutPerRow::complete_rows)) ||
			             (std::holds_alternative<TimedRecording::ObservablePerNeuron>(type) &&
			              (std::get<TimedRecording::ObservablePerNeuron>(type).layout ==
			               TimedRecording::ObservablePerNeuron::Layout::complete_row));
		}
		if (has_vector) {
			// TODO: replace by constant
			return 128;
		}
	}
	return sizeof(uint64_t /* time value */);
}

void to_json(inja::json& j, PlasticityRule::SynapseViewShape const& shape)
{
	j = inja::json{{"num_rows", shape.num_rows}, {"num_columns", shape.columns.size()}};
}

void to_json(inja::json& j, PlasticityRule::NeuronViewShape const& shape)
{
	j = inja::json{{"num_columns", shape.columns.size()}};
}

std::string PlasticityRule::get_recorded_memory_declaration() const
{
	if (!m_recording) {
		return "";
	}
	if (std::holds_alternative<RawRecording>(*m_recording)) {
		// clang-format off
		std::string source_template = R"grenadeTemplate(
#include <array>

struct Recording
{
	std::array<char, {{scratchpad_memory_size}}> memory;
};
)grenadeTemplate";
		// clang-format on
		inja::json parameters;
		parameters["scratchpad_memory_size"] =
		    std::get<RawRecording>(*m_recording).scratchpad_memory_size;
		return inja::render(source_template, parameters);
	} else if (std::holds_alternative<TimedRecording>(*m_recording)) {
		// clang-format off
		std::string source_template = R"grenadeTemplate(
#include <array>
#include <tuple>
#include "libnux/vx/vector_row.h"

struct Recording
{
	uint64_t time;

## if exists("observables")
## for observable in observables
## if observable.is_array
	std::array<{{observable.type}}, {{observable.size}}> {{observable.name}};
## else if observable.is_synapse
	std::tuple<
## for synapse_view in synapse_views
## if observable.layout_per_row == "complete_rows"
	    std::array<{{observable.type}}, {{synapse_view.num_rows}}>{% if not loop.is_last %},{% endif %}
## else if observable.layout_per_row == "packed_active_columns"
	    std::array<std::array<{{observable.type}}, {{synapse_view.num_columns}}>, {{synapse_view.num_rows}}>{% if not loop.is_last %},{% endif %}
## endif
## endfor
	    > {{observable.name}};
## else
	std::tuple<
## for neuron_view in neuron_views
## if observable.layout == "complete_row"
	    {{observable.type}}{% if not loop.is_last %},{% endif %}
## else if observable.layout == "packed_active_columns"
	    std::array<{{observable.type}}, {{neuron_view.num_columns}}>{% if not loop.is_last %},{% endif %}
## endif
## endfor
	    > {{observable.name}};
## endif
## endfor
## endif
};

namespace {

template <size_t RealSize>
void static_assert_size()
{
	static_assert(RealSize == {{recorded_scratchpad_memory_size}}, "Recording size doesn't match expectation.");
}

[[maybe_unused]] void static_assert_size_eval()
{
	static_assert_size<sizeof(Recording)>();
}

} // namespace
)grenadeTemplate";
		// clang-format on

		inja::json parameters;
		std::vector<size_t> observable_array_scratchpad_memory_sizes;
		for (auto const& [name, type] : std::get<TimedRecording>(*m_recording).observables) {
			std::visit(
			    hate::overloaded{
			        [&parameters, &name](TimedRecording::ObservableArray const& observable) {
				        std::string const observable_type_str = [&observable]() {
					        std::stringstream ss;
					        std::visit([&ss](auto const& t) { ss << t << "_t"; }, observable.type);
					        return ss.str();
				        }();
				        parameters["observables"].push_back(
				            {{"is_array", true},
				             {"name", name},
				             {"size", observable.size},
				             {"type", observable_type_str}});
			        },
			        [&parameters, name,
			         this](TimedRecording::ObservablePerSynapse const& observable) {
				        std::string layout_per_row = [&observable]() {
					        std::stringstream ss;
					        ss << observable.layout_per_row;
					        return ss.str();
				        }();
				        std::string type;
				        switch (observable.layout_per_row) {
					        case TimedRecording::ObservablePerSynapse::LayoutPerRow::
					            complete_rows: {
						        std::visit(
						            [&type](auto t) { type = decltype(t)::on_ppu_type; },
						            observable.type);
						        break;
					        }
					        case TimedRecording::ObservablePerSynapse::LayoutPerRow::
					            packed_active_columns: {
						        type = [&observable]() {
							        std::stringstream ss;
							        std::visit(
							            [&ss](auto const& t) { ss << t << "_t"; }, observable.type);
							        return ss.str();
						        }();
						        break;
					        }
					        default: {
						        throw std::logic_error("Unknown layout per row.");
					        }
				        }
				        parameters["observables"].push_back(
				            {{"is_array", false},
				             {"is_synapse", true},
				             {"name", name},
				             {"type", type},
				             {"layout_per_row", layout_per_row}});
			        },
			        [&](TimedRecording::ObservablePerNeuron const& observable) {
				        std::string layout = [&observable]() {
					        std::stringstream ss;
					        ss << observable.layout;
					        return ss.str();
				        }();
				        std::string type;
				        switch (observable.layout) {
					        case TimedRecording::ObservablePerNeuron::Layout::complete_row: {
						        std::visit(
						            [&type](auto t) { type = decltype(t)::on_ppu_type; },
						            observable.type);
						        break;
					        }
					        case TimedRecording::ObservablePerNeuron::Layout::
					            packed_active_columns: {
						        type = [&observable]() {
							        std::stringstream ss;
							        std::visit(
							            [&ss](auto const& t) { ss << t << "_t"; }, observable.type);
							        return ss.str();
						        }();
						        break;
					        }
					        default: {
						        throw std::logic_error("Unknown layout.");
					        }
				        }
				        parameters["observables"].push_back(
				            {{"is_array", false},
				             {"is_synapse", false},
				             {"name", name},
				             {"type", type},
				             {"layout", layout}});
			        },
			        [](auto const&) {
				        throw std::logic_error("Observable type not implemented.");
			        }},
			    type);
		}
		parameters["synapse_views"] = m_synapse_view_shapes;
		parameters["neuron_views"] = m_neuron_view_shapes;
		parameters["recorded_scratchpad_memory_size"] = get_recorded_scratchpad_memory_size();

		return inja::render(source_template, parameters);
	} else {
		throw std::logic_error("Recording type not implemented.");
	}
}

std::pair<size_t, size_t> PlasticityRule::get_recorded_memory_data_interval() const
{
	if (!m_recording) {
		return std::pair<size_t, size_t>{0, 0};
	}

	if (std::holds_alternative<RawRecording>(*m_recording)) {
		return std::pair<size_t, size_t>{
		    0, std::get<RawRecording>(*m_recording).scratchpad_memory_size};
	} else if (std::holds_alternative<TimedRecording>(*m_recording)) {
		auto const& observables = std::get<TimedRecording>(*m_recording).observables;
		if (observables.empty()) {
			return std::pair<size_t, size_t>{sizeof(uint64_t), sizeof(uint64_t)};
		}
		auto const& [_, obsv] = *(observables.begin());
		return std::visit(
		    hate::overloaded{
		        [&](TimedRecording::ObservablePerSynapse const& observable) {
			        switch (observable.layout_per_row) {
				        case TimedRecording::ObservablePerSynapse::LayoutPerRow::complete_rows: {
					        return std::pair<size_t, size_t>{
					            128, get_recorded_scratchpad_memory_size()};
					        break;
				        }
				        case TimedRecording::ObservablePerSynapse::LayoutPerRow::
				            packed_active_columns: {
					        return std::pair<size_t, size_t>{
					            sizeof(uint64_t), get_recorded_scratchpad_memory_size()};
					        break;
				        }
				        default: {
					        throw std::logic_error(
					            "Observable per synapse layout per row not implemented.");
				        }
			        }
		        },
		        [&](TimedRecording::ObservablePerNeuron const& observable) {
			        switch (observable.layout) {
				        case TimedRecording::ObservablePerNeuron::Layout::complete_row: {
					        return std::pair<size_t, size_t>{
					            128, get_recorded_scratchpad_memory_size()};
					        break;
				        }
				        case TimedRecording::ObservablePerNeuron::Layout::packed_active_columns: {
					        return std::pair<size_t, size_t>{
					            sizeof(uint64_t), get_recorded_scratchpad_memory_size()};
					        break;
				        }
				        default: {
					        throw std::logic_error("Observable per neuron layout not implemented.");
				        }
			        }
		        },
		        [&](TimedRecording::ObservableArray const&) {
			        return std::pair<size_t, size_t>{
			            sizeof(uint64_t), get_recorded_scratchpad_memory_size()};
		        },
		        [](auto const&) { throw std::logic_error("Observable type not implemented."); },
		    },
		    obsv);
	} else {
		throw std::logic_error("Recording type not implemented.");
	}
}

std::map<std::string, std::pair<size_t, size_t>>
PlasticityRule::get_recorded_memory_timed_data_intervals() const
{
	if (!m_recording) {
		throw std::runtime_error("No recording present to get timed data intervals for.");
	}

	if (!std::holds_alternative<TimedRecording>(*m_recording)) {
		throw std::runtime_error("Present recording not timed.");
	}

	std::map<std::string, std::pair<size_t, size_t>> ret;
	auto const [absolute_begin, _] = get_recorded_memory_data_interval();
	size_t begin = 8;
	for (auto const& [name, observable] : std::get<TimedRecording>(*m_recording).observables) {
		std::visit(
		    hate::overloaded{
		        [&](TimedRecording::ObservablePerSynapse const& observable) {
			        switch (observable.layout_per_row) {
				        case TimedRecording::ObservablePerSynapse::LayoutPerRow::complete_rows: {
					        size_t num_synapse_rows = 0;
					        for (auto const& synapse_view_shape : m_synapse_view_shapes) {
						        num_synapse_rows += synapse_view_shape.num_rows;
					        }
					        size_t const element_size = std::visit(
					            [](auto t) { return sizeof(typename decltype(t)::ElementType); },
					            observable.type);
					        begin = hate::math::round_up_to_multiple(begin, 128);
					        ret[name].first = begin - absolute_begin;
					        begin += num_synapse_rows * element_size *
					                 halco::hicann_dls::vx::v3::SynapseOnSynapseRow::size;
					        ret.at(name).second = begin - absolute_begin;
					        break;
				        }
				        case TimedRecording::ObservablePerSynapse::LayoutPerRow::
				            packed_active_columns: {
					        size_t const element_size = std::visit(
					            [](auto t) { return sizeof(typename decltype(t)::ElementType); },
					            observable.type);
					        begin = hate::math::round_up_to_multiple(begin, element_size);
					        ret[name].first = begin - absolute_begin;
					        for (auto const& synapse_view_shape : m_synapse_view_shapes) {
						        begin += synapse_view_shape.num_rows *
						                 synapse_view_shape.columns.size() * element_size;
					        }
					        ret.at(name).second = begin - absolute_begin;
					        break;
				        }
				        default: {
					        throw std::logic_error(
					            "Observable per synapse layout per row not implemented.");
				        }
			        }
		        },
		        [&](TimedRecording::ObservablePerNeuron const& observable) {
			        switch (observable.layout) {
				        case TimedRecording::ObservablePerNeuron::Layout::complete_row: {
					        size_t const element_size = std::visit(
					            [](auto t) { return sizeof(typename decltype(t)::ElementType); },
					            observable.type);
					        begin = hate::math::round_up_to_multiple(begin, 128);
					        ret[name].first = begin - absolute_begin;
					        begin += m_neuron_view_shapes.size() * element_size *
					                 halco::hicann_dls::vx::v3::NeuronColumnOnDLS::size;
					        ret.at(name).second = begin - absolute_begin;
					        break;
				        }
				        case TimedRecording::ObservablePerNeuron::Layout::packed_active_columns: {
					        size_t const element_size = std::visit(
					            [](auto t) { return sizeof(typename decltype(t)::ElementType); },
					            observable.type);
					        begin = hate::math::round_up_to_multiple(begin, element_size);
					        ret[name].first = begin - absolute_begin;
					        for (auto const& neuron_view_shape : m_neuron_view_shapes) {
						        begin += neuron_view_shape.columns.size() * element_size;
					        }
					        ret.at(name).second = begin - absolute_begin;
					        break;
				        }
				        default: {
					        throw std::logic_error("Observable per neuron layout not implemented.");
				        }
			        }
		        },
		        [&](TimedRecording::ObservableArray const& observable) {
			        size_t const element_size = std::visit(
			            [](auto t) { return sizeof(typename decltype(t)::ElementType); },
			            observable.type);
			        begin = hate::math::round_up_to_multiple(begin, element_size);
			        ret[name].first = begin - absolute_begin;
			        begin += observable.size * element_size;
			        ret.at(name).second = begin - absolute_begin;
		        },
		        [](auto const&) { throw std::logic_error("Observable type not implemented."); },
		    },
		    observable);
	}
	return ret;
}

PlasticityRule::RecordingData PlasticityRule::extract_recording_data(
    std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>> const& data) const
{
	hate::Timer timer;
	if (!m_recording) {
		throw std::runtime_error("Observable extraction only possible when recording is present.");
	}

	size_t const output_size =
	    get_recorded_memory_data_interval().second - get_recorded_memory_data_interval().first;
	for (auto const& batch : data) {
		for (auto const& sample : batch) {
			if (sample.data.size() != output_size) {
				throw std::runtime_error(
				    "Size of given data (" + std::to_string(sample.data.size()) +
				    ") from which to extract recorded "
				    "data does not match expectation (" +
				    std::to_string(output_size) + ").");
			}
		}
	}

	if (std::holds_alternative<RawRecording>(*m_recording)) {
		for (auto const& batch : data) {
			if (batch.size() != 1) {
				throw std::runtime_error(
				    "RawRecording expects exactly one sample per batch entry, but got " +
				    std::to_string(batch.size()) + ".");
			}
		}

		RawRecordingData recording;
		recording.data.resize(data.size());
		for (size_t i = 0; i < recording.data.size(); ++i) {
			recording.data.at(i) = data.at(i).at(0).data;
		}
		LOG4CXX_TRACE(
		    log4cxx::Logger::getLogger("grenade.signal_flow.vertex."
		                               "PlasticityRule.extract_recording_data"),
		    recording);
		return recording;
	} else if (!std::holds_alternative<TimedRecording>(*m_recording)) {
		throw std::runtime_error("Recording extraction for given recording type not supported.");
	}

	TimedRecordingData observable_data;

	auto const timed_data_intervals = get_recorded_memory_timed_data_intervals();

	auto const extract_observable_per_synapse_complete_rows = [&]([[maybe_unused]] auto const& type,
	                                                              auto const& name) {
		auto& data_per_synapse = observable_data.data_per_synapse[name];
		data_per_synapse.resize(m_synapse_view_shapes.size());
		// iterate in reversed order because std::tuple memory
		// layout is last element first
		size_t synapse_view_offset = 0;
		for (int synapse_view = data_per_synapse.size() - 1; synapse_view >= 0; --synapse_view) {
			auto const& synapse_view_shape = m_synapse_view_shapes.at(synapse_view);
			typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
			std::vector<common::TimedDataSequence<std::vector<ElementType>>> batch_values(
			    data.size());
			size_t local_synapse_view_offset = 0;
			for (size_t batch = 0; batch < batch_values.size(); ++batch) {
				auto& local_batch_values = batch_values.at(batch);
				auto const& local_data = data.at(batch);
				local_batch_values.resize(local_data.size());
				size_t sample_local_synapse_view_offset = 0;
				for (size_t sample = 0; sample < local_batch_values.size(); ++sample) {
					auto& local_sample = local_batch_values.at(sample);
					auto& local_data_sample = local_data.at(sample);
					local_sample.time = local_data_sample.time;
					local_sample.data.resize(
					    synapse_view_shape.num_rows * synapse_view_shape.columns.size());
					for (size_t synapse = 0; synapse < local_sample.data.size(); ++synapse) {
						size_t const row_offset =
						    (synapse / synapse_view_shape.columns.size()) *
						    halco::hicann_dls::vx::v3::SynapseOnSynapseRow::size;
						size_t const column_parity_offset =
						    (synapse_view_shape.columns
						         .at(synapse % synapse_view_shape.columns.size())
						         .value() %
						     2) *
						    halco::hicann_dls::vx::v3::SynapseOnSynapseRow::size / 2;
						size_t const column_in_parity_offset =
						    synapse_view_shape.columns
						        .at(synapse % synapse_view_shape.columns.size())
						        .value() /
						    2;
						size_t const total_offset =
						    (sizeof(ElementType) *
						     (row_offset + column_parity_offset + column_in_parity_offset)) +
						    timed_data_intervals.at(name).first;
						std::array<int8_t, sizeof(ElementType)> local_sample_bytes;
						for (size_t byte = 0; byte < sizeof(ElementType); ++byte) {
							local_sample_bytes.at(sizeof(ElementType) - 1 - byte) =
							    local_data_sample.data.at(total_offset + byte).value();
						}
						local_sample.data.at(synapse) =
						    *reinterpret_cast<ElementType*>(&local_sample_bytes);
					}
					sample_local_synapse_view_offset += sizeof(ElementType) *
					                                    synapse_view_shape.columns.size() *
					                                    synapse_view_shape.num_rows;
				}
				local_synapse_view_offset += sample_local_synapse_view_offset /
				                             (batch_values.size() * local_batch_values.size());
			}
			synapse_view_offset += local_synapse_view_offset;
			data_per_synapse.at(synapse_view) = batch_values;
		}
	};

	auto const extract_observable_per_synapse_packed_active_columns = [&](auto const& type,
	                                                                      auto const& name) {
		auto& data_per_synapse = observable_data.data_per_synapse[name];
		data_per_synapse.resize(m_synapse_view_shapes.size());
		// iterate in reversed order because std::tuple memory
		// layout is last element first
		size_t synapse_view_offset = 0;
		for (int synapse_view = data_per_synapse.size() - 1; synapse_view >= 0; --synapse_view) {
			auto const& synapse_view_shape = m_synapse_view_shapes.at(synapse_view);
			typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
			std::vector<common::TimedDataSequence<std::vector<ElementType>>> batch_values(
			    data.size());
			size_t local_synapse_view_offset = 0;
			for (size_t batch = 0; batch < batch_values.size(); ++batch) {
				auto& local_batch_values = batch_values.at(batch);
				auto const& local_data = data.at(batch);
				local_batch_values.resize(local_data.size());
				size_t sample_local_synapse_view_offset = 0;
				for (size_t sample = 0; sample < local_batch_values.size(); ++sample) {
					auto& local_sample = local_batch_values.at(sample);
					auto& local_data_sample = local_data.at(sample);
					local_sample.time = local_data_sample.time;
					local_sample.data.resize(
					    synapse_view_shape.num_rows * synapse_view_shape.columns.size());
					for (size_t synapse = 0; synapse < local_sample.data.size(); ++synapse) {
						size_t const total_offset = (sizeof(ElementType) * synapse) +
						                            timed_data_intervals.at(name).first +
						                            synapse_view_offset;
						std::array<int8_t, sizeof(ElementType)> local_sample_bytes;
						for (size_t byte = 0; byte < sizeof(ElementType); ++byte) {
							local_sample_bytes.at(sizeof(ElementType) - 1 - byte) =
							    local_data_sample.data.at(total_offset + byte).value();
						}
						local_sample.data.at(synapse) =
						    *reinterpret_cast<ElementType*>(&local_sample_bytes);
					}
					sample_local_synapse_view_offset +=
					    sizeof(ElementType) * local_sample.data.size();
				}
				local_synapse_view_offset += sample_local_synapse_view_offset /
				                             (batch_values.size() * local_batch_values.size());
			}
			synapse_view_offset += local_synapse_view_offset;
			data_per_synapse.at(synapse_view) = batch_values;
		}
	};

	auto const extract_observable_per_neuron_complete_row = [&]([[maybe_unused]] auto const& type,
	                                                            auto const& name) {
		auto& data_per_neuron = observable_data.data_per_neuron[name];
		data_per_neuron.resize(m_neuron_view_shapes.size());
		// iterate in reversed order because std::tuple memory
		// layout is last element first
		size_t neuron_view_offset = 0;
		for (int neuron_view = data_per_neuron.size() - 1; neuron_view >= 0; --neuron_view) {
			auto const& neuron_view_shape = m_neuron_view_shapes.at(neuron_view);
			typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
			std::vector<common::TimedDataSequence<std::vector<ElementType>>> batch_values(
			    data.size());
			size_t local_neuron_view_offset = 0;
			for (size_t batch = 0; batch < batch_values.size(); ++batch) {
				auto& local_batch_values = batch_values.at(batch);
				auto const& local_data = data.at(batch);
				local_batch_values.resize(local_data.size());
				size_t sample_local_neuron_view_offset = 0;
				for (size_t sample = 0; sample < local_batch_values.size(); ++sample) {
					auto& local_sample = local_batch_values.at(sample);
					auto& local_data_sample = local_data.at(sample);
					local_sample.time = local_data_sample.time;
					local_sample.data.resize(neuron_view_shape.columns.size());
					for (size_t neuron = 0; neuron < local_sample.data.size(); ++neuron) {
						size_t const column_parity_offset =
						    (neuron_view_shape.columns.at(neuron % neuron_view_shape.columns.size())
						         .value() %
						     2) *
						    halco::hicann_dls::vx::v3::SynapseOnSynapseRow::size / 2;
						size_t const column_in_parity_offset =
						    neuron_view_shape.columns.at(neuron % neuron_view_shape.columns.size())
						        .value() /
						    2;
						size_t const total_offset =
						    (sizeof(ElementType) *
						     (column_parity_offset + column_in_parity_offset)) +
						    timed_data_intervals.at(name).first;
						std::array<int8_t, sizeof(ElementType)> local_sample_bytes;
						for (size_t byte = 0; byte < sizeof(ElementType); ++byte) {
							local_sample_bytes.at(sizeof(ElementType) - 1 - byte) =
							    local_data_sample.data.at(total_offset + byte).value();
						}
						local_sample.data.at(neuron) =
						    *reinterpret_cast<ElementType*>(&local_sample_bytes);
					}
					sample_local_neuron_view_offset +=
					    sizeof(ElementType) * neuron_view_shape.columns.size();
				}
				local_neuron_view_offset += sample_local_neuron_view_offset /
				                            (batch_values.size() * local_batch_values.size());
			}
			neuron_view_offset += local_neuron_view_offset;
			data_per_neuron.at(neuron_view) = batch_values;
		}
	};

	auto const extract_observable_per_neuron_packed_active_columns = [&](auto const& type,
	                                                                     auto const& name) {
		auto& data_per_neuron = observable_data.data_per_neuron[name];
		data_per_neuron.resize(m_neuron_view_shapes.size());
		// iterate in reversed order because std::tuple memory
		// layout is last element first
		size_t neuron_view_offset = 0;
		for (int neuron_view = data_per_neuron.size() - 1; neuron_view >= 0; --neuron_view) {
			auto const& neuron_view_shape = m_neuron_view_shapes.at(neuron_view);
			typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
			std::vector<common::TimedDataSequence<std::vector<ElementType>>> batch_values(
			    data.size());
			size_t local_neuron_view_offset = 0;
			for (size_t batch = 0; batch < batch_values.size(); ++batch) {
				auto& local_batch_values = batch_values.at(batch);
				auto const& local_data = data.at(batch);
				local_batch_values.resize(local_data.size());
				size_t sample_local_neuron_view_offset = 0;
				for (size_t sample = 0; sample < local_batch_values.size(); ++sample) {
					auto& local_sample = local_batch_values.at(sample);
					auto& local_data_sample = local_data.at(sample);
					local_sample.time = local_data_sample.time;
					local_sample.data.resize(neuron_view_shape.columns.size());
					for (size_t neuron = 0; neuron < local_sample.data.size(); ++neuron) {
						size_t const total_offset = (sizeof(ElementType) * neuron) +
						                            timed_data_intervals.at(name).first +
						                            neuron_view_offset;
						std::array<int8_t, sizeof(ElementType)> local_sample_bytes;
						for (size_t byte = 0; byte < sizeof(ElementType); ++byte) {
							local_sample_bytes.at(sizeof(ElementType) - 1 - byte) =
							    local_data_sample.data.at(total_offset + byte).value();
						}
						local_sample.data.at(neuron) =
						    *reinterpret_cast<ElementType*>(&local_sample_bytes);
					}
					sample_local_neuron_view_offset +=
					    sizeof(ElementType) * local_sample.data.size();
				}
				local_neuron_view_offset += sample_local_neuron_view_offset /
				                            (batch_values.size() * local_batch_values.size());
			}
			neuron_view_offset += local_neuron_view_offset;
			data_per_neuron.at(neuron_view) = batch_values;
		}
	};

	auto const extract_observable_array = [&](auto const& type, size_t const size,
	                                          auto const& name) {
		auto& data_array = observable_data.data_array[name];
		typedef typename std::decay_t<decltype(type)>::ElementType ElementType;
		std::vector<common::TimedDataSequence<std::vector<ElementType>>> batch_values(data.size());
		for (size_t batch = 0; batch < batch_values.size(); ++batch) {
			auto& local_batch_values = batch_values.at(batch);
			auto const& local_data = data.at(batch);
			local_batch_values.resize(local_data.size());
			for (size_t sample = 0; sample < local_batch_values.size(); ++sample) {
				auto& local_sample = local_batch_values.at(sample);
				auto& local_data_sample = local_data.at(sample);
				local_sample.time = local_data_sample.time;
				local_sample.data.resize(size);
				for (size_t element = 0; element < local_sample.data.size(); ++element) {
					size_t const total_offset =
					    (sizeof(ElementType) * element) + timed_data_intervals.at(name).first;
					std::array<int8_t, sizeof(ElementType)> local_sample_bytes;
					for (size_t byte = 0; byte < sizeof(ElementType); ++byte) {
						local_sample_bytes.at(sizeof(ElementType) - 1 - byte) =
						    local_data_sample.data.at(total_offset + byte).value();
					}
					local_sample.data.at(element) =
					    *reinterpret_cast<ElementType*>(&local_sample_bytes);
				}
			}
		}
		data_array = batch_values;
	};


	for (auto const& [name, obsv] : std::get<TimedRecording>(*m_recording).observables) {
		std::visit(
		    hate::overloaded{
		        [this, &observable_data, &data, &name, &timed_data_intervals,
		         extract_observable_per_synapse_complete_rows,
		         extract_observable_per_synapse_packed_active_columns](
		            TimedRecording::ObservablePerSynapse const& observable) {
			        switch (observable.layout_per_row) {
				        case TimedRecording::ObservablePerSynapse::LayoutPerRow::complete_rows: {
					        std::visit(
					            hate::overloaded{
					                [&](TimedRecording::ObservablePerSynapse::Type::Int8 const&
					                        type) {
						                extract_observable_per_synapse_complete_rows(type, name);
					                },
					                [&](TimedRecording::ObservablePerSynapse::Type::UInt8 const&
					                        type) {
						                extract_observable_per_synapse_complete_rows(type, name);
					                },
					                [&](TimedRecording::ObservablePerSynapse::Type::Int16 const&
					                        type) {
						                extract_observable_per_synapse_complete_rows(type, name);
					                },
					                [&](TimedRecording::ObservablePerSynapse::Type::UInt16 const&
					                        type) {
						                extract_observable_per_synapse_complete_rows(type, name);
					                },
					                [](auto const&) {
						                throw std::logic_error(
						                    "Observable per synapse type not implemented.");
					                }},
					            observable.type);
					        break;
				        }
				        case TimedRecording::ObservablePerSynapse::LayoutPerRow::
				            packed_active_columns: {
					        std::visit(
					            hate::overloaded{
					                [&](TimedRecording::ObservablePerSynapse::Type::Int8 const&
					                        type) {
						                extract_observable_per_synapse_packed_active_columns(
						                    type, name);
					                },
					                [&](TimedRecording::ObservablePerSynapse::Type::UInt8 const&
					                        type) {
						                extract_observable_per_synapse_packed_active_columns(
						                    type, name);
					                },
					                [&](TimedRecording::ObservablePerSynapse::Type::Int16 const&
					                        type) {
						                extract_observable_per_synapse_packed_active_columns(
						                    type, name);
					                },
					                [&](TimedRecording::ObservablePerSynapse::Type::UInt16 const&
					                        type) {
						                extract_observable_per_synapse_packed_active_columns(
						                    type, name);
					                },
					                [](auto const&) {
						                throw std::logic_error(
						                    "Observable per synapse type not implemented.");
					                }},
					            observable.type);
					        break;
				        }
				        default: {
					        throw std::logic_error("Unknown layout per row.");
				        }
			        }
		        },
		        [this, &observable_data, &data, &name, &timed_data_intervals,
		         extract_observable_per_neuron_complete_row,
		         extract_observable_per_neuron_packed_active_columns](
		            TimedRecording::ObservablePerNeuron const& observable) {
			        switch (observable.layout) {
				        case TimedRecording::ObservablePerNeuron::Layout::complete_row: {
					        std::visit(
					            hate::overloaded{
					                [&](TimedRecording::ObservablePerNeuron::Type::Int8 const&
					                        type) {
						                extract_observable_per_neuron_complete_row(type, name);
					                },
					                [&](TimedRecording::ObservablePerNeuron::Type::UInt8 const&
					                        type) {
						                extract_observable_per_neuron_complete_row(type, name);
					                },
					                [&](TimedRecording::ObservablePerNeuron::Type::Int16 const&
					                        type) {
						                extract_observable_per_neuron_complete_row(type, name);
					                },
					                [&](TimedRecording::ObservablePerNeuron::Type::UInt16 const&
					                        type) {
						                extract_observable_per_neuron_complete_row(type, name);
					                },
					                [](auto const&) {
						                throw std::logic_error(
						                    "Observable per neuron type not implemented.");
					                }},
					            observable.type);
					        break;
				        }
				        case TimedRecording::ObservablePerNeuron::Layout::packed_active_columns: {
					        std::visit(
					            hate::overloaded{
					                [&](TimedRecording::ObservablePerNeuron::Type::Int8 const&
					                        type) {
						                extract_observable_per_neuron_packed_active_columns(
						                    type, name);
					                },
					                [&](TimedRecording::ObservablePerNeuron::Type::UInt8 const&
					                        type) {
						                extract_observable_per_neuron_packed_active_columns(
						                    type, name);
					                },
					                [&](TimedRecording::ObservablePerNeuron::Type::Int16 const&
					                        type) {
						                extract_observable_per_neuron_packed_active_columns(
						                    type, name);
					                },
					                [&](TimedRecording::ObservablePerNeuron::Type::UInt16 const&
					                        type) {
						                extract_observable_per_neuron_packed_active_columns(
						                    type, name);
					                },
					                [](auto const&) {
						                throw std::logic_error(
						                    "Observable per neuron type not implemented.");
					                }},
					            observable.type);
					        break;
				        }
				        default: {
					        throw std::logic_error("Unknown layout.");
				        }
			        }
		        },
		        [this, &observable_data, &data, &name, &timed_data_intervals,
		         extract_observable_array](TimedRecording::ObservableArray const& observable) {
			        std::visit(
			            hate::overloaded{
			                [&](TimedRecording::ObservableArray::Type::Int8 const& type) {
				                extract_observable_array(type, observable.size, name);
			                },
			                [&](TimedRecording::ObservableArray::Type::UInt8 const& type) {
				                extract_observable_array(type, observable.size, name);
			                },
			                [&](TimedRecording::ObservableArray::Type::Int16 const& type) {
				                extract_observable_array(type, observable.size, name);
			                },
			                [&](TimedRecording::ObservableArray::Type::UInt16 const& type) {
				                extract_observable_array(type, observable.size, name);
			                },
			                [](auto const&) {
				                throw std::logic_error("Observable array type not implemented.");
			                }},
			            observable.type);
		        },
		        [this](auto const&) {
			        throw std::logic_error("Observable type not implemented.");
		        }},
		    obsv);
	}

	LOG4CXX_TRACE(
	    log4cxx::Logger::getLogger("grenade.signal_flow.vertex.PlasticityRule"),
	    "extract_recording_data(): " << observable_data);
	LOG4CXX_TRACE(
	    log4cxx::Logger::getLogger("grenade.signal_flow.vertex.PlasticityRule"),
	    "extract_recording_data(): Extracted recording in " << timer.print());
	return observable_data;
}

std::vector<PlasticityRule::SynapseViewShape> const& PlasticityRule::get_synapse_view_shapes() const
{
	return m_synapse_view_shapes;
}

std::vector<PlasticityRule::NeuronViewShape> const& PlasticityRule::get_neuron_view_shapes() const
{
	return m_neuron_view_shapes;
}

std::optional<PlasticityRule::Recording> PlasticityRule::get_recording() const
{
	return m_recording;
}

PlasticityRule::ID const& PlasticityRule::get_id() const
{
	return m_id;
}

bool PlasticityRule::valid_output_port_data(
    size_t output_port_on_vertex, grenade::common::PortData const& data) const
{
	return m_recording && output_port_on_vertex == 0 &&
	       dynamic_cast<Results const*>(&data) != nullptr;
}

bool PlasticityRule::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	return (input_port_on_vertex == (m_synapse_view_shapes.size() + m_neuron_view_shapes.size()) &&
	        dynamic_cast<Parameterization const*>(&data) != nullptr) ||
	       (input_port_on_vertex ==
	            (m_synapse_view_shapes.size() + m_neuron_view_shapes.size() + 1) &&
	        dynamic_cast<Dynamics const*>(&data) != nullptr);
}

std::vector<grenade::common::Vertex::Port> PlasticityRule::get_input_ports() const
{
	std::vector<Port> ret;
	for (auto const& shape : m_synapse_view_shapes) {
		ret.push_back(grenade::common::Vertex::Port(
		    VertexPortType(ConnectionType::SynapticInput),
		    grenade::common::Vertex::Port::SumOrSplitSupport::no,
		    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
		    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
		    grenade::common::CuboidMultiIndexSequence({shape.columns.size()})));
	}
	for (auto const& shape : m_neuron_view_shapes) {
		ret.push_back(grenade::common::Vertex::Port(
		    VertexPortType(ConnectionType::MembraneVoltage),
		    grenade::common::Vertex::Port::SumOrSplitSupport::no,
		    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
		    grenade::common::Vertex::Port::RequiresOrGeneratesData::no,
		    grenade::common::CuboidMultiIndexSequence({shape.columns.size()})));
	}
	ret.push_back(grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})})));
	ret.push_back(grenade::common::Vertex::Port(
	    DynamicsPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})})));
	return ret;
}

std::vector<grenade::common::Vertex::Port> PlasticityRule::get_output_ports() const
{
	if (!m_recording) {
		return {};
	}
	return {grenade::common::Vertex::Port(
	    ResultsPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::unique_ptr<grenade::common::Vertex> PlasticityRule::copy() const
{
	return std::make_unique<PlasticityRule>(*this);
}

std::unique_ptr<grenade::common::Vertex> PlasticityRule::move()
{
	return std::make_unique<PlasticityRule>(std::move(*this));
}

std::ostream& PlasticityRule::print(std::ostream& os) const
{
	os << "PlasticityRule(\n";
	if (m_recording) {
		os << "\nrecording:\n";
		std::visit([&](auto const& recording) { os << recording << "\n"; }, *m_recording);
	}
	os << "\n" << m_id << "\n";
	os << ")";
	return os;
}

bool PlasticityRule::valid_edge_from(Vertex const& source, grenade::common::Edge const& edge) const
{
	if (!PartitionedVertex::valid_edge_from(source, edge)) {
		return false;
	}
	// to ensure correct mapping we only support a single complete edge to the plasticity rule for
	// each network entity it acts on
	if (!edge.get_channels_on_target().is_injective() ||
	    edge.get_channels_on_target().size() !=
	        get_input_ports().at(edge.port_on_target).get_channels().size()) {
		return false;
	}
	return true;
}

bool PlasticityRule::is_equal_to(Vertex const& other) const
{
	auto const& other_plasticity = static_cast<PlasticityRule const&>(other);
	return (m_synapse_view_shapes == other_plasticity.m_synapse_view_shapes) &&
	       (m_recording == other_plasticity.m_recording) && (m_id == other_plasticity.m_id) &&
	       EntityOnChip::is_equal_to(other);
}


std::ostream& operator<<(
    std::ostream& os,
    PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow const& layout)
{
	switch (layout) {
		case PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::complete_rows: {
			return (os << "complete_rows");
		}
		case PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::
		    packed_active_columns: {
			return (os << "packed_active_columns");
		}
		default: {
			throw std::logic_error("Unknown layout per row.");
		}
	}
}


std::ostream& operator<<(
    std::ostream& os, PlasticityRule::TimedRecording::ObservablePerNeuron::Layout const& layout)
{
	switch (layout) {
		case PlasticityRule::TimedRecording::ObservablePerNeuron::Layout::complete_row: {
			return (os << "complete_row");
		}
		case PlasticityRule::TimedRecording::ObservablePerNeuron::Layout::packed_active_columns: {
			return (os << "packed_active_columns");
		}
		default: {
			throw std::logic_error("Unknown layout.");
		}
	}
}

} // namespace grenade::vx::signal_flow::vertex
