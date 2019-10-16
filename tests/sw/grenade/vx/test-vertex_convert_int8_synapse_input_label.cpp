#include <gtest/gtest.h>

#include "grenade/vx/vertex/convert_uint8_synapse_input_label.h"

using namespace grenade::vx;
using namespace grenade::vx::vertex;

TEST(ConvertInt8ToSynapseInputLabel, General)
{
	EXPECT_NO_THROW(ConvertInt8ToSynapseInputLabel(123));

	ConvertInt8ToSynapseInputLabel vertex(123);
	EXPECT_EQ(vertex.inputs().size(), 1);
	EXPECT_EQ(vertex.inputs().front().size, 123);
	EXPECT_EQ(vertex.output().size, 123);

	EXPECT_EQ(vertex.inputs().front().type, ConnectionType::Int8);
	EXPECT_EQ(vertex.output().type, ConnectionType::SynapseInputLabel);
}
