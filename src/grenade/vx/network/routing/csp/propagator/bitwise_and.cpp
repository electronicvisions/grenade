#include "grenade/vx/network/routing/csp/propagator/bitwise_and.h"

#include <gecode/int.hh>
#include <gecode/int/support-values.hh>
#include <gecode/search.hh>

namespace grenade::vx::network::routing::csp::propagator {

namespace {

/**
 * Bitwise AND propagator inspired by multiplication propagator.
 */
template <
    Gecode::PropCond PA,
    Gecode::PropCond PB,
    Gecode::PropCond PC,
    typename VA,
    typename VB,
    typename VC>
class BitwiseAnd : public Gecode::MixTernaryPropagator<VA, PA, VB, PB, VC, PC>
{
public:
	typedef Gecode::MixTernaryPropagator<VA, PA, VB, PB, VC, PC> base_t;
	using base_t::x0;
	using base_t::x1;
	using base_t::x2;

	BitwiseAnd(Gecode::Home home, VA x0, VB x1, VC x2) : base_t(home, x0, x1, x2) {}

	static Gecode::ExecStatus post(Gecode::Home home, VA x0, VB x1, VC x2)
	{
		(void) new (home) BitwiseAnd(home, x0, x1, x2);
		return Gecode::ES_OK;
	}
	BitwiseAnd(Gecode::Space& home, BitwiseAnd& p) : base_t(home, p) {}

	virtual Gecode::Propagator* copy(Gecode::Space& home)
	{
		return new (home) BitwiseAnd(home, *this);
	}

	virtual Gecode::ExecStatus propagate(Gecode::Space& home, const Gecode::ModEventDelta&)
	{
		Gecode::Region r;
		Gecode::Int::SupportValues<VA, Gecode::Region> s0(r, x0);
		Gecode::Int::SupportValues<VB, Gecode::Region> s1(r, x1);
		Gecode::Int::SupportValues<VC, Gecode::Region> s2(r, x2);
		while (s0()) {
			while (s1()) {
				if (s2.support(s0.val() & s1.val())) {
					s0.support();
					s1.support();
				}
				++s1;
			}
			s1.reset();
			++s0;
		}
		GECODE_ME_CHECK(s0.tell(home));
		GECODE_ME_CHECK(s1.tell(home));
		GECODE_ME_CHECK(s2.tell(home));
		return x0.assigned() && x1.assigned() ? home.ES_SUBSUMED(*this) : Gecode::ES_FIX;
	}
};

} // namespace

void bitwise_and(Gecode::Home home, Gecode::IntVar x0, Gecode::IntVar x1, Gecode::IntVar x2)
{
	GECODE_POST;
	GECODE_ES_FAIL((BitwiseAnd<
	                Gecode::Int::PC_INT_VAL, Gecode::Int::PC_INT_VAL, Gecode::Int::PC_INT_VAL,
	                Gecode::Int::IntView, Gecode::Int::IntView, Gecode::Int::IntView>::
	                    post(
	                        home, Gecode::Int::IntView(x0.varimp()),
	                        Gecode::Int::IntView(x1.varimp()), Gecode::Int::IntView(x2.varimp()))));
}

void bitwise_and(Gecode::Home home, int x0, Gecode::IntVar x1, Gecode::IntVar x2)
{
	GECODE_POST;
	GECODE_ES_FAIL((BitwiseAnd<
	                Gecode::Int::PC_INT_VAL, Gecode::Int::PC_INT_VAL, Gecode::Int::PC_INT_VAL,
	                Gecode::Int::ConstIntView, Gecode::Int::IntView, Gecode::Int::IntView>::
	                    post(
	                        home, Gecode::Int::ConstIntView(x0), Gecode::Int::IntView(x1.varimp()),
	                        Gecode::Int::IntView(x2.varimp()))));
}

} // namespace grenade::vx::network::routing::csp::propagator