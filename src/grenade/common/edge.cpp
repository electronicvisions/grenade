#include "grenade/common/edge.h"
#include "hate/indent.h"

#include <stdexcept>

namespace grenade::common {

namespace {

void check_channels(
    MultiIndexSequence const& channels_on_source, MultiIndexSequence const& channels_on_target)
{
	if (channels_on_source.size() != channels_on_target.size()) {
		throw std::invalid_argument("Source and target channels not bijective.");
	}
}

} // namespace

Edge::Edge(
    MultiIndexSequence const& channels_on_source, MultiIndexSequence const& channels_on_target) :
    m_channels_on_source(), m_channels_on_target()
{
	check_channels(channels_on_source, channels_on_target);
	m_channels_on_source = channels_on_source;
	m_channels_on_target = channels_on_target;
}

Edge::Edge(MultiIndexSequence&& channels_on_source, MultiIndexSequence&& channels_on_target) :
    m_channels_on_source(), m_channels_on_target()
{
	check_channels(channels_on_source, channels_on_target);
	m_channels_on_source = std::move(channels_on_source);
	m_channels_on_target = std::move(channels_on_target);
}

Edge::Edge(
    MultiIndexSequence const& channels_on_source,
    MultiIndexSequence const& channels_on_target,
    PortOnVertex port_on_source,
    PortOnVertex port_on_target) :
    port_on_source(port_on_source),
    port_on_target(port_on_target),
    m_channels_on_source(),
    m_channels_on_target()
{
	check_channels(channels_on_source, channels_on_target);
	m_channels_on_source = channels_on_source;
	m_channels_on_target = channels_on_target;
}

Edge::Edge(
    MultiIndexSequence&& channels_on_source,
    MultiIndexSequence&& channels_on_target,
    PortOnVertex port_on_source,
    PortOnVertex port_on_target) :
    port_on_source(port_on_source),
    port_on_target(port_on_target),
    m_channels_on_source(),
    m_channels_on_target()
{
	check_channels(channels_on_source, channels_on_target);
	m_channels_on_source = std::move(channels_on_source);
	m_channels_on_target = std::move(channels_on_target);
}

MultiIndexSequence const& Edge::get_channels_on_source() const
{
	return *m_channels_on_source;
}

void Edge::set_channels_on_source(MultiIndexSequence const& value)
{
	check_channels(*m_channels_on_source, value);
	m_channels_on_source = value;
}

void Edge::set_channels_on_source(MultiIndexSequence&& value)
{
	check_channels(*m_channels_on_source, value);
	m_channels_on_source = std::move(value);
}

MultiIndexSequence const& Edge::get_channels_on_target() const
{
	return *m_channels_on_target;
}

void Edge::set_channels_on_target(MultiIndexSequence const& value)
{
	check_channels(value, *m_channels_on_target);
	m_channels_on_target = value;
}

void Edge::set_channels_on_target(MultiIndexSequence&& value)
{
	check_channels(value, *m_channels_on_target);
	m_channels_on_target = std::move(value);
}

std::unique_ptr<Edge> Edge::copy() const
{
	return std::make_unique<Edge>(*this);
}

std::unique_ptr<Edge> Edge::move()
{
	return std::make_unique<Edge>(std::move(*this));
}

std::ostream& Edge::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Edge(\n";
	ios << hate::Indentation("\t");
	ios << "port_on_source: " << port_on_source << "\n";
	ios << "port_on_target: " << port_on_target << "\n";
	ios << "channels on source: " << m_channels_on_source << "\n";
	ios << "channels on target: " << m_channels_on_target << "\n";
	ios << hate::Indentation() << ")";
	return os;
}

bool Edge::is_equal_to(Edge const& other) const
{
	return port_on_source == other.port_on_source && port_on_target == other.port_on_target &&
	       m_channels_on_source == other.m_channels_on_source &&
	       m_channels_on_target == other.m_channels_on_target;
}

} // namespace grenade::common
