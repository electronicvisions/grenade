#include "grenade/vx/network/routing/csp/tracer.h"
#include "grenade/vx/network/routing/csp/router_space.h"
#include <iomanip>
#include <sstream>
#include <log4cxx/logger.h>


namespace grenade::vx::network::routing::csp {

IntTracer::IntTracer(std::string name) :
    m_logger(log4cxx::Logger::getLogger("grenade.network.routing.csp.IntTracer(" + name + ")"))
{
}

void IntTracer::init(Gecode::Space const&, Gecode::IntTraceRecorder const&) {}

void IntTracer::prune(
    Gecode::Space const& /* home */,
    Gecode::IntTraceRecorder const& t,
    Gecode::ViewTraceInfo const& vti,
    int i,
    Gecode::IntTraceDelta& d)
{
	std::stringstream ss;
	ss << "prune: [" << i << "] = " << t[i] << " - {";
	ss << d.min();
	if (d.width() > 1)
		ss << ".." << d.max();
	++d;
	while (d()) {
		ss << ',' << d.min();
		if (d.width() > 1)
			ss << ".." << d.max();
		++d;
	}
	ss << "} by " << vti;
	LOG4CXX_TRACE(m_logger, ss.str());
}

void IntTracer::fix(Gecode::Space const& /* home */, Gecode::IntTraceRecorder const& t)
{
	std::stringstream ss;
	ss << "fix: slack: ";
	double sl_i = static_cast<double>(t.slack().initial());
	double sl_p = static_cast<double>(t.slack().previous());
	double sl_c = static_cast<double>(t.slack().current());
	double p_c = 100.0 * (sl_c / sl_i);
	double p_d = 100.0 * (sl_p / sl_i) - p_c;
	ss << std::showpoint << std::setprecision(4) << p_c << "% - " << std::showpoint
	   << std::setprecision(4) << p_d << '%';
	LOG4CXX_TRACE(m_logger, ss.str());
}

void IntTracer::fail(Gecode::Space const& /* home */, Gecode::IntTraceRecorder const& t)
{
	std::stringstream ss;
	ss << "fail: slack: ";
	double sl_i = static_cast<double>(t.slack().initial());
	double sl_p = static_cast<double>(t.slack().previous());
	double sl_c = static_cast<double>(t.slack().current());
	double p_c = 100.0 * (sl_c / sl_i);
	double p_d = 100.0 * (sl_p / sl_i) - p_c;
	ss << std::showpoint << std::setprecision(4) << p_c << "% - " << std::showpoint
	   << std::setprecision(4) << p_d << '%';
	// ss << "\n" << dynamic_cast<RouterSpace const&>(home);
	LOG4CXX_TRACE(m_logger, ss.str());
}

void IntTracer::done(Gecode::Space const&, Gecode::IntTraceRecorder const&)
{
	LOG4CXX_TRACE(m_logger, "done.");
}

bool IntTracer::is_enabled() const
{
	return m_logger->isTraceEnabled();
}

SearchTracer::SearchTracer() :
    m_logger(log4cxx::Logger::getLogger("grenade.network.routing.csp.SearchTracer"))
{
}

void SearchTracer::init() {}

void SearchTracer::round(unsigned int eid)
{
	std::stringstream os;
	os << "round(e:" << eid << ")";
	LOG4CXX_TRACE(m_logger, os.str());
}

void SearchTracer::skip(const EdgeInfo& ei)
{
	std::stringstream os;
	os << "skip(w:" << ei.wid() << ",n:" << ei.nid() << ",a:" << ei.alternative() << ")";
	LOG4CXX_TRACE(m_logger, os.str());
}

void SearchTracer::node(const EdgeInfo& ei, const NodeInfo& ni)
{
	std::stringstream os;
	os << "node(";
	switch (ni.type()) {
		case NodeType::FAILED:
			os << "FAILED";
			break;
		case NodeType::SOLVED:
			os << "SOLVED";
			break;
		case NodeType::BRANCH:
			os << "BRANCH(" << ni.choice().alternatives() << ")";
			break;
	}
	if (!ei) {
		os << ",root";
	}
	os << ",w:" << ni.wid() << ',';
	if (ei) {
		os << "p:" << ei.nid() << ',';
	}
	os << "n:" << ni.nid() << ')';
	if (ei) {
		if (ei.wid() != ni.wid())
			os << " [stolen from w:" << ei.wid() << "]";
		os << "\n\t" << ei.string();
	}
	LOG4CXX_TRACE(m_logger, os.str());
}

void SearchTracer::done(void) {}

SearchTracer::~SearchTracer() {}

bool SearchTracer::is_enabled() const
{
	return m_logger->isTraceEnabled();
}

} // namespace grenade::vx::network::routing::csp