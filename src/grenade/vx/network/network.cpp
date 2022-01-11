#include "grenade/vx/network/network.h"

#include "hate/indent.h"
#include "hate/join.h"
#include <ostream>
#include <sstream>

namespace grenade::vx::network {

bool Network::operator==(Network const& other) const
{
	return populations == other.populations && projections == other.projections &&
	       madc_recording == other.madc_recording && cadc_recording == other.cadc_recording;
}

bool Network::operator!=(Network const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Network const& network)
{
	os << "Network(\n";
	os << "\tpopulations:\n";
	{
		std::vector<std::string> pops;
		for (auto const& [descriptor, population] : network.populations) {
			std::stringstream ss;
			ss << descriptor << ": ";
			std::visit([&ss](auto const& pop) { ss << pop; }, population);
			pops.push_back(ss.str());
		}
		os << hate::indent(hate::join_string(pops.begin(), pops.end(), "\n"), "\t\t");
		os << "\tprojections:\n";
	}
	{
		std::stringstream ss;
		for (auto const& [descriptor, projection] : network.projections) {
			ss << descriptor << ": " << projection << "\n";
		}
		os << hate::indent(ss.str(), "\t\t");
	}
	os << "\n)";
	return os;
}

} // namespace grenade::vx::network
