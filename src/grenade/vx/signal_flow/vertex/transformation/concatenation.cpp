#include "grenade/vx/signal_flow/vertex/transformation/concatenation.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include "hate/join.h"
#include <functional>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

Concatenation::Concatenation(ConnectionType const type, std::vector<size_t> const& sizes) :
    m_type(type), m_sizes(sizes)
{
}

std::vector<Transformation::Function::Port> Concatenation::get_input_ports() const
{
	std::vector<Port> ports;
	for (auto const size : m_sizes) {
		ports.push_back(
		    Port(VertexPortType(m_type), grenade::common::CuboidMultiIndexSequence({size})));
	}
	return ports;
}

std::vector<Transformation::Function::Port> Concatenation::get_output_ports() const
{
	size_t acc = 0;
	for (auto const size : m_sizes) {
		acc += size;
	}
	return {Port(VertexPortType(m_type), grenade::common::CuboidMultiIndexSequence({acc}))};
}

std::vector<Transformation::Results> Concatenation::apply(
    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data) const
{
	size_t batch_size = 0;
	if (!data.empty()) {
		batch_size = std::visit(
		    [](auto const& v) { return v.size(); },
		    dynamic_cast<Transformation::Dynamics const&>(data.at(0).get()).value);
	}
	auto const output_size = static_cast<grenade::common::CuboidMultiIndexSequence const&>(
	                             get_output_ports().at(0).get_channels())
	                             .get_shape()
	                             .at(0);

	auto const copy = [&](auto& ret) {
		for (size_t i = 0; i < ret.size(); ++i) {
			size_t o_offset = 0;
			for (auto const& svalue : data) {
				auto const& v = std::get<std::remove_reference_t<decltype(ret)>>(
				    dynamic_cast<Transformation::Dynamics const&>(svalue.get()).value);
				ret.at(i).resize(v.at(i).size());
				for (size_t j = 0; j < v.at(i).size(); ++j) {
					ret.at(i).at(j).data.resize(output_size);
					// TODO: think about what to do with event time information, whether to
					// check for equality or just drop, etc.
					ret.at(i).at(j).time = v.at(i).at(j).time;
					for (size_t k = 0; k < v.at(i).at(j).data.size(); ++k) {
						ret.at(i).at(j).data.at(k + o_offset) = v.at(i).at(j).data.at(k);
						ret.at(i).at(j).data.at(k + o_offset) = v.at(i).at(j).data.at(k);
					}
				}
				if (!v.at(i).empty()) {
					o_offset += v.at(i).at(0).data.size();
				}
			}
		}
		return ret;
	};

	switch (m_type) {
		case ConnectionType::UInt32: {
			std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>> output(
			    batch_size);
			return {{copy(output)}};
		}
		case ConnectionType::Int8: {
			std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>> output(
			    batch_size);
			return {{copy(output)}};
		}
		case ConnectionType::UInt5: {
			std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>> output(
			    batch_size);
			return {{copy(output)}};
		}
		default: {
			throw std::logic_error("Concatenation for given data type not implemented.");
		}
	};
}

std::unique_ptr<Transformation::Function> Concatenation::copy() const
{
	return std::make_unique<Concatenation>(*this);
}

std::unique_ptr<Transformation::Function> Concatenation::move()
{
	return std::make_unique<Concatenation>(*this);
}

bool Concatenation::is_equal_to(Transformation::Function const& other) const
{
	auto const& o = static_cast<Concatenation const&>(other);
	return (m_type == o.m_type) && (m_sizes == o.m_sizes);
}

std::ostream& Concatenation::print(std::ostream& os) const
{
	return os << "Concatenation(type: " << m_type << ", sizes: [" << hate::join(m_sizes, ", ")
	          << "])";
}

} // namespace grenade::vx::signal_flow::vertex::transformation
