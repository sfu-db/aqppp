/* distribute MTL_POINTS into different dimension.
*/
#pragma once
//#include"stdafx.h"
#include "assign_budget_for_dimensions.h"

namespace aqppp
{
	AssignBudgetForDimensions::AssignBudgetForDimensions(Settings settings, bool uniform)
	{
		this->PAR.SAMPLE_RATE = settings.SAMPLE_RATE;
		this->PAR.UNIFORM = uniform;
		this->PAR.ALL_MTL_POINTS = settings.ALL_MTL_POINTS;
		this->PAR.PIECE_NUM = settings.EP_PIECE_NUM;
		this->PAR.SAMPLE_ROW_NUM = settings.SAMPLE_ROW_NUM;
		this->PAR.CI_INDEX = settings.CI_INDEX;
		this->PAR.CLIMB_MAX_ITER = settings.NF_MAX_ITER;
		this->PAR.CLIMB_INIT_DISTINCT_EVEN = settings.INIT_DISTINCT_EVEN;
	}

	void AssignBudgetForDimensions::AssignBudget(const std::vector<std::vector<CA>> &casample, std::vector<int> &o_MTL_dist_result, int ADJUST_WAY)
	{
		if (casample.size() == 1)
		{
			o_MTL_dist_result = std::vector<int>();
			o_MTL_dist_result.push_back(this->PAR.ALL_MTL_POINTS);
			return;
		}
		std::vector<std::vector<Point>> err_profiles;
		BuildErrorProfiles(casample, err_profiles, ADJUST_WAY);
		AssignBudget(casample, err_profiles, o_MTL_dist_result);
	}

	void AssignBudgetForDimensions::AssignBudget(const std::vector<std::vector<CA>> &casample, const std::vector<std::vector<Point>> &err_profiles, std::vector<int> &o_MTL_dist_result)
	{
		const int DIM = casample.size();
		double init_max_err = 0;
		std::vector<ME> left_mtl_res = std::vector<ME>(DIM + 1);
		std::vector<ME> right_mtl_res = std::vector<ME>(DIM + 1);
		for (int i = 0; i < DIM; i++)
		{
			if (err_profiles[i][0].err > init_max_err) init_max_err = err_profiles[i][0].err;
			left_mtl_res[i] = ME(i, err_profiles[i][0].mtl_num, err_profiles[i][0].err);
			int len = err_profiles[i].size();
			right_mtl_res[i] = ME(i, err_profiles[i][len - 1].mtl_num, err_profiles[i][len - 1].err);
		}

		left_mtl_res[DIM] = ME(-1, 1, init_max_err);
		right_mtl_res[DIM] = ME(-1, -1, 0.0);
		bool right_over_int = true;
		double max_err = BinsearchMaxError(err_profiles, left_mtl_res, right_mtl_res, right_over_int);
		//assert(right_over_int == false);
		std::vector<ME> mtl_res;
		if (right_mtl_res[DIM].mtl_num <= this->PAR.ALL_MTL_POINTS) mtl_res = right_mtl_res;
		else mtl_res = left_mtl_res;
		for (int i = 0; i < DIM; i++)
		{
			mtl_res[i].dim_id = i;
		}
		mtl_res[DIM].dim_id = -1;

		int act_product = mtl_res[DIM].mtl_num;
		// sort mtl_res by err desc. This is to make col with big err to have as much as more mtl points.
		sort(mtl_res.begin(), mtl_res.end(), ErrorComparatorDesc());
		o_MTL_dist_result = std::vector<int>(DIM);


		for (int ci = 0; ci <DIM + 1; ci++)
		{
			int dim_id = mtl_res[ci].dim_id;
			if (dim_id == -1) continue;
			int cur_min_mtl = mtl_res[ci].mtl_num;

			if (act_product % cur_min_mtl != 0) std::cout << "not div integer wrong" << std::endl;

			int LB_other = act_product / cur_min_mtl; //product of mtl in other dims. 
			o_MTL_dist_result[dim_id] = min((int)casample[dim_id].size(), this->PAR.ALL_MTL_POINTS / LB_other);   //cur allowed max mtl.
			act_product = act_product / cur_min_mtl*o_MTL_dist_result[dim_id];

			//cout << "dim: " << dim_id << " " << o_MTL_dist_result[dim_id] << " " << mtl_res[ci].mtl_num << " " << mtl_res[ci].err << endl;
		}
		return;
	}

	/*given  err, find the min mtl_num that related err<=given err.
	return mtl_num.
	If not found, return -1;
	Can use binary_search to speed up.
	*/
	int AssignBudgetForDimensions::FindMinPointsNum(const std::vector<Point> &err_profile, double err)
	{
		int pos = -1;
		if (DoubleLeq(err_profile[0].err, err)) return err_profile[0].mtl_num;
		if (DoubleGeq(err_profile[err_profile.size() - 1].err, err)) return err_profile[err_profile.size() - 1].mtl_num;
		for (int i = 0; i < err_profile.size() - 1; i++)
		{
			if (DoubleGeq(err_profile[i].err, err) && DoubleLeq(err_profile[i + 1].err, err))
			{
				pos = i;
				break;
			}
		}
		assert(pos != -1);

		double a = err_profile[pos].a;
		double b = err_profile[pos].b;
		long int x = pow(a / (err - b), 2.0);
		double real_err = a / sqrt(x) + b;
		int max_mtl = err_profile[err_profile.size() - 1].mtl_num;
		if (DoubleGreater(real_err, err) && x < max_mtl) x++;
		return x;
	}


	void AssignBudgetForDimensions::BuildErrorProfile1Dim(const std::vector<CA> &cur_col, std::vector<Point> &o_err_profile, int ADJUST_WAY)
	{
		int k_min = 1;
		//cout << "sample rate" <<this->PAR.SAMPLE_RATE << endl;
		HillClimbing climb_method = HillClimbing(this->PAR.SAMPLE_ROW_NUM, this->PAR.SAMPLE_RATE, this->PAR.CI_INDEX, this->PAR.CLIMB_MAX_ITER, this->PAR.CLIMB_INIT_DISTINCT_EVEN);

		double err_max = climb_method.ChoosePoints1Dim(cur_col, k_min, std::vector<CA>(), 0, 0, CLB_EVEN_INIT, ADJUST_WAY);

		int k_max = cur_col.size();
		double y_gap = (1.0 - 1.0 / sqrt((double)k_max)) / (double)this->PAR.PIECE_NUM;
		double err_min = 0.0;
		o_err_profile = std::vector<Point>();
		o_err_profile.push_back(Point(k_min, err_max));
		for (int pi = 1; pi < this->PAR.PIECE_NUM; pi++)
		{
			double cur_y = 1.0 - pi*y_gap;
			int cur_k = (1.0 / cur_y)*(1.0 / cur_y);
			while (cur_k <= o_err_profile[pi - 1].mtl_num && cur_k < k_max) cur_k++;
			double cur_err = climb_method.ChoosePoints1Dim(cur_col, cur_k, std::vector<CA>());
			if (cur_err > o_err_profile[pi - 1].err)
			{
				//cout << "err profile not decrease in point" << pi << " " << o_err_profile[pi - 1].err << " " << cur_err << endl;
				cur_err = o_err_profile[pi - 1].err;
			}

			o_err_profile.push_back(Point(cur_k, cur_err));
			if (cur_k >= k_max) break;
		}
		if (k_max != o_err_profile[o_err_profile.size() - 1].mtl_num) o_err_profile.push_back(Point(k_max, 0.0));
		assert(o_err_profile.size() == this->PAR.PIECE_NUM + 1);


		/*-------------build curve--------------*/
		for (int i = 0; i < o_err_profile.size() - 1; i++)
		{
			double x1 = o_err_profile[i].mtl_num;
			double x2 = o_err_profile[i + 1].mtl_num;
			double y1 = o_err_profile[i].err;
			double y2 = o_err_profile[i + 1].err;
			assert(x1*x2 > 0 && x1 != x2);
			o_err_profile[i].a = (y1 - y2) / (1.0 / sqrt(x1) - 1.0 / sqrt(x2));
			o_err_profile[i].b = y1 - o_err_profile[i].a / sqrt(x1);

			/*
			double c2 = (1.0 / x1 + 1.0 / x2);    //coefficient for a^2 term.
			double c1 = -2 * (y1 / sqrt(x1) + y2 / sqrt(x2));  //coeffience for a term.
			double c0 = y1*y1 + y2*y2; //coefficience for constanr term.
			assert(fabs(c2-0)<Algo_settings.DB_EQUAL_THRESHOLD);
			o_err_profile[i].a = -c1 / (2.0 * c2);
			o_err_profile[i].mse = c0 - c1*c1 / (4 * c2);
			*/

		}
		o_err_profile[o_err_profile.size() - 1].a = -1;
		o_err_profile[o_err_profile.size() - 1].b = -1;
		/*------------finish build curve*/

		return;
	}



	double AssignBudgetForDimensions::InferErrorByProfile(std::vector<Point> err_profile, int mtl_num)
	{
		int pos = -1;
		if (mtl_num >= err_profile[err_profile.size() - 1].mtl_num) return err_profile[err_profile.size() - 1].err;
		for (int i = 0; i<err_profile.size() - 1; i++)
			if (mtl_num >= err_profile[i].mtl_num && mtl_num <= err_profile[i + 1].mtl_num)
			{
				pos = i;
				break;
			}
		assert(pos != -1);
		double a = err_profile[pos].a;
		double b = err_profile[pos].b;
		double x = mtl_num;
		double err = a / sqrt(x) + b;
		return err;

	}
	double AssignBudgetForDimensions::BinsearchMaxError(const std::vector<std::vector<Point>> &err_profiles, std::vector<ME> &left_mtl, std::vector<ME> &right_mtl, bool &right_over_int)
	{
		const int DIM = err_profiles.size();
		double left_err = left_mtl[DIM].err;
		double right_err = right_mtl[DIM].err;
		int L_PRODUCT = left_mtl[DIM].mtl_num;
		int R_PRODUCT = right_mtl[DIM].mtl_num;
		if ((!right_over_int) && L_PRODUCT == R_PRODUCT) return right_err;

		std::vector<ME> mid_mtl = std::vector<ME>(DIM + 1);
		double mid_err = (left_err + right_err) / 2.0;
		long int MID_PRODUCT = 1;
		bool mid_over_int = false;
		double real_mid_err = 0;
		for (int i = 0; i < DIM; i++)
		{

			int tp = FindMinPointsNum(err_profiles[i], mid_err);
			mid_mtl[i].mtl_num = tp;
			mid_mtl[i].err = InferErrorByProfile(err_profiles[i], tp);
			if (mid_mtl[i].err > real_mid_err) real_mid_err = mid_mtl[i].err;
			if (INT_MAX / MID_PRODUCT < tp) mid_over_int = true;
			else MID_PRODUCT = MID_PRODUCT*tp;
		}
		if (!mid_over_int) mid_mtl[DIM].mtl_num = MID_PRODUCT;
		else mid_mtl[DIM].mtl_num = -1;
		mid_mtl[DIM].err = real_mid_err;
		if ((!mid_over_int) && (this->PAR.ALL_MTL_POINTS >MID_PRODUCT))
		{
			if ((!mid_over_int) && MID_PRODUCT == L_PRODUCT) return mid_err; //in case the result of next iter is the same as this iter.
			left_mtl = mid_mtl;
			return BinsearchMaxError(err_profiles, left_mtl, right_mtl, right_over_int);
		}

		if ((!right_over_int) && R_PRODUCT == MID_PRODUCT) return left_err; //in case the result of next iter is the same as this iter.

		right_mtl = mid_mtl;
		right_over_int = mid_over_int;
		return BinsearchMaxError(err_profiles, left_mtl, right_mtl, right_over_int);
	}

	void AssignBudgetForDimensions::BuildErrorProfiles(std::vector<std::vector<CA>> casample, std::vector<std::vector<Point>> &o_err_profiles, int ADJUST_WAY)
	{
		const int DIM = casample.size();
		o_err_profiles = std::vector<std::vector<Point>>(DIM);
		for (int di = 0; di < DIM; di++)
		{
			BuildErrorProfile1Dim(casample[di], o_err_profiles[di], ADJUST_WAY);
		}
		return;
	}

}
