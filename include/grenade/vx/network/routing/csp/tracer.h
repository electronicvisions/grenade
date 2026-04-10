#pragma once
#include "hate/visibility.h"
#include <memory>
#include <gecode/int.hh>
#include <gecode/search.hh>

namespace log4cxx {
struct Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx


namespace grenade::vx::network::routing::csp {

/**
 * Tracer to logger for int variables.
 */
struct SYMBOL_VISIBLE IntTracer : public Gecode::IntTracer
{
	/**
	 * Construct int variable tracer with name.
	 * @param name Name to use for tracer
	 */
	IntTracer(std::string name = "");

	virtual void init(Gecode::Space const& home, Gecode::IntTraceRecorder const& t);

	virtual void prune(
	    Gecode::Space const& home,
	    Gecode::IntTraceRecorder const& t,
	    Gecode::ViewTraceInfo const& vti,
	    int i,
	    Gecode::IntTraceDelta& d);

	virtual void fix(Gecode::Space const& home, Gecode::IntTraceRecorder const& t);

	virtual void fail(Gecode::Space const& home, Gecode::IntTraceRecorder const& t);

	virtual void done(Gecode::Space const& home, Gecode::IntTraceRecorder const& t);

	bool is_enabled() const;

private:
	log4cxx::LoggerPtr m_logger;
};


/**
 * Tracer to logger for search algorithm.
 */
struct SYMBOL_VISIBLE SearchTracer : public Gecode::SearchTracer
{
	SearchTracer();

	virtual void init();

	virtual void round(unsigned int eid);

	virtual void skip(const EdgeInfo& ei);

	virtual void node(const EdgeInfo& ei, const NodeInfo& ni);

	virtual void done();

	virtual ~SearchTracer();

	bool is_enabled() const;

private:
	log4cxx::LoggerPtr m_logger;
};

} // namespace grenade::vx::network::routing::csp