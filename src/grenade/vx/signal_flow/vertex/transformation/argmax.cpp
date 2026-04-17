#include "grenade/vx/signal_flow/vertex/transformation/argmax.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <functional>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

ArgMax::ArgMax(size_t const size, ConnectionType const type)
{
	switch (type) {
		case ConnectionType::UInt32: {
			m_type = type;
			break;
		}
		case ConnectionType::UInt5: {
			m_type = type;
			break;
		}
		case ConnectionType::Int8: {
			m_type = type;
			break;
		}
		default: {
			std::stringstream ss;
			ss << "Specified ConnectionType(" << type << ") to ArgMax not supported.";
			throw std::runtime_error(ss.str());
		}
	}
	if (size > std::numeric_limits<uint32_t>::max()) {
		throw std::runtime_error(
		    "Specified size(" + std::to_string(size) + ") to ArgMax not representable.");
	}
	m_size = size;
}

std::vector<Transformation::Function::Port> ArgMax::get_input_ports() const
{
	return {Port(VertexPortType(m_type), grenade::common::CuboidMultiIndexSequence({m_size}))};
}

std::vector<Transformation::Function::Port> ArgMax::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::UInt32), grenade::common::CuboidMultiIndexSequence({1}))};
}

std::vector<Transformation::Results> ArgMax::apply(
    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data) const
{
	auto const compute = [&](auto const& local_data) {
		// check size match only for first because we know that the data map is valid
		assert(
		    !local_data.size() ||
		    (local_data.front().size() && (m_size == local_data.front().at(0).data.size())));
		std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>> tmps(
		    local_data.size());
		for (auto& t : tmps) {
			// TODO: Think about what shall happen with timing info and when multiple events are
			// present
			t.resize(1);
			t.at(0).data.resize(1 /* data.output().size */);
		}
		size_t i = 0;
		for (auto const& entry : local_data) {
			tmps.at(i).at(0).data.at(0) = signal_flow::UInt32(std::distance(
			    entry.at(0).data.begin(),
			    std::max_element(entry.at(0).data.begin(), entry.at(0).data.end())));
			i++;
		}
		return tmps;
	};

	auto const visitor = [&](auto const& d) -> Transformation::Results::Value {
		typedef std::remove_cvref_t<decltype(d)> Data;
		typedef hate::type_list<
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt32>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::UInt5>>>,
		    std::vector<common::TimedDataSequence<std::vector<signal_flow::Int8>>>>
		    MatrixData;
		if constexpr (hate::is_in_type_list<Data, MatrixData>::value) {
			return compute(d);
		} else {
			throw std::logic_error("ArgMax data type not implemented.");
		}
	};
	return {
	    std::visit(visitor, dynamic_cast<Transformation::Dynamics const&>(data.at(0).get()).value)};
}

std::unique_ptr<Transformation::Function> ArgMax::copy() const
{
	return std::make_unique<ArgMax>(*this);
}

std::unique_ptr<Transformation::Function> ArgMax::move()
{
	return std::make_unique<ArgMax>(*this);
}

bool ArgMax::is_equal_to(Transformation::Function const& other) const
{
	auto const& o = static_cast<ArgMax const&>(other);
	return (m_size == o.m_size) && (m_type == o.m_type);
}

std::ostream& ArgMax::print(std::ostream& os) const
{
	return os << "ArgMax(size: " << m_size << ", type: " << m_type << ")";
}

} // namespace grenade::vx::signal_flow::vertex::transformation
