#include "grenade/vx/signal_flow/transformation/concatenation.h"

#include "grenade/cerealization.h"
#include <stdexcept>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

namespace grenade::vx::signal_flow::transformation {

Concatenation::Concatenation(ConnectionType const type, std::vector<size_t> const& sizes) :
    m_type(type), m_sizes(sizes)
{}

Concatenation::~Concatenation() {}

std::vector<Port> Concatenation::inputs() const
{
	std::vector<Port> ports;
	for (auto const size : m_sizes) {
		ports.push_back(Port(size, m_type));
	}
	return ports;
}

Port Concatenation::output() const
{
	size_t acc = 0;
	for (auto const size : m_sizes) {
		acc += size;
	}
	return Port(acc, m_type);
}

Concatenation::Function::Value Concatenation::apply(std::vector<Function::Value> const& value) const
{
	size_t batch_size = 0;
	if (!value.empty()) {
		batch_size = std::visit([](auto const& v) { return v.size(); }, value.at(0));
	}
	auto const output_size = output().size;

	auto const copy = [&](auto& ret) {
		for (size_t i = 0; i < ret.size(); ++i) {
			size_t o_offset = 0;
			for (auto const& svalue : value) {
				auto const& v = std::get<std::remove_reference_t<decltype(ret)>>(svalue);
				ret.at(i).resize(v.at(i).size());
				for (size_t j = 0; j < v.at(i).size(); ++j) {
					ret.at(i).at(j).data.resize(output_size);
					// TODO: think about what to do with event time information, whether to
					// check for equality or just drop, etc.
					ret.at(i).at(j).chip_time = v.at(i).at(j).chip_time;
					ret.at(i).at(j).fpga_time = v.at(i).at(j).fpga_time;
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
			std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::UInt32>>> output(
			    batch_size);
			return copy(output);
		}
		case ConnectionType::Int8: {
			std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::Int8>>> output(
			    batch_size);
			return copy(output);
		}
		case ConnectionType::UInt5: {
			std::vector<signal_flow::TimedDataSequence<std::vector<signal_flow::UInt5>>> output(
			    batch_size);
			return copy(output);
		}
		default: {
			throw std::logic_error("Concatenation for given data type not implemented.");
		}
	};
}

bool Concatenation::equal(vertex::Transformation::Function const& other) const
{
	Concatenation const* o = dynamic_cast<Concatenation const*>(&other);
	if (o == nullptr) {
		return false;
	}
	return (m_type == o->m_type) && (m_sizes == o->m_sizes);
}

template <typename Archive>
void Concatenation::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_type);
	ar(m_sizes);
}

} // namespace grenade::vx::signal_flow::transformation

CEREAL_REGISTER_TYPE(grenade::vx::signal_flow::transformation::Concatenation)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    grenade::vx::signal_flow::vertex::Transformation::Function,
    grenade::vx::signal_flow::transformation::Concatenation)
EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::signal_flow::transformation::Concatenation)
CEREAL_CLASS_VERSION(grenade::vx::signal_flow::transformation::Concatenation, 0)
