#include "grenade/vx/signal_flow/vertex/transformation/subtraction.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"

#include <functional>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

Subtraction::Subtraction(size_t const num_inputs, size_t const size) :
    m_num_inputs(num_inputs), m_size(size)
{}

std::vector<Transformation::Function::Port> Subtraction::get_input_ports() const
{
	return std::vector{
	    m_num_inputs, Port(
	                      VertexPortType(ConnectionType::Int8),
	                      grenade::common::CuboidMultiIndexSequence({m_size}))};
}

std::vector<Transformation::Function::Port> Subtraction::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::Int8), grenade::common::CuboidMultiIndexSequence({m_size}))};
}

std::vector<Transformation::Results> Subtraction::apply(
    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& data) const
{
	size_t batch_size = 0;
	if (!data.empty()) {
		batch_size = std::visit(
		    [](auto const& v) { return v.size(); },
		    dynamic_cast<Transformation::Dynamics const&>(data.at(0).get()).value);
	}

	std::vector<common::TimedDataSequence<std::vector<Int8>>> ret(batch_size);
	for (auto& e : ret) {
		// TODO: Think about what shall happen with timing info and when multiple events are present
		e.resize(1);
		e.at(0).data.resize(m_size);
	}

	std::vector<intmax_t> tmps(m_size, 0);
	for (size_t j = 0; j < batch_size; ++j) {
		// we perform the subtraction input[0] - sum(input[1:])
		if (!data.empty()) {
			{
				auto const& batch_entry =
				    std::get<std::vector<common::TimedDataSequence<std::vector<Int8>>>>(
				        dynamic_cast<Transformation::Dynamics const&>(data.at(0).get()).value)
				        .at(j);
				for (size_t i = 0; i < m_size; ++i) {
					tmps[i] += static_cast<int64_t>(batch_entry.at(0).data[i]);
				}
			}
			// subtract all remaining inputs from temporary
			for (size_t k = 1; k < data.size(); ++k) {
				auto const& batch_entry =
				    std::get<std::vector<common::TimedDataSequence<std::vector<Int8>>>>(
				        dynamic_cast<Transformation::Dynamics const&>(data.at(k).get()).value)
				        .at(j);
				for (size_t i = 0; i < m_size; ++i) {
					tmps[i] -= static_cast<int64_t>(batch_entry.at(0).data[i]);
				}
			}
		}
		// restrict to range [-128,127]
		std::transform(tmps.begin(), tmps.end(), ret.at(j).at(0).data.begin(), [](auto const tmp) {
			return Int8(std::min(std::max(tmp, intmax_t(-128)), intmax_t(127)));
		});
		std::fill(tmps.begin(), tmps.end(), 0);
	}
	return {{ret}};
}

bool Subtraction::is_equal_to(Transformation::Function const& other) const
{
	auto const& o = static_cast<Subtraction const&>(other);
	return (m_num_inputs == o.m_num_inputs) && (m_size == o.m_size);
}

std::unique_ptr<Transformation::Function> Subtraction::copy() const
{
	return std::make_unique<Subtraction>(*this);
}

std::unique_ptr<Transformation::Function> Subtraction::move()
{
	return std::make_unique<Subtraction>(std::move(*this));
}

std::ostream& Subtraction::print(std::ostream& os) const
{
	return os << "Subtraction(num_inputs: " << m_num_inputs << ", size: " << m_size << ")";
}

} // namespace grenade::vx::signal_flow::vertex::transformation
