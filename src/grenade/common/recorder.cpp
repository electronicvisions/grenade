#include "grenade/common/recorder.h"

#include "grenade/common/connection_on_executor.h"
#include "grenade/common/execution_instance_id.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "hate/indent.h"
#include <memory>

namespace grenade::common {

Recorder::Recorder(
    MultiIndexSequence const& shape,
    TimeDomainOnTopology const& time_domain,
    std::optional<grenade::common::ExecutionInstanceOnExecutor> const&
        execution_instance_on_executor) :
    PartitionedVertex(execution_instance_on_executor), m_shape(shape), m_time_domain(time_domain)
{
}

MultiIndexSequence const& Recorder::get_shape() const
{
	return *m_shape;
}

void Recorder::set_shape(MultiIndexSequence const& value)
{
	m_shape = value;
}

void Recorder::set_shape(MultiIndexSequence&& value)
{
	m_shape = std::move(value);
}

std::optional<TimeDomainOnTopology> Recorder::get_time_domain() const
{
	return m_time_domain;
}

void Recorder::set_time_domain(TimeDomainOnTopology const& value)
{
	m_time_domain = value;
}

std::unique_ptr<Recorder::Results> Recorder::create_empty_results(size_t) const
{
	return nullptr;
}

std::unique_ptr<Recorder> Recorder::get_section(MultiIndexSequence const& sequence) const
{
	if (!m_shape->includes(sequence)) {
		throw std::invalid_argument(
		    "Given sequence not included in shape of recorder to get section from.");
	}

	auto copy_ptr = copy();
	dynamic_cast<Recorder&>(*copy_ptr).set_shape(sequence);
	return std::unique_ptr<Recorder>(dynamic_cast<Recorder*>(copy_ptr.release()));
}

bool Recorder::is_equal_to(Vertex const& other) const
{
	return PartitionedVertex::is_equal_to(other) &&
	       m_shape == static_cast<Recorder const&>(other).m_shape &&
	       m_time_domain == static_cast<Recorder const&>(other).m_time_domain;
}

std::ostream& Recorder::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Recorder(\n";
	ios << hate::Indentation("\t");
	PartitionedVertex::print(ios) << "\n";
	ios << "shape: " << m_shape << "\n";
	ios << "time_domain: " << m_time_domain << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

} // namespace grenade::common
