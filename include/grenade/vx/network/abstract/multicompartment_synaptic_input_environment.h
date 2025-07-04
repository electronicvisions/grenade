#pragma once
#include "grenade/common/property.h"
#include "grenade/vx/genpybind.h"
#include "grenade/vx/network/abstract/multicompartment_top_bottom.h"
#include <utility>


namespace grenade::vx::network {
namespace abstract GENPYBIND_TAG_GRENADE_VX_NETWORK {

struct SYMBOL_VISIBLE GENPYBIND(inline_base("*")) SynapticInputEnvironment
    : public common::Property<SynapticInputEnvironment>
{
	SynapticInputEnvironment() = default;
	SynapticInputEnvironment(bool exitatory, int number);
	SynapticInputEnvironment(bool exitatory, NumberTopBottom numbers);

	bool exitatory;
	NumberTopBottom number_of_inputs;

	// Property Methods
	virtual std::unique_ptr<SynapticInputEnvironment> copy() const;
	virtual std::unique_ptr<SynapticInputEnvironment> move();

protected:
	virtual std::ostream& print(std::ostream& os) const;
	virtual bool is_equal_to(SynapticInputEnvironment const& other) const;
};

} // abstract
} // namespace grenade::vx::network
