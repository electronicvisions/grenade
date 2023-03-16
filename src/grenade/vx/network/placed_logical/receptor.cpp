#include "grenade/vx/network/placed_logical/receptor.h"

#include <ostream>

namespace grenade::vx::network::placed_logical {

Receptor::Receptor(ID const id, Type const type) : id(id), type(type) {}

bool Receptor::operator==(Receptor const& other) const
{
	return id == other.id && type == other.type;
}

bool Receptor::operator!=(Receptor const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, Receptor const& receptor)
{
	os << "Receptor(" << receptor.id << ", " << receptor.type << ")";
	return os;
}

size_t Receptor::hash() const
{
	// We include the type name in the hash to reduce the number of hash collisions in
	// python code, where __hash__ is used in heterogeneous containers.
	static const size_t seed = boost::hash_value(typeid(Receptor).name());
	size_t hash = seed;
	boost::hash_combine(hash, hash_value(*this));
	return hash;
}

size_t hash_value(Receptor const& receptor)
{
	size_t hash = 0;
	boost::hash_combine(hash, boost::hash<Receptor::ID>()(receptor.id));
	boost::hash_combine(hash, boost::hash<Receptor::Type>()(receptor.type));
	return hash;
}

} // namespace grenade::vx::network::placed_logical
