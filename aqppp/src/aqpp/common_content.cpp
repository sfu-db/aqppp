//#include"stdafx.h"
#include"common_content.h"

namespace aqppp {

	std::ostream& operator << (std::ostream& os, const Condition& rhs) {
		os << "lb_id=" << rhs.lb_id << " ub_id=" << rhs.ub_id << " lb=" << rhs.lb << " ub=" << rhs.ub;
		return os;
	}
	bool CA_compare(const CA a, const CA b)
	{
		return DoubleLess(a.condition_value, b.condition_value);
	}

	bool DoubleEqual(double a, double b)
	{
		return fabs(a - b) <= DB_EQUAL_THRESHOLD;
	}
	bool DoubleLeq(double a, double b)
	{
		return a - b <= DB_EQUAL_THRESHOLD;
	}
	bool DoubleGeq(double a, double b)
	{
		return a - b >= -DB_EQUAL_THRESHOLD;
	}

	bool DoubleGreater(double a, double b)
	{
		return a - b > DB_EQUAL_THRESHOLD;
	}
	bool DoubleLess(double a, double b)
	{
		return a - b < -DB_EQUAL_THRESHOLD;
	}
	

}