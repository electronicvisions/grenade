#include "grenade/vx/compute/conv1d.h"

#include "grenade/cerealization.h"
#include "grenade/vx/backend/connection.h"
#include "grenade/vx/config.h"
#include "grenade/vx/event.h"
#include "grenade/vx/execution_instance.h"
#include "grenade/vx/graph.h"
#include "grenade/vx/input.h"
#include "grenade/vx/io_data_map.h"
#include "grenade/vx/jit_graph_executor.h"
#include "grenade/vx/range_split.h"
#include "grenade/vx/single_chip_execution_instance_manager.h"
#include "halco/common/cerealization_geometry.h"
#include "hate/math.h"
#include "hate/timer.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <cereal/types/vector.hpp>
#include <log4cxx/logger.h>
#include <tbb/parallel_for_each.h>

#include <algorithm>

namespace grenade::vx::compute {

size_t Conv1d::input_size() const
{
	return m_input_size;
}

size_t Conv1d::output_size() const
{
	size_t const num = (m_input_size - m_kernel_size) / m_stride + 1;
	return m_out_channels * num;
}

void Conv1d::build_mac(Weights&& weights)
{
	// ensure weights shape
	if (weights.empty()) {
		throw std::runtime_error("Weights are empty.");
	}
	m_out_channels = weights.size();
	m_in_channels = weights.at(0).size();
	if (weights.at(0).empty()) {
		throw std::runtime_error("Weight in_channel size is zero.");
	}
	m_kernel_size = weights.at(0).at(0).size();
	for (auto const& in : weights) {
		if (in.size() != m_in_channels) {
			throw std::runtime_error("Weight in_channel size is not constant.");
		}
		for (auto const& k : in) {
			if (k.size() != m_kernel_size) {
				throw std::runtime_error("Weight kernel_size is not constant.");
			}
		}
	}

	// generate Conv1d weight matrix
	grenade::vx::compute::MAC::Weights mac_weights(m_in_channels * m_kernel_size);
	for (size_t i = 0; i < m_in_channels; ++i) {
		for (size_t k = 0; k < m_kernel_size; ++k) {
			auto& local = mac_weights.at(i * m_kernel_size + k);
			local.resize(m_out_channels);
			for (size_t o = 0; o < m_out_channels; ++o) {
				local.at(o) = weights.at(o).at(i).at(k);
			}
		}
	}

	m_mac = MAC(std::move(mac_weights), m_num_sends, m_wait_between_events, m_enable_loopback);
}

std::vector<std::vector<Int8>> Conv1d::run(
    Activations const& inputs, ChipConfig const& config, backend::Connection& connection) const
{
	if (inputs.size() == 0) {
		throw std::runtime_error("Provided inputs are empty.");
	}

	size_t const batch_entry_size = inputs.front().size();
	if (batch_entry_size != m_input_size * m_in_channels) {
		throw std::runtime_error("Provided input size does not match enpectation.");
	}
	for (auto const& batch_entry : inputs) {
		if (batch_entry.size() != batch_entry_size) {
			throw std::runtime_error("Provided batch entries don't share a common size.");
		}
	}

	size_t const num = (m_input_size - m_kernel_size) / m_stride + 1;
	grenade::vx::compute::Conv1d::Activations mac_inputs(inputs.size() * num);

	for (size_t b = 0; b < inputs.size(); ++b) {
		for (size_t n = 0; n < num; ++n) {
			auto& local = mac_inputs.at(b * num + n);
			local.resize(m_kernel_size * m_in_channels);
			for (size_t i = 0; i < m_in_channels; ++i) {
				for (size_t k = 0; k < m_kernel_size; ++k) {
					local.at(i * m_kernel_size + k) =
					    inputs.at(b).at(i * m_input_size + (n * m_stride) + k);
				}
			}
		}
	}

	auto const mac_output = m_mac.run(mac_inputs, config, connection);

	std::vector<std::vector<Int8>> output(inputs.size());
	for (size_t b = 0; b < output.size(); ++b) {
		auto& local = output.at(b);
		local.resize(m_out_channels * num);
		for (size_t n = 0; n < num; ++n) {
			auto& mac_local = mac_output.at(b * num + n);
			for (size_t o = 0; o < m_out_channels; ++o) {
				local.at(n * m_out_channels + o) = mac_local.at(o);
			}
		}
	}
	return output;
}

template <typename Archive>
void Conv1d::serialize(Archive& ar, std::uint32_t const)
{
	ar(m_enable_loopback);
	ar(m_input_size);
	ar(m_kernel_size);
	ar(m_in_channels);
	ar(m_out_channels);
	ar(m_stride);
	ar(m_mac);
	ar(m_num_sends);
	ar(m_wait_between_events);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::Conv1d)
CEREAL_CLASS_VERSION(grenade::vx::compute::Conv1d, 0)
