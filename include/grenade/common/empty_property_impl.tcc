#pragma once
#include "grenade/common/empty_property.h"
#include "hate/type_index.h"
#include <ostream>

namespace grenade::common {

template <typename Derived, typename Base>
std::unique_ptr<Base> EmptyProperty<Derived, Base>::copy() const
{
	return std::make_unique<Derived>(dynamic_cast<Derived const&>(*this));
}

template <typename Derived, typename Base>
std::unique_ptr<Base> EmptyProperty<Derived, Base>::move()
{
	return std::make_unique<Derived>(std::move(dynamic_cast<Derived&>(*this)));
}

template <typename Derived, typename Base>
std::ostream& EmptyProperty<Derived, Base>::print(std::ostream& os) const
{
	return os << hate::name<Derived>() << "()";
}

template <typename Derived, typename Base>
bool EmptyProperty<Derived, Base>::is_equal_to(Base const& /* other */) const
{
	return true;
}

} // namespace grenade::common
