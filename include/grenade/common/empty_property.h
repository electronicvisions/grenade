#pragma once
#include "grenade/common/property.h"
#include "hate/visibility.h"
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <type_traits>

namespace grenade::common {

/**
 * Property type without content for using when the type information alone suffices.
 */
template <typename Derived, typename Base>
struct SYMBOL_VISIBLE EmptyProperty : public Base
{
	virtual std::unique_ptr<Base> copy() const override;
	virtual std::unique_ptr<Base> move() override;

protected:
	virtual std::ostream& print(std::ostream& os) const override;
	virtual bool is_equal_to(Base const& other) const override;

private:
	static_assert(
	    std::is_base_of_v<Property<Base>, Base>,
	    "EmptyProperty expects Base to be derived from Property<Base>.");
};

} // namespace grenade::common
