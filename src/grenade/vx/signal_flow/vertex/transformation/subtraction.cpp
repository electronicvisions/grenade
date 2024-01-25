#include "grenade/vx/signal_flow/vertex/transformation/subtraction.h"

#include <stdexcept>

namespace grenade::vx::signal_flow::vertex::transformation {

Subtraction::Subtraction(size_t const num_inputs, size_t const size) :
    m_num_inputs(num_inputs), m_size(size)
{}

Subtraction::~Subtraction() {}

std::vector<Port> Subtraction::inputs() const
{
	return std::vector{m_num_inputs, Port(m_size, ConnectionType::Int8)};
}

Port Subtraction::output() const
{
	return Port(m_size, ConnectionType::Int8);
}

Subtraction::Function::Value Subtraction::apply(std::vector<Function::Value> const& value) const
{
	size_t batch_size = 0;
	if (!value.empty()) {
		batch_size = std::visit([](auto const& v) { return v.size(); }, value.at(0));
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
		if (!value.empty()) {
			{
				auto const& batch_entry =
				    std::get<std::vector<common::TimedDataSequence<std::vector<Int8>>>>(value.at(0))
				        .at(j);
				for (size_t i = 0; i < m_size; ++i) {
					tmps[i] += static_cast<int64_t>(batch_entry.at(0).data[i]);
				}
			}
			// subtract all remaining inputs from temporary
			for (size_t k = 1; k < value.size(); ++k) {
				auto const& batch_entry =
				    std::get<std::vector<common::TimedDataSequence<std::vector<Int8>>>>(value.at(k))
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
	return ret;
}

bool Subtraction::equal(Transformation::Function const& other) const
{
	if (auto const o = dynamic_cast<Subtraction const*>(&other); o != nullptr) {
		return (m_num_inputs == o->m_num_inputs) && (m_size == o->m_size);
	}
	return false;
}

std::unique_ptr<Transformation::Function> Subtraction::clone() const
{
	return std::make_unique<Subtraction>(*this);
}

} // namespace grenade::vx::signal_flow::vertex::transformation
