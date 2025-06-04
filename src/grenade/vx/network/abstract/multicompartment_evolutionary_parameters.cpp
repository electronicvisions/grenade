#include "grenade/vx/network/abstract/multicompartment_evolutionary_parameters.h"

namespace grenade::vx::network::abstract {

std::ostream& operator<<(std::ostream& os, EvolutionaryParameters const& parameters)
{
	os << std::endl
	   << "Run Parameters:                      " << std::endl
	   << "Time limit:                          " << parameters.time_limit << std::endl
	   << "Run limit:                           " << parameters.run_limit << std::endl
	   << "x_max:                               " << parameters.x_max << std::endl
	   << "Population size:                     " << parameters.population_size << std::endl
	   << "Number in Hall of Fame:              " << parameters.number_hall_of_fame << std::endl
	   << "Number of Tournament contestants:    " << parameters.tournament_contestants << std::endl
	   << "Mating probability:                  " << parameters.p_mate << std::endl
	   << "Mating block size:                   " << parameters.mating_block_size << std::endl
	   << "Mutation probability for individual: " << parameters.p_mutate_individual << std::endl
	   << "Mutation probability for gen:        " << parameters.p_mutate_gene << std::endl
	   << "Mutation probabiity initial setup:   " << parameters.p_mutate_initial << std::endl
	   << "Shift probability:                   " << parameters.p_shift << std::endl
	   << "Shift range:                         " << parameters.min_shift << ", "
	   << parameters.max_shift << std::endl
	   << "Adding column probability:           " << parameters.p_add << std::endl
	   << "Removing column probability:         " << parameters.p_remove << std::endl
	   << "Adding/Removing blocks:              " << parameters.together_add_remove << std::endl
	   << "Number of added/removed columns:     " << parameters.number_columns_add_remove
	   << std::endl
	   << "Limits for adding/removing:          " << parameters.lower_limit_add_remove << ", "
	   << parameters.upper_limit_add_remove << std::endl;

	return os;
}

} // namespace grenade::vx::network::abstract