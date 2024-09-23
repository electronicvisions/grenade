#include "grenade/common/projection_connector/sequence.h"

#include "cereal/types/halco/common/geometry.h"
#include "grenade/common/edge.h"
#include "grenade/common/execution_instance_on_executor.h"
#include "grenade/common/multi_index.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/multi_index_sequence/list.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/common/vertex_port_type/empty.h"
#include <stdexcept>
#include <cereal/archives/json.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/polymorphic.hpp>
#include <gtest/gtest.h>

using namespace grenade::common;


TEST(SequenceConnector, General)
{
	// connection_sequence not included in input x output sequence
	EXPECT_THROW(
	    SequenceConnector(
	        CuboidMultiIndexSequence({5}), CuboidMultiIndexSequence({7}),
	        ListMultiIndexSequence({
	            MultiIndex({10, 1}),
	        })),
	    std::invalid_argument);

	CuboidMultiIndexSequence input_sequence({5});
	CuboidMultiIndexSequence output_sequence({7});
	ListMultiIndexSequence connection_sequence(
	    {MultiIndex({0, 1}), MultiIndex({3, 5}), MultiIndex({2, 1})});

	SequenceConnector connector(input_sequence, output_sequence, connection_sequence);

	EXPECT_TRUE(connector.get_input_sequence());
	EXPECT_EQ(*connector.get_input_sequence(), input_sequence);

	EXPECT_TRUE(connector.get_output_sequence());
	EXPECT_EQ(*connector.get_output_sequence(), output_sequence);

	EXPECT_EQ(connector.get_num_synapses(*input_sequence.cartesian_product(output_sequence)), 3);
	EXPECT_EQ(
	    connector.get_num_synapses(
	        *input_sequence.cartesian_product(CuboidMultiIndexSequence({2}))),
	    2);

	EXPECT_EQ(
	    connector.get_num_synapse_parameterizations(
	        *input_sequence.cartesian_product(output_sequence)),
	    3);
	EXPECT_EQ(
	    connector.get_num_synapse_parameterizations(
	        *input_sequence.cartesian_product(CuboidMultiIndexSequence({2}))),
	    2);

	EXPECT_TRUE(connector.get_synapse_parameterization_indices(
	    *input_sequence.cartesian_product(output_sequence)));
	EXPECT_EQ(
	    *connector.get_synapse_parameterization_indices(
	        *input_sequence.cartesian_product(output_sequence)),
	    CuboidMultiIndexSequence({3}));
	EXPECT_EQ(
	    *connector.get_synapse_parameterization_indices(
	        *input_sequence.cartesian_product(CuboidMultiIndexSequence({2}))),
	    ListMultiIndexSequence({MultiIndex({0}), MultiIndex({2})}));

	EXPECT_TRUE(connector.get_section(*input_sequence.cartesian_product(output_sequence)));
	EXPECT_EQ(
	    *connector.get_section(*input_sequence.cartesian_product(output_sequence)), connector);

	auto const connector_section =
	    connector.get_section(*input_sequence.cartesian_product(CuboidMultiIndexSequence({2})));
	SequenceConnector connector_section_expectation(
	    input_sequence, CuboidMultiIndexSequence({2}),
	    ListMultiIndexSequence({MultiIndex({0, 1}), MultiIndex({2, 1})}));
	EXPECT_TRUE(connector_section);
	EXPECT_EQ(*connector_section, connector_section_expectation);

	EXPECT_TRUE(connector.get_input_ports().empty());
	EXPECT_TRUE(connector.get_output_ports().empty());
}
