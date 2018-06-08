#pragma once
#include<vector>
#include<random>
#include<algorithm>
#include<assert.h>
#include "common_content.h"
#include "tool.h"
namespace aqppp {
	class Bootstrap
	{
	public:
		double StatInterface(const std::vector<double> &data, std::string cpt_fun, double percentail = 0.5);
		double EstimateCondition(const std::vector<std::vector<double>> &sample, const std::vector<std::vector<Condition>> &query_condition, std::string est_fun);
		std::vector<double> BootstrapEstimator(const std::vector<std::vector<double>> &sample, const double sample_rate, const std::vector<std::vector<Condition>> &query_condition, std::string est_fun, int resample_num, double CI_level=0.95, int rand_seed=1);
		static void FilterData(const std::vector<std::vector<double>> &org_sample, const std::vector<Condition> &query_cond, std::vector<double> &osample);
		static void PredicateTrans(const std::vector<std::vector<double>> &org_data, const std::vector<Condition> &query_cond, std::vector<double> &odata);
	};
}
