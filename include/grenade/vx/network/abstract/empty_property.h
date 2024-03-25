#pragma once
#include "grenade/vx/network/abstract/property.h"
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <type_traits>

namespace grenade::vx::network {

/**
 * Property type without content for using when the type information alone suffices.
 */
template <typename Derived, typename Base>
struct EmptyProperty : public Base
{
	virtual std::unique_ptr<Base> copy() const override;
	virtual std::unique_ptr<Base> move() override;
	virtual std::ostream& print(std::ostream& os) const override;

protected:
	virtual bool is_equal_to(Base const& other) const override;

private:
	static_assert(
	    std::is_base_of_v<Property<Base>, Base>,
	    "EmptyProperty expects Base to be derived from Property<Base>.");
};

} // namespace grenade::vx::network
