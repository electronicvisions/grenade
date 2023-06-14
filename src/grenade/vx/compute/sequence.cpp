#include "grenade/vx/compute/sequence.h"

#include "grenade/vx/execution/jit_graph_executor.h"

namespace grenade::vx::compute {

namespace detail {

template <auto F>
struct run_input;

template <
    typename C,
    typename Ret,
    typename Input,
    Ret (C::*F)(Input const&, lola::vx::v3::Chip const&, execution::JITGraphExecutor&) const>
struct run_input<F>
{
	typedef Input type;
};

template <auto F>
using run_input_t = typename run_input<F>::type;

} // namespace detail

Sequence::IOData Sequence::run(
    Sequence::IOData const& input,
    lola::vx::v3::Chip const& config,
    execution::JITGraphExecutor& executor)
{
	if (data.empty()) {
		throw std::runtime_error("Empty compute sequence can't be run.");
	}

	IOData tmp = input;

	auto const visit_entry = [&](auto const& e) -> IOData {
		typedef detail::run_input_t<&std::decay_t<decltype(e)>::run> Input;
		return e.run(std::get<Input>(tmp), config, executor);
	};
	for (auto const& entry : data) {
		tmp = std::visit(visit_entry, entry);
	}
	return tmp;
}

} // namespace grenade::vx::compute
