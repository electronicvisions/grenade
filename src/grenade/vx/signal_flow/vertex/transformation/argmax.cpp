#include "grenade/vx/signal_flow/vertex/transformation/argmax.h"

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

ArgMax::~ArgMax() {}

std::vector<Port> ArgMax::inputs() const
{
	return {Port(m_size, m_type)};
}

Port ArgMax::output() const
{
	return Port(1, ConnectionType::UInt32);
}

ArgMax::Function::Value ArgMax::apply(std::vector<Function::Value> const& value) const
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

	auto const visitor = [&](auto const& d) -> Function::Value {
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
	return std::visit(visitor, value.at(0));
}

bool ArgMax::equal(Transformation::Function const& other) const
{
	if (auto const o = dynamic_cast<ArgMax const*>(&other); o != nullptr) {
		return (m_size == o->m_size) && (m_type == o->m_type);
	}
	return false;
}

} // namespace grenade::vx::signal_flow::vertex::transformation
