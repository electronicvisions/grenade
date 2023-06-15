#include "grenade/vx/signal_flow/vertex/transformation/relu.h"

#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

ReLU::ReLU(size_t const size) : m_size(size) {}

ReLU::~ReLU() {}

std::vector<Port> ReLU::inputs() const
{
	return {Port(m_size, ConnectionType::Int8)};
}

Port ReLU::output() const
{
	return Port(m_size, ConnectionType::Int8);
}

ReLU::Function::Value ReLU::apply(std::vector<Function::Value> const& value) const
{
	size_t batch_size = 0;
	batch_size = std::visit([](auto const& v) { return v.size(); }, value.at(0));

	std::vector<common::TimedDataSequence<std::vector<Int8>>> ret(batch_size);
	for (auto& e : ret) {
		// TODO: Think about what shall happen with timing info and when multiple events are present
		e.resize(1);
		e.at(0).data.resize(m_size);
	}
	for (size_t j = 0; j < ret.size(); ++j) {
		auto const& d =
		    std::get<std::vector<common::TimedDataSequence<std::vector<Int8>>>>(value.at(0)).at(j);
		assert(d.size() == 1);
		for (size_t i = 0; i < m_size; ++i) {
			ret.at(j).at(0).data.at(i) = std::max(d.at(0).data.at(i), Int8(0));
		}
	}
	return ret;
}

bool ReLU::equal(Transformation::Function const& other) const
{
	if (auto const o = dynamic_cast<ReLU const*>(&other); o != nullptr) {
		return (m_size == o->m_size);
	}
	return false;
}

} // namespace grenade::vx::signal_flow::vertex::transformation
