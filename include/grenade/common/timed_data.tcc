#pragma once
#include "grenade/common/timed_data.h"
#include <ostream>
#include <variant>

namespace grenade::common {

template <typename Time, typename T>
TimedData<Time, T>::TimedData(Time const& time, T const& data) : time(time), data(data)
{
}

template <typename Time, typename T>
bool TimedData<Time, T>::operator==(TimedData const& other) const
{
	return time == other.time && data == other.data;
}

template <typename Time, typename T>
bool TimedData<Time, T>::operator!=(TimedData const& other) const
{
	return !(*this == other);
}

template <typename T>
struct is_variant : std::false_type
{};

template <typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type
{};

template <typename Time, typename T>
std::ostream& operator<<(std::ostream& os, TimedData<Time, T> const& data)
{
	os << "TimedData(time: " << data.time << ", data: ";
	if constexpr (is_variant<T>::value) {
		std::visit([&os](auto const& d) { os << d; }, data.data);
	} else {
		os << data.data;
	}

	os << ")";
	return os;
}

} // namespace grenade::common
