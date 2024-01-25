#include "grenade/vx/signal_flow/vertex/transformation/converting_relu.h"

#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

ConvertingReLU::ConvertingReLU(size_t const size, uint32_t const shift) :
    m_size(size), m_shift(shift)
{}

ConvertingReLU::~ConvertingReLU() {}

std::vector<Port> ConvertingReLU::inputs() const
{
	return {Port(m_size, ConnectionType::Int8)};
}

Port ConvertingReLU::output() const
{
	return Port(m_size, ConnectionType::UInt5);
}

ConvertingReLU::Function::Value ConvertingReLU::apply(
    std::vector<Function::Value> const& value) const
{
	size_t batch_size = 0;
	batch_size = std::visit([](auto const& v) { return v.size(); }, value.at(0));

	std::vector<common::TimedDataSequence<std::vector<UInt5>>> ret(batch_size);
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
			ret.at(j).at(0).data.at(i) = UInt5(std::min(
			    static_cast<UInt5::value_type>(std::max(d.at(0).data.at(i), Int8(0)) >> m_shift),
			    UInt5::max));
		}
	}
	return ret;
}

bool ConvertingReLU::equal(Transformation::Function const& other) const
{
	if (auto const o = dynamic_cast<ConvertingReLU const*>(&other); o != nullptr) {
		return (m_size == o->m_size);
	}
	return false;
}

std::unique_ptr<Transformation::Function> ConvertingReLU::clone() const
{
	return std::make_unique<ConvertingReLU>(*this);
}

} // namespace grenade::vx::signal_flow::vertex::transformation
