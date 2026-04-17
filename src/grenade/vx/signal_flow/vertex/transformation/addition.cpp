#include "grenade/vx/signal_flow/vertex/transformation/addition.h"

#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/port_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/vertex/transformation.h"
#include <functional>
#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

Addition::Addition(size_t const num_inputs, size_t const size) :
    m_num_inputs(num_inputs), m_size(size)
{}

std::vector<Addition::Port> Addition::get_input_ports() const
{
	return std::vector{
	    m_num_inputs, Port(
	                      VertexPortType(ConnectionType::Int8),
	                      grenade::common::CuboidMultiIndexSequence({m_size}))};
}

std::vector<Addition::Port> Addition::get_output_ports() const
{
	return {Port(
	    VertexPortType(ConnectionType::Int8), grenade::common::CuboidMultiIndexSequence({m_size}))};
}

std::vector<Transformation::Results> Addition::apply(
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
		for (size_t k = 0; k < data.size(); ++k) {
			auto const& batch_entry =
			    std::get<std::vector<common::TimedDataSequence<std::vector<Int8>>>>(
			        dynamic_cast<Transformation::Dynamics const&>(data.at(k).get()).value)
			        .at(j);
			assert(batch_entry.size() == 1);
			assert(tmps.size() == batch_entry.at(0).data.size());
			for (size_t i = 0; i < m_size; ++i) {
				tmps[i] += batch_entry.at(0).data[i];
			}
		}
		// restrict to range [-128,127]
		std::transform(tmps.begin(), tmps.end(), ret.at(j).at(0).data.begin(), [](auto const tmp) {
			return Int8(std::min(std::max(tmp, intmax_t(-128)), intmax_t(127)));
		});
		std::fill(tmps.begin(), tmps.end(), 0);
	}
	return {{std::move(ret)}};
}

std::unique_ptr<Transformation::Function> Addition::copy() const
{
	return std::make_unique<Addition>(*this);
}

std::unique_ptr<Transformation::Function> Addition::move()
{
	return std::make_unique<Addition>(*this);
}

bool Addition::is_equal_to(Transformation::Function const& other) const
{
	auto const& o = static_cast<Addition const&>(other);
	return (m_num_inputs == o.m_num_inputs) && (m_size == o.m_size);
}

std::ostream& Addition::print(std::ostream& os) const
{
	return os << "Addition(num_inputs: " << m_num_inputs << ", size: " << m_size << ")";
}

} // namespace grenade::vx::signal_flow::vertex::transformation
