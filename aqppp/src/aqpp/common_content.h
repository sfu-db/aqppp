#ifndef COMMON_CONTENT
#define COMMON_CONTENT

//#include<mysql.h>
#include<string>
#include<vector>
#include<map>
#include<unordered_map>

namespace aqppp {
	typedef std::vector<std::unordered_map<std::string, int>> DistId;
	struct Settings {
		std::string DB_NAME = "skew_s100_z2.dbo";
		std::string TABLE_NAME = "lineitem";
		std::string SAMPLE_NAME = "sample";
		std::string SUB_SAMPLE_NAME = "sub_sample";
		bool CREATE_DB_SAMPLES = true;
		double SAMPLE_RATE =0.0005;
		double SUB_SAMPLE_RATE = 0.1;
		std::string AGGREGATE_NAME = "L_EXTENDEDPRICE";
		std::vector<std::string> CONDITION_NAMES = { "L_ORDERKEY","L_SUPPKEY" };
		int GROUPBY_LEN = 0;//# attributes in CONDITION_NAMES are group by attributes. the last GROUPBY_LEN cols in CONDITION_NAMES should be group by attributes.
		double CI_INDEX =1.96;
		int SAMPLE_ROW_NUM = -1;
		int NF_MAX_ITER = 1000;
		int ALL_MTL_POINTS = 50000;
		int EP_PIECE_NUM = 20;
		bool INIT_DISTINCT_EVEN = false;
		int RAND_SEED = 1;
	};

	const double DB_EQUAL_THRESHOLD = 0.0000000001;
	struct Condition
	{
		double lb = 0.0, ub = 0.0;
		int lb_id = 0, ub_id = 0;
	};
	std::ostream& operator << (std::ostream& os, const Condition& rhs);

	struct CA {
		double condition_value = 0.0;
		double sum = 0.0;//sum of aggregate value in this condition.
		double sqrsum = 0.0;//sqrsum of aggregate value in this condition.
		int count = 0; //CA is the distinct value, count is the number of tuples it has.
		int id = 0;//its order sorted by condition value, started with 0.
	};

	struct Group
	{
		std::vector<std::vector<double>> sample, small_sample;
		double sample_rate, small_sample_rate;
	};

	bool CA_compare(CA a, CA b);

	bool DoubleEqual(double a, double b);

	bool DoubleLeq(double a, double b);

	bool DoubleGeq(double a, double b);

	bool DoubleGreater(double a, double b);

	bool DoubleLess(double a, double b);


	template <class T>
	static void HashCombine(std::size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}


	struct VectorHash {
		template <class T>
		std::size_t operator () (const std::vector<T> &vec) const {
			std::size_t seed = 0;
			for (T ele : vec)
			{
				auto h1 = std::hash<T>{}(ele);
				HashCombine(seed, h1);
			}
			return seed;
		}
	};

	
	struct IntVectorEqual
	{
		bool operator () (const std::vector<int>  &lhs, const std::vector<int> &rhs) const
		{
			if (lhs.size() != rhs.size()) return false;
			for (int i = 0; i < lhs.size(); i++) if (lhs[i]!=rhs[i]) return false;
			return true;
		}
	};


	typedef std::unordered_map<std::vector<int>, double, VectorHash,IntVectorEqual> MTL_STRU;

}

#endif // !COMMON_CONTENT
