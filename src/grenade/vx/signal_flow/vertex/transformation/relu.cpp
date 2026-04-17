#include "grenade/vx/signal_flow/vertex/transformation/relu.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"

#include <functional>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

ReLU::ReLU(size_t const size) : m_size(size) {}

std::vector<Transformation::Function::Port> ReLU::get_input_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::Int8), grenade::common::CuboidMultiIndexSequence({m_size}))};
}

std::vector<Transformation::Function::Port> ReLU::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::Int8), grenade::common::CuboidMultiIndexSequence({m_size}))};
}

std::vector<Transformation::Results> ReLU::apply(
    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data) const
{
	size_t batch_size = 0;
	batch_size = std::visit(
	    [](auto const& v) { return v.size(); },
	    dynamic_cast<Transformation::Dynamics const&>(data.at(0).get()).value);

	std::vector<common::TimedDataSequence<std::vector<Int8>>> ret(batch_size);
	for (auto& e : ret) {
		// TODO: Think about what shall happen with timing info and when multiple events are present
		e.resize(1);
		e.at(0).data.resize(m_size);
	}
	for (size_t j = 0; j < ret.size(); ++j) {
		auto const& d = std::get<std::vector<common::TimedDataSequence<std::vector<Int8>>>>(
		                    dynamic_cast<Transformation::Dynamics const&>(data.at(0).get()).value)
		                    .at(j);
		assert(d.size() == 1);
		for (size_t i = 0; i < m_size; ++i) {
			ret.at(j).at(0).data.at(i) = std::max(d.at(0).data.at(i), Int8(0));
		}
	}
	return {{ret}};
}

bool ReLU::is_equal_to(Transformation::Function const& other) const
{
	auto const& o = static_cast<ReLU const&>(other);
	return (m_size == o.m_size);
}

std::unique_ptr<Transformation::Function> ReLU::copy() const
{
	return std::make_unique<ReLU>(*this);
}

std::unique_ptr<Transformation::Function> ReLU::move()
{
	return std::make_unique<ReLU>(std::move(*this));
}

std::ostream& ReLU::print(std::ostream& os) const
{
	return os << "ReLU(size: " << m_size << ")";
}

} // namespace grenade::vx::signal_flow::vertex::transformation
