//#include "stdafx.h"
#include "bootstrap.h"
namespace aqppp
{
		double Bootstrap::StatInterface(const std::vector<double> &data, std::string cpt_fun, double percentile)
		{
			if (cpt_fun == "sum") return Tool::get_sum(data);
			else if (cpt_fun == "count") return data.size();
			else if (cpt_fun == "avg") return Tool::get_avg(data);
			else if (cpt_fun == "variance") return Tool::get_var(data);
			else if (cpt_fun == "percentile") return Tool::get_percentile(data, percentile);
			return -1;
		}

		void Bootstrap::FilterData(const std::vector<std::vector<double>> &org_data, const std::vector<Condition> &query_cond, std::vector<double> &odata)
		{
			odata = std::vector<double>();
			for (int ri=0;ri<org_data[0].size();ri+=1)
			{
				bool meet = true;
				for (int ci=0;ci<query_cond.size();ci++)
				{
					if (DoubleLess(org_data[ci + 1][ri], query_cond[ci].lb) || DoubleGreater(org_data[ci + 1][ri], query_cond[ci].ub))
					{
						meet = false;
						break;
					}
				}
				if (meet) odata.push_back(org_data[0][ri]);
			}
			return;
		}


		void Bootstrap::PredicateTrans(const std::vector<std::vector<double>> &org_data, const std::vector<Condition> &query_cond, std::vector<double> &odata)
		{
			odata = std::vector<double>(org_data[0].size());
			for (int ri = 0;ri<org_data[0].size();ri += 1)
			{
				bool meet = true;
				for (int ci = 0;ci<query_cond.size();ci++)
				{
					if (DoubleLess(org_data[ci + 1][ri], query_cond[ci].lb) || DoubleGreater(org_data[ci + 1][ri], query_cond[ci].ub))
					{
						meet = false;
						break;
					}
				}
				odata[ri] = (meet ? org_data[0][ri] : 0);
			}
			return;
		}


		double Bootstrap::EstimateCondition(const std::vector<std::vector<double>> &sample,const std::vector<std::vector<Condition>> &query_condition, std::string est_fun)
		{
			assert(query_condition.size() == 1 || query_condition.size() == 2);

			std::vector<Condition> spl_q = query_condition[0];
			std::vector<double> spl_filtered_sample = std::vector<double>();
			FilterData(sample, spl_q, spl_filtered_sample);
			double spl_est_value = StatInterface(spl_filtered_sample, est_fun);

			double pre_est_value = 0;
			if (query_condition.size() == 2)
			{
				std::vector<Condition> pre_q = query_condition[1];
				std::vector<double> pre_filtered_sample = std::vector<double>();
				FilterData(sample, pre_q, pre_filtered_sample);
				pre_est_value = StatInterface(pre_filtered_sample, est_fun);
			}
			return spl_est_value - pre_est_value;
		}

		std::vector<double> Bootstrap::BootstrapEstimator(const std::vector<std::vector<double>> &sample,const double sample_rate, const std::vector<std::vector<Condition>> &query_condition, std::string est_fun, int resample_num, double CI_level, int rand_seed)
		{
			assert(query_condition.size() == 1 || query_condition.size() == 2);

			double est_value = EstimateCondition(sample, query_condition, est_fun);

			std::mt19937 rand_gen(rand_seed);
			std::vector<double> diff = std::vector<double>();
			int len = sample[0].size();
			for (int ri = 0;ri < resample_num;ri++)
			{
				std::vector<std::vector<double>> re_sample = std::vector<std::vector<double>>(sample.size());
				for (int i = 0;i < re_sample.size();i++)
				{
					re_sample[i] = std::vector<double>(sample[0].size());
				}

				for (int i = 0;i < len;i++)
				{
					int po = rand_gen() % len;
					for (int ci = 0;ci < sample.size();ci++)
					{
						re_sample[ci][i] = sample[ci][po];
					}
				}

				double re_est_value= EstimateCondition(re_sample, query_condition, est_fun);
				double cur_diff = re_est_value - est_value;
				diff.push_back(cur_diff);
			}
			sort(diff.begin(), diff.end());
			double left_per = (1.0 - CI_level) / 2.0;
			double right_per = 1.0 - left_per;
			double CI_left_point = 0 - Tool::get_percentile(diff, right_per); //should add est_value outside this func.
			double CI_right_point =0 - Tool::get_percentile(diff, left_per);

			std::vector<double> res = std::vector<double>();
			res.push_back(est_value);
			res.push_back(CI_left_point);
			res.push_back(CI_right_point);
			if (est_fun == "sum" || est_fun == "count")
			{
				for (int i = 0;i < res.size();i++)
					res[i] /= sample_rate;
			}

			return res;
		}
}