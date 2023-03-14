#include "grenade/vx/signal_flow/vertex/plasticity_rule.h"

#include "grenade/cerealization.h"
#include "grenade/vx/signal_flow/port_restriction.h"
#include "grenade/vx/signal_flow/types.h"
#include "grenade/vx/signal_flow/vertex/synapse_array_view.h"
#include "halco/common/cerealization_geometry.h"
#include "halco/common/cerealization_typed_array.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include "halco/hicann-dls/vx/v3/synapse.h"
#include "hate/math.h"
#include "hate/variant.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <cereal/types/map.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::signal_flow::vertex {

bool PlasticityRule::Timer::operator==(Timer const& other) const
{
	return (start == other.start) && (period == other.period) && (num_periods == other.num_periods);
}

bool PlasticityRule::Timer::operator!=(Timer const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void PlasticityRule::Timer::serialize(Archive& ar, std::uint32_t const)
{
	ar(start);
	ar(period);
	ar(num_periods);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::Timer const& timer)
{
	os << "Timer(start: " << timer.start << ", period: " << timer.period
	   << ", num_periods: " << timer.num_periods << ")";
	return os;
}

bool PlasticityRule::RawRecording::operator==(RawRecording const& other) const
{
	return scratchpad_memory_size == other.scratchpad_memory_size;
}

bool PlasticityRule::RawRecording::operator!=(RawRecording const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void PlasticityRule::RawRecording::serialize(Archive& ar, std::uint32_t const)
{
	ar(scratchpad_memory_size);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::RawRecording const& recording)
{
	os << "RawRecording(scratchpad_memory_size: " << recording.scratchpad_memory_size << ")";
	return os;
}

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

template <typename Archive>
void PlasticityRule::TimedRecording::ObservablePerSynapse::serialize(
    Archive& ar, std::uint32_t const)
{
	ar(type);
	ar(layout_per_row);
}

std::ostream& operator<<(
    std::ostream& os, PlasticityRule::TimedRecording::ObservablePerSynapse const& observable)
{
	os << "ObservablePerSynapse(type: ";
	std::visit([&os](auto const& type) { os << type; }, observable.type);
	os << ", layout_per_row: " << observable.layout_per_row << ")";
	return os;
}

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

template <typename Archive>
void PlasticityRule::TimedRecording::ObservablePerNeuron::serialize(
    Archive& ar, std::uint32_t const)
{
	ar(type);
	ar(layout);
}

std::ostream& operator<<(
    std::ostream& os, PlasticityRule::TimedRecording::ObservablePerNeuron const& observable)
{
	os << "ObservablePerNeuron(type: ";
	std::visit([&os](auto const& type) { os << type; }, observable.type);
	os << ", layout: " << observable.layout << ")";
	return os;
}

bool PlasticityRule::TimedRecording::ObservableArray::operator==(ObservableArray const& other) const
{
	return (type == other.type) && (size == other.size);
}

bool PlasticityRule::TimedRecording::ObservableArray::operator!=(ObservableArray const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void PlasticityRule::TimedRecording::ObservableArray::serialize(Archive& ar, std::uint32_t const)
{
	ar(type);
	ar(size);
}

std::ostream& operator<<(
    std::ostream& os, PlasticityRule::TimedRecording::ObservableArray const& observable)
{
	os << "ObservableArray(type: ";
	std::visit([&os](auto const& type) { os << type; }, observable.type);
	os << ", size: " << observable.size << ")";
	return os;
}

bool PlasticityRule::TimedRecording::operator==(TimedRecording const& other) const
{
	return observables == other.observables;
}

bool PlasticityRule::TimedRecording::operator!=(TimedRecording const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void PlasticityRule::TimedRecording::serialize(Archive& ar, std::uint32_t const)
{
	ar(observables);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::TimedRecording const& recording)
{
	os << "TimedRecording(observables:\n";
	for (auto const& [name, type] : recording.observables) {
		os << "\t" << name << ": ";
		std::visit([&](auto const& t) { os << t; }, type);
		os << "\n";
	}
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

template <typename Archive>
void PlasticityRule::SynapseViewShape::serialize(Archive& ar, std::uint32_t const)
{
	ar(num_rows);
	ar(columns.size());
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::SynapseViewShape const& shape)
{
	os << "SynapseViewShape(num_rows: " << shape.num_rows
	   << ", columns.size(): " << shape.columns.size() << ")";
	return os;
}

bool PlasticityRule::NeuronViewShape::operator==(NeuronViewShape const& other) const
{
	return (columns.size() == other.columns.size()) && row == other.row &&
	       neuron_readout_sources == other.neuron_readout_sources;
}

bool PlasticityRule::NeuronViewShape::operator!=(NeuronViewShape const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void PlasticityRule::NeuronViewShape::serialize(Archive& ar, std::uint32_t const)
{
	ar(columns);
	ar(row);
	ar(neuron_readout_sources);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule::NeuronViewShape const& shape)
{
	os << "NeuronViewShape(\n";
	os << "\tcolumns:\n";
	for (auto const& column : shape.columns) {
		os << "\t\t" << column << "\n";
	}
	os << "\tneuron_readout_sources:\n";
	for (auto const& readout_source : shape.neuron_readout_sources) {
		os << "\t\t";
		if (readout_source) {
			os << *readout_source;
		} else {
			os << "unset";
		}
		os << "\n";
	}
	os << "\trow: " << shape.row << "\n";
	os << ")";
	return os;
}

PlasticityRule::PlasticityRule(
    std::string kernel,
    Timer const& timer,
    std::vector<SynapseViewShape> const& synapse_view_shapes,
    std::vector<NeuronViewShape> const& neuron_view_shapes,
    std::optional<Recording> const& recording) :
    m_kernel(std::move(kernel)),
    m_timer(timer),
    m_synapse_view_shapes(synapse_view_shapes),
    m_neuron_view_shapes(neuron_view_shapes),
    m_recording(recording)
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

std::string PlasticityRule::get_recorded_memory_definition() const
{
	if (!m_recording) {
		return "";
	}
	if (std::holds_alternative<RawRecording>(*m_recording)) {
		std::stringstream ss;
		ss << "#include <array>\n";
		ss << "\n";
		ss << "struct Recording\n";
		ss << "{\n";
		ss << "\tstd::array<char, " << std::get<RawRecording>(*m_recording).scratchpad_memory_size
		   << "> memory;\n";
		ss << "};";
		return ss.str();
	} else if (std::holds_alternative<TimedRecording>(*m_recording)) {
		auto const print_observable_array_type = [](TimedRecording::ObservableArray const&
		                                                observable) {
			std::stringstream ss;
			ss << "std::array<";
			std::visit(
			    hate::overloaded{
			        [&ss](TimedRecording::ObservableArray::Type::Int8 const&) { ss << "int8_t"; },
			        [&ss](TimedRecording::ObservableArray::Type::UInt8 const&) { ss << "uint8_t"; },
			        [&ss](TimedRecording::ObservableArray::Type::Int16 const&) { ss << "int16_t"; },
			        [&ss](TimedRecording::ObservableArray::Type::UInt16 const&) {
				        ss << "uint16_t";
			        },
			        [](auto const&) { throw std::logic_error("Observable array type unknown."); }},
			    observable.type);
			ss << ", " << observable.size << ">";
			return ss.str();
		};

		auto const print_observable_per_synapse_type =
		    [](TimedRecording::ObservablePerSynapse const& observable,
		       SynapseViewShape synapse_view_shape) {
			    std::stringstream ss;
			    ss << "std::array<";
			    switch (observable.layout_per_row) {
				    case TimedRecording::ObservablePerSynapse::LayoutPerRow::complete_rows: {
					    ss << "libnux::vx::VectorRow";
					    std::visit(
					        hate::overloaded{
					            [&ss](TimedRecording::ObservablePerSynapse::Type::Int8 const&) {
						            ss << "FracSat8";
					            },
					            [&ss](TimedRecording::ObservablePerSynapse::Type::UInt8 const&) {
						            ss << "Mod8";
					            },
					            [&ss](TimedRecording::ObservablePerSynapse::Type::Int16 const&) {
						            ss << "FracSat16";
					            },
					            [&ss](TimedRecording::ObservablePerSynapse::Type::UInt16 const&) {
						            ss << "Mod16";
					            },
					            [](auto const&) {
						            throw std::logic_error("Observable array type unknown.");
					            }},
					        observable.type);
					    break;
				    }
				    case TimedRecording::ObservablePerSynapse::LayoutPerRow::
				        packed_active_columns: {
					    ss << "std::array<";
					    std::visit(
					        hate::overloaded{
					            [&ss](TimedRecording::ObservablePerSynapse::Type::Int8 const&) {
						            ss << "int8_t";
					            },
					            [&ss](TimedRecording::ObservablePerSynapse::Type::UInt8 const&) {
						            ss << "uint8_t";
					            },
					            [&ss](TimedRecording::ObservablePerSynapse::Type::Int16 const&) {
						            ss << "int16_t";
					            },
					            [&ss](TimedRecording::ObservablePerSynapse::Type::UInt16 const&) {
						            ss << "uint16_t";
					            },
					            [](auto const&) {
						            throw std::logic_error("Observable array type unknown.");
					            }},
					        observable.type);
					    ss << ", " << synapse_view_shape.columns.size() << ">";
					    break;
				    }
				    default: {
					    throw std::logic_error("Unknown layout per row.");
				    }
			    }
			    ss << ", " << synapse_view_shape.num_rows << ">";
			    return ss.str();
		    };

		auto const print_observable_per_neuron_type =
		    [](TimedRecording::ObservablePerNeuron const& observable,
		       NeuronViewShape neuron_view_shape) {
			    std::stringstream ss;
			    switch (observable.layout) {
				    case TimedRecording::ObservablePerNeuron::Layout::complete_row: {
					    ss << "libnux::vx::VectorRow";
					    std::visit(
					        hate::overloaded{
					            [&ss](TimedRecording::ObservablePerNeuron::Type::Int8 const&) {
						            ss << "FracSat8";
					            },
					            [&ss](TimedRecording::ObservablePerNeuron::Type::UInt8 const&) {
						            ss << "Mod8";
					            },
					            [&ss](TimedRecording::ObservablePerNeuron::Type::Int16 const&) {
						            ss << "FracSat16";
					            },
					            [&ss](TimedRecording::ObservablePerNeuron::Type::UInt16 const&) {
						            ss << "Mod16";
					            },
					            [](auto const&) {
						            throw std::logic_error("Observable array type unknown.");
					            }},
					        observable.type);
					    break;
				    }
				    case TimedRecording::ObservablePerNeuron::Layout::packed_active_columns: {
					    ss << "std::array<";
					    std::visit(
					        hate::overloaded{
					            [&ss](TimedRecording::ObservablePerNeuron::Type::Int8 const&) {
						            ss << "int8_t";
					            },
					            [&ss](TimedRecording::ObservablePerNeuron::Type::UInt8 const&) {
						            ss << "uint8_t";
					            },
					            [&ss](TimedRecording::ObservablePerNeuron::Type::Int16 const&) {
						            ss << "int16_t";
					            },
					            [&ss](TimedRecording::ObservablePerNeuron::Type::UInt16 const&) {
						            ss << "uint16_t";
					            },
					            [](auto const&) {
						            throw std::logic_error("Observable array type unknown.");
					            }},
					        observable.type);
					    ss << ", " << neuron_view_shape.columns.size() << ">";
					    break;
				    }
				    default: {
					    throw std::logic_error("Unknown layout per row.");
				    }
			    }
			    return ss.str();
		    };

		std::stringstream ss;
		ss << "#include <array>\n";
		ss << "#include <tuple>\n";
		ss << "#include \"libnux/vx/vector_row.h\"\n";
		ss << "\n";
		ss << "struct Recording\n";
		ss << "{\n";
		ss << "\tuint64_t time;\n";
		for (auto const& [name, type] : std::get<TimedRecording>(*m_recording).observables) {
			ss << "\t";
			std::visit(
			    hate::overloaded{
			        [&ss, print_observable_array_type](
			            TimedRecording::ObservableArray const& observable) {
				        ss << print_observable_array_type(observable);
			        },
			        [&](TimedRecording::ObservablePerSynapse const& observable) {
				        ss << "std::tuple<";
				        std::vector<std::string> synapse_view_entries;
				        for (auto const& synapse_view_shape : m_synapse_view_shapes) {
					        synapse_view_entries.push_back(
					            print_observable_per_synapse_type(observable, synapse_view_shape));
				        }
				        ss << hate::join_string(
				            synapse_view_entries.begin(), synapse_view_entries.end(), ", ");
				        ss << ">";
			        },
			        [&](TimedRecording::ObservablePerNeuron const& observable) {
				        ss << "std::tuple<";
				        std::vector<std::string> neuron_view_entries;
				        for (auto const& neuron_view_shape : m_neuron_view_shapes) {
					        neuron_view_entries.push_back(
					            print_observable_per_neuron_type(observable, neuron_view_shape));
				        }
				        ss << hate::join_string(
				            neuron_view_entries.begin(), neuron_view_entries.end(), ", ");
				        ss << ">";
			        },
			        [](auto const&) {
				        throw std::logic_error("Observable type not implemented.");
			        }},
			    type);
			ss << " " << name << ";\n";
		}
		ss << "};\n";
		ss << "namespace {\n";
		ss << "template <size_t RealSize>\n";
		ss << "void static_assert_size() {\n";
		ss << "\tstatic_assert(RealSize == " << get_recorded_scratchpad_memory_size()
		   << ", \"Recording size doesn't match expectation.\");\n";
		ss << "}\n";
		ss << "[[maybe_unused]] void static_assert_size_eval() {\n";
		ss << "static_assert_size<sizeof(Recording)>();\n";
		ss << "}\n";
		ss << "}\n";
		return ss.str();
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
	if (!m_recording) {
		throw std::runtime_error("Observable extraction only possible when recording is present.");
	}

	for (auto const& batch : data) {
		for (auto const& sample : batch) {
			if (sample.data.size() != output().size) {
				throw std::runtime_error(
				    "Size of given data (" + std::to_string(sample.data.size()) +
				    ") from which to extract recorded "
				    "data does not match expectation (" +
				    std::to_string(output().size) + ").");
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
						size_t const ppu_offset = synapse_view_shape.hemisphere.value()
						                              ? local_data_sample.data.size() / 2
						                              : 0;
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
						    timed_data_intervals.at(name).first + ppu_offset;
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
					size_t const ppu_offset = synapse_view_shape.hemisphere.value()
					                              ? local_data_sample.data.size() / 2
					                              : 0;
					local_sample.time = local_data_sample.time;
					local_sample.data.resize(
					    synapse_view_shape.num_rows * synapse_view_shape.columns.size());
					for (size_t synapse = 0; synapse < local_sample.data.size(); ++synapse) {
						size_t const total_offset = (sizeof(ElementType) * synapse) +
						                            timed_data_intervals.at(name).first +
						                            ppu_offset + synapse_view_offset;
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
						size_t const ppu_offset =
						    neuron_view_shape.row.value() ? local_data_sample.data.size() / 2 : 0;
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
						    timed_data_intervals.at(name).first + ppu_offset;
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
					size_t const ppu_offset =
					    neuron_view_shape.row.value() ? local_data_sample.data.size() / 2 : 0;
					local_sample.time = local_data_sample.time;
					local_sample.data.resize(neuron_view_shape.columns.size());
					for (size_t neuron = 0; neuron < local_sample.data.size(); ++neuron) {
						size_t const total_offset = (sizeof(ElementType) * neuron) +
						                            timed_data_intervals.at(name).first +
						                            ppu_offset + neuron_view_offset;
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
				local_sample.data.resize(size * 2);
				for (auto const ppu :
				     halco::common::iter_all<halco::hicann_dls::vx::v3::PPUOnDLS>()) {
					for (size_t element = 0; element < local_sample.data.size() / 2; ++element) {
						auto const local_ppu_offset =
						    (ppu == halco::hicann_dls::vx::v3::PPUOnDLS::bottom)
						        ? local_sample.data.size() / 2
						        : 0;
						auto const ppu_offset = (ppu == halco::hicann_dls::vx::v3::PPUOnDLS::bottom)
						                            ? local_data_sample.data.size() / 2
						                            : 0;
						size_t const total_offset = (sizeof(ElementType) * element) +
						                            timed_data_intervals.at(name).first +
						                            ppu_offset;
						std::array<int8_t, sizeof(ElementType)> local_sample_bytes;
						for (size_t byte = 0; byte < sizeof(ElementType); ++byte) {
							local_sample_bytes.at(sizeof(ElementType) - 1 - byte) =
							    local_data_sample.data.at(total_offset + byte).value();
						}
						local_sample.data.at(element + local_ppu_offset) =
						    *reinterpret_cast<ElementType*>(&local_sample_bytes);
					}
				}
			}
		}
		data_array = batch_values;
	};


	for (auto const& [name, obsv] : std::get<TimedRecording>(*m_recording).observables) {
		std::visit(
		    hate::overloaded{
		        [this, &observable_data, data, name, &timed_data_intervals,
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
		        [this, &observable_data, data, name, &timed_data_intervals,
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
		        [this, &observable_data, data, name, &timed_data_intervals,
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

	return observable_data;
}

std::string const& PlasticityRule::get_kernel() const
{
	return m_kernel;
}

PlasticityRule::Timer const& PlasticityRule::get_timer() const
{
	return m_timer;
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

std::vector<Port> PlasticityRule::inputs() const
{
	std::vector<Port> ret;
	for (auto const& shape : m_synapse_view_shapes) {
		ret.push_back(Port(shape.columns.size(), ConnectionType::SynapticInput));
	}
	for (auto const& shape : m_neuron_view_shapes) {
		ret.push_back(Port(shape.columns.size(), ConnectionType::MembraneVoltage));
	}
	return ret;
}

Port PlasticityRule::output() const
{
	return Port(
	    (get_recorded_memory_data_interval().second - get_recorded_memory_data_interval().first) *
	        2 /* two PPUs */,
	    ConnectionType::Int8);
}

std::ostream& operator<<(std::ostream& os, PlasticityRule const& config)
{
	os << "PlasticityRule(\nkernel:\n" << config.m_kernel << "\ntimer:\n" << config.m_timer;
	if (config.m_recording) {
		os << "\nrecording:\n";
		std::visit([&](auto const& recording) { os << recording << "\n"; }, *config.m_recording);
	}
	os << ")";
	return os;
}

bool PlasticityRule::supports_input_from(
    SynapseArrayView const& /*input*/, std::optional<PortRestriction> const& restriction) const
{
	return !restriction;
}

bool PlasticityRule::supports_input_from(
    NeuronView const& /*input*/, std::optional<PortRestriction> const& restriction) const
{
	return !restriction;
}

bool PlasticityRule::operator==(PlasticityRule const& other) const
{
	return (m_kernel == other.m_kernel) && (m_timer == other.m_timer) &&
	       (m_synapse_view_shapes == other.m_synapse_view_shapes) &&
	       (m_recording == other.m_recording);
}

bool PlasticityRule::operator!=(PlasticityRule const& other) const
{
	return !(*this == other);
}

template <typename Archive>
void PlasticityRule::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_kernel);
	ar(m_timer);
	ar(m_synapse_view_shapes);
	ar(m_neuron_view_shapes);
	ar(m_recording);
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

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::PlasticityRule::Timer)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::vertex::PlasticityRule)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::PlasticityRule::Timer, 0)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::vertex::PlasticityRule, 3)
