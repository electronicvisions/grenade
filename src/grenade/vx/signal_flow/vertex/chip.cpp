#include "grenade/vx/signal_flow/vertex/chip.h"

#include "grenade/common/multi_index_sequence/list.h"
#include "hate/indent.h"
#include <ostream>

namespace grenade::vx::signal_flow::vertex {

Chip::Parameterization::Parameterization(lola::vx::v3::Chip base) : base(std::move(base)) {}

std::unique_ptr<grenade::common::PortData> Chip::Parameterization::copy() const
{
	return std::make_unique<Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> Chip::Parameterization::move()
{
	return std::make_unique<Parameterization>(std::move(*this));
}

bool Chip::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	return base == static_cast<Parameterization const&>(other).base;
}

std::ostream& Chip::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n"
	    << hate::Indentation("\t") << base << "\n"
	    << hate::Indentation() << ")";
	return os;
}

Chip::Chip(
    common::ChipOnConnection const& chip_on_connection,
    grenade::common::TimeDomainOnTopology const& time_domain,
    grenade::common::ExecutionInstanceOnExecutor const& execution_instance_on_executor) :
    EntityOnChip(chip_on_connection, time_domain, execution_instance_on_executor)
{
}

bool Chip::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	return input_port_on_vertex == 0 && dynamic_cast<Parameterization const*>(&data) != nullptr;
}

std::vector<grenade::common::Vertex::Port> Chip::get_input_ports() const
{
	return {grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::ListMultiIndexSequence({grenade::common::MultiIndex({0})}))};
}

std::vector<grenade::common::Vertex::Port> Chip::get_output_ports() const
{
	return {};
}

std::unique_ptr<grenade::common::Vertex> Chip::copy() const
{
	return std::make_unique<Chip>(*this);
}

std::unique_ptr<grenade::common::Vertex> Chip::move()
{
	return std::make_unique<Chip>(std::move(*this));
}

bool Chip::is_equal_to(Vertex const& other) const
{
	return EntityOnChip::is_equal_to(other);
}

std::ostream& Chip::print(std::ostream& os) const
{
	os << "Chip(";
	EntityOnChip::print(os) << ")";
	return os;
}

} // namespace grenade::vx::signal_flow::vertex
