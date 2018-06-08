#pragma once
//#include"stdafx.h"
#include "query_template.h"
namespace aqppp
{
	double QueryTemplate::ComputeDifferenceForAvg(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands, const std::vector<Condition>& mtl_choices, double CI_INDEX, double ufactor)
	{
		double sum = 0;
		double sum2 = 0;
		const int ROW_NUM = sample[0].size();
		const int USER_CONDITION_DIM = user_demands.size();

		double mpred = 0.0;
		for (int ri = 0;ri < ROW_NUM;ri++)
		{
			boolean mmeet = true;
			for (int ci = 1; ci < USER_CONDITION_DIM + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLeq(con_data, mtl_choices[ci - 1].lb) || DoubleGreater(con_data, mtl_choices[ci - 1].ub))
				{
					mmeet = false;
					break;
				}
			}
			if (mmeet) mpred += 1.0;
		}
		double mfactor = mpred > 0 ? ROW_NUM / mpred : 0;
		for (int ri = 0; ri < ROW_NUM; ri++)
		{
			bool umeet = true;
			bool mmeet = true;
			for (int ci = 1; ci < USER_CONDITION_DIM + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLess(con_data, user_demands[ci - 1].lb) || DoubleGreater(con_data, user_demands[ci - 1].ub))
				{
					umeet = false;
				}
				if (DoubleLeq(con_data, mtl_choices[ci - 1].lb) || DoubleGreater(con_data, mtl_choices[ci - 1].ub))
				{
					mmeet = false;
				}
			}
			double udata = 0;
			double mdata = 0;
			if (umeet) udata = sample[0][ri] * ufactor;
			if (mmeet) mdata = sample[0][ri] * mfactor;
			double data = udata - mdata;
			sum += data;
			sum2 += data*data;
		}
		double variance = 0;
		variance = sum2 / ROW_NUM - (sum / ROW_NUM)*(sum / ROW_NUM);
		double ci = CI_INDEX*sqrt(variance / ROW_NUM);
		return ci;
	}

	double QueryTemplate::ComputeUfactor(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands)
	{
		const int ROW_NUM = sample[0].size();
		const int USER_CONDITION_DIM = user_demands.size();
		double upred = 0.0;
		for (int ri = 0;ri < ROW_NUM;ri++)
		{
			boolean umeet = true;
			for (int ci = 1; ci < USER_CONDITION_DIM + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLess(con_data, user_demands[ci - 1].lb) || DoubleGreater(con_data, user_demands[ci - 1].ub))
				{
					umeet = false;
				}
			}
			if (umeet) upred += 1.0;
		}
		double ufactor = upred > 0 ? ROW_NUM / upred : 0;
		return ufactor;
	}

	void QueryTemplate::FinalComputeDifferenceForAvg(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands, const std::vector<Condition>& mtl_choices, double ufactor, double CI_INDEX, std::vector<std::pair<double, double>>& final_diff_res)
	{
		double diffSum = 0;//h for hybrid
		double diffSum2 = 0;
		double ssum = 0;//s for sampling
		double ssum2 = 0;
		const int ROW_NUM = sample[0].size();
		const int USER_CONDITION_DIM = user_demands.size();

		double mpred = 0.0;
		for (int ri = 0;ri < ROW_NUM;ri++)
		{
			boolean mmeet = true;
			for (int ci = 1; ci < USER_CONDITION_DIM + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLeq(con_data, mtl_choices[ci - 1].lb) || DoubleGreater(con_data, mtl_choices[ci - 1].ub))
				{
					mmeet = false;
					break;
				}
			}
			if (mmeet) mpred += 1.0;
		}
		double mfactor = mpred > 0 ? ROW_NUM / mpred : 0;
		for (int ri = 0; ri < ROW_NUM; ri++)
		{
			bool umeet = true;
			bool mmeet = true;
			for (int ci = 1; ci < USER_CONDITION_DIM + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLess(con_data, user_demands[ci - 1].lb) || DoubleGreater(con_data, user_demands[ci - 1].ub))
				{
					umeet = false;
				}
				if (DoubleLeq(con_data, mtl_choices[ci - 1].lb) || DoubleGreater(con_data, mtl_choices[ci - 1].ub))
				{
					mmeet = false;
				}
			}

			double udata = 0;
			double mdata = 0;
			if (umeet) udata = sample[0][ri] * ufactor;
			if (mmeet) mdata = sample[0][ri] * mfactor;
			double data = udata - mdata;
			diffSum += data; //diff is the difference.
			diffSum2 += data*data;
			ssum += udata; //s for sampling.
			ssum2 += udata*udata;
		}

		double diffVar = diffSum2 / ROW_NUM - (diffSum / ROW_NUM)*(diffSum / ROW_NUM);
		double diffCI = CI_INDEX*sqrt(diffVar / ROW_NUM);
		double svariance = ssum2 / ROW_NUM - (ssum / ROW_NUM)*(ssum / ROW_NUM);
		double sci = CI_INDEX*sqrt(svariance / ROW_NUM);
		double sAvg = ssum / ROW_NUM;
		double diffAvg = diffSum / ROW_NUM;
		final_diff_res = std::vector<std::pair<double, double>>();
		final_diff_res.push_back(std::pair<double, double>(sAvg, sci));
		final_diff_res.push_back(std::pair<double, double>(diffAvg, diffCI));
		return;
	}


	void QueryTemplate::FillPointChoicesForAvg(const int col, double CI_INDEX, double ufactor, const std::vector<std::vector<double>>& small_sample, const std::vector<RGs>& all_rgs, const std::vector<Condition>& user_demands, double &o_min_ci, std::vector<Condition >& mtl_choices, std::vector<Condition>& final_mtl_choices)
	{
		if (col >= all_rgs.size())
		{
			//to do something.

			double ci = ComputeDifferenceForAvg(small_sample, user_demands, mtl_choices, CI_INDEX, ufactor);
			if (ci < o_min_ci)
			{
				o_min_ci = ci;
				final_mtl_choices = mtl_choices;
			}
			return;
		}

		const int CUR_SIZE = all_rgs[col].size();
		for (int i = 0; i < CUR_SIZE; i++)
		{
			mtl_choices[col] = all_rgs[col][i];
			FillPointChoicesForAvg(col + 1, CI_INDEX, ufactor, small_sample, all_rgs, user_demands, o_min_ci, mtl_choices, final_mtl_choices);
		}
	}

	std::pair<double, double> QueryTemplate::SamplingForAvg(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands, double ufactor, double CI_INDEX)
	{
		double diffSum = 0;//h for hybrid
		double diffSum2 = 0;
		double ssum = 0;//s for sampling
		double ssum2 = 0;
		const int ROW_NUM = sample[0].size();
		const int USER_CONDITION_DIM = user_demands.size();
		for (int ri = 0; ri < ROW_NUM; ri++)
		{
			bool umeet = true;
			for (int ci = 1; ci < USER_CONDITION_DIM + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLess(con_data, user_demands[ci - 1].lb) || DoubleGreater(con_data, user_demands[ci - 1].ub))
				{
					umeet = false;
				}
			}
			double udata = 0;
			if (umeet) udata = sample[0][ri] * ufactor;
			ssum += udata; //s for sampling.
			ssum2 += udata*udata;
		}

		double svariance = ssum2 / ROW_NUM - (ssum / ROW_NUM)*(ssum / ROW_NUM);
		double sci = CI_INDEX*sqrt(svariance / ROW_NUM);
		double sAvg = ssum / ROW_NUM;
		double diffAvg = diffSum / ROW_NUM;
		return std::pair<double, double>(sAvg, sci);
	}

	std::pair<double, double> QueryTemplate::AqppQueryForAvg(int query_id, FILE* info_file, Settings & PAR, const std::vector<std::vector<double>> &sample, const std::vector<std::vector<double>> &small_sample, const std::vector<std::vector<CA>>& mtl_points, const MTL_STRU& mtlResAvgSum, const MTL_STRU& mtlResAvgCount, const std::vector<Condition>& user_demands)
	{
		const int DIM = PAR.CONDITION_NAMES.size();
		std::vector<RGs> all_rgs = std::vector<RGs>();

		double ufactor = ComputeUfactor(sample, user_demands);

		Aqpp aqpp_oper = Aqpp(PAR.SAMPLE_ROW_NUM, PAR.SAMPLE_RATE, PAR.CI_INDEX);
		aqpp_oper.GenAllRanges(mtl_points, user_demands, all_rgs);

		for (int i = 0; i < all_rgs.size(); i++)
		{
			if (all_rgs[i].size() < 1)
			{
				fprintf(info_file, "cannot gen rg, and return sampling result. query_id:%d conditions:", query_id);
				for (auto ele : user_demands)
				{
					fprintf(info_file, "(%f,%f) ", ele.lb, ele.ub);
				}
				fprintf(info_file, "\n");
				return SamplingForAvg(sample, user_demands, ufactor, PAR.CI_INDEX);
			}
		}

		std::vector<Condition> mtl_choices = std::vector<Condition>(all_rgs.size());
		std::vector<Condition> final_mtl_choices = std::vector<Condition>();
		double min_ci = DBL_MAX;
		FillPointChoicesForAvg(0, PAR.CI_INDEX, ufactor, small_sample, all_rgs, user_demands, min_ci, mtl_choices, final_mtl_choices);



		//const vector<vector<double>>&sample, const vector<Condition>& user_demands, const vector<Condition>& mtl_choices, double ufactor, double CI_INDEX, vector<pair<double, double>>& final_diff_res

		std::vector<std::pair<double, double>> final_diff_res = std::vector<std::pair<double, double>>();
		FinalComputeDifferenceForAvg(sample, user_demands, final_mtl_choices, ufactor, PAR.CI_INDEX, final_diff_res);
		std::pair<double, double> spl_est_ci = final_diff_res[0];
		std::pair<double, double> hybrid_est_ci = final_diff_res[1];
		if (spl_est_ci.second < hybrid_est_ci.second) return spl_est_ci;


		std::vector<int> indsSum = std::vector<int>(DIM);
		double deltaSum = 0;
		aqpp_oper.ComputeMquery(0, indsSum, mtlResAvgSum, final_mtl_choices, deltaSum, 0);
		std::vector<int> indsCount = std::vector<int>(DIM);
		double deltaCount = 0;
		aqpp_oper.ComputeMquery(0, indsCount, mtlResAvgCount, final_mtl_choices, deltaCount, 0);

		double deltaAvg = deltaCount > 0 ? deltaSum / deltaCount : 0;

		hybrid_est_ci.first += deltaAvg;
		return hybrid_est_ci;
	}



	double QueryTemplate::ComputeDataCubeForAvg(Settings &PAR, const std::vector<std::vector<CA>>& mtl_points, SQLHANDLE &sqlstatementhandle, MTL_STRU& o_mtl_data4sum, MTL_STRU& o_mtl_data4count)
	{
		double start_time = clock();
		double trans_time = 0;
		const int CONDITION_DIM = PAR.CONDITION_NAMES.size();
		o_mtl_data4sum = MTL_STRU();
		o_mtl_data4count = MTL_STRU();
		std::string query = SqlInterface::ReadTableStr(PAR.DB_NAME, PAR.TABLE_NAME, PAR.AGGREGATE_NAME, PAR.CONDITION_NAMES);
		SqlInterface::SqlQuery(query, sqlstatementhandle);

		std::vector<int> indx = std::vector<int>(CONDITION_DIM);
		while (SQLFetch(sqlstatementhandle) == SQL_SUCCESS)
		{
			double acc_data=SqlInterface::Column2Numeric(sqlstatementhandle, 0, PAR.AGGREGATE_NAME);
			for (int ci = 1; ci < CONDITION_DIM + 1; ci++) //note sample dim is equal to COL_NUM+1.
			{
				double st = clock();
				double cond_data = SqlInterface::Column2Numeric(sqlstatementhandle, ci, PAR.CONDITION_NAMES[ci - 1]);
				trans_time += ((clock() - st) / CLOCKS_PER_SEC);
				CA cur_ca = CA();
				cur_ca.condition_value = cond_data;
				int indi = lower_bound(mtl_points[ci - 1].begin(), mtl_points[ci - 1].end(), cur_ca, CA_compare) - mtl_points[ci - 1].begin();
				indx[ci - 1] = indi;
			}

			bool out_range = false;
			/*in case the processing data is greater than the last mtl data.*/
			for (int i = 0; i < CONDITION_DIM; i++)
			{
				if (indx[i] >= mtl_points[i].size())
				{
					out_range = true;
					break;
				};
			}
			if (out_range) continue;

			if (o_mtl_data4sum.count(indx) == 1)
			{
				o_mtl_data4sum[indx] += acc_data;
			}
			else
			{
				o_mtl_data4sum.insert({ indx,acc_data });
			}

			if (o_mtl_data4count.count(indx) == 1)
			{
				o_mtl_data4count[indx] += 1;
			}
			else
			{
				o_mtl_data4count.insert({ indx,1 });
			}

		}

		double run_time = (clock() - start_time) / CLOCKS_PER_SEC-trans_time;
		return run_time;
	}
}
