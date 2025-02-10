#include "grenade/common/resource_estimator.h"

#include "grenade/common/topology.h"
#include <stdexcept>

namespace grenade::common {

ResourceEstimator::Resource::~Resource() {}

bool ResourceEstimator::Resource::any_scalar_greater(Resource const& other) const
{
	if (typeid(*this) != typeid(other)) {
		throw std::invalid_argument("Resource types don't match.");
	}
	auto const values = scalar_values();
	auto const other_values = other.scalar_values();
	if (values.size() != other_values.size()) {
		throw std::invalid_argument("Resource scalar value count doesn't match.");
	}
	for (size_t i = 0; i < values.size(); ++i) {
		if (values[i] > other_values[i]) {
			return true;
		}
	}
	return false;
}

size_t ResourceEstimator::Resource::max_scalar_factor(Resource const& other) const
{
	if (typeid(*this) != typeid(other)) {
		throw std::invalid_argument("Resource types don't match.");
	}
	auto values = scalar_values();
	auto const other_values = other.scalar_values();
	assert(values.size() == other_values.size());
	for (size_t i = 0; i < values.size(); ++i) {
		if (values[i] != 0) {
			values[i] = other_values[i] / values[i];
		}
	}
	if (values.empty()) {
		throw std::runtime_error("No max scalar factor value in empty resource.");
	}
	return *std::max_element(values.begin(), values.end());
}

std::unique_ptr<ResourceEstimator::Resource> ResourceEstimator::Resource::operator+(
    Resource const& other) const
{
	auto ret = copy();
	assert(ret);
	*ret += other;
	return ret;
}

std::unique_ptr<ResourceEstimator::Resource> ResourceEstimator::Resource::operator*(
    size_t factor) const
{
	auto ret = copy();
	assert(ret);
	*ret *= factor;
	return ret;
}


ResourceEstimator::ResourceEstimator(Topology const& topology) : m_topology(topology) {}

} // namespace grenade::common
