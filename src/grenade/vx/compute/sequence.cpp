#include "grenade/vx/compute/sequence.h"

#include "grenade/cerealization.h"
#include "grenade/vx/backend/connection.h"
#include "grenade/vx/config.h"
#include <cereal/types/list.hpp>
#include <cereal/types/variant.hpp>

namespace grenade::vx::compute {

namespace detail {

template <auto F>
struct run_input;

template <
    typename C,
    typename Ret,
    typename Input,
    Ret (C::*F)(Input const&, ChipConfig const&, grenade::vx::backend::Connection&) const>
struct run_input<F>
{
	typedef Input type;
};

template <auto F>
using run_input_t = typename run_input<F>::type;

} // namespace detail

Sequence::IOData Sequence::run(
    Sequence::IOData const& input, ChipConfig const& config, backend::Connection& connection)
{
	if (data.empty()) {
		throw std::runtime_error("Empty compute sequence can't be run.");
	}

	IOData tmp = input;

	auto const visit_entry = [&](auto const& e) -> IOData {
		typedef detail::run_input_t<&std::decay_t<decltype(e)>::run> Input;
		return e.run(std::get<Input>(tmp), config, connection);
	};
	for (auto const& entry : data) {
		tmp = std::visit(visit_entry, entry);
	}
	return tmp;
}

template <typename Archive>
void Sequence::serialize(Archive& ar, std::uint32_t const)
{
	ar(data);
}

} // namespace grenade::vx::compute

EXPLICIT_INSTANTIATE_CEREAL_SERIALIZE(grenade::vx::compute::Sequence)
CEREAL_CLASS_VERSION(grenade::vx::compute::Sequence, 0)
