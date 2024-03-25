#include "grenade/vx/network/abstract/printable.h"

#include <sstream>
#include <gtest/gtest.h>


struct DummyPrintable : public grenade::vx::network::Printable
{
	int value;

	DummyPrintable(int value) : value(value) {}

protected:
	virtual std::ostream& print(std::ostream& os) const override
	{
		return os << "DummyPrintable(" << value << ")";
	}
};


TEST(Printable, General)
{
	DummyPrintable dummy_5(5);

	std::stringstream ss_operator;
	ss_operator << dummy_5;
	EXPECT_EQ(ss_operator.str(), "DummyPrintable(5)");
}
