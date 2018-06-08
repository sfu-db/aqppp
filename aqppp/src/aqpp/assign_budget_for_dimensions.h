/* distribute MTL_POINTS into different dimension.
*/
#pragma once
#include <vector>
#include"common_content.h"
#include "hill_climbing.h"

namespace aqppp
{
	class AssignBudgetForDimensions
	{
	private:
		struct Par
		{
			int ALL_MTL_POINTS;
			int PIECE_NUM;
			bool UNIFORM;
			int SAMPLE_ROW_NUM;
			double SAMPLE_RATE;
			double CI_INDEX;
			int CLIMB_MAX_ITER;
			bool CLIMB_INIT_DISTINCT_EVEN;
		} PAR;

	public:
		AssignBudgetForDimensions(Settings settings, bool uniform);

		struct Point
		{
			int mtl_num;
			double err;
			double a; //the par. for curve between current point and next_point. Func: y=a/sqrt(x)+b.
			double b;//the par. for curve between current point and next_point. Func: y=a/sqrt(x)+b.
			Point() {}
			Point(int k, double e) :mtl_num(k), err(e) {}
		};

		struct ME
		{
			int dim_id = -1;
			int mtl_num;
			double err;
			ME() {}
			ME(int id, int p, double e) :dim_id(id), mtl_num(p), err(e) {}
		};

		struct ErrorComparatorDesc
		{
			bool operator()(ME a, ME b)   //sort by err desc.
			{
				return a.err > b.err;
			}
		};
		void AssignBudget(const std::vector<std::vector<CA>> &casample, std::vector<int> &o_MTL_dist_result, int ADJUST_WAY = CLB_ADJUST_MINVAR);

		void AssignBudget(const std::vector<std::vector<CA>> &casample, const std::vector<std::vector<Point>> &err_profiles, std::vector<int> &o_MTL_dist_result);

		/*given  err, find the min mtl_num that related err<=given err.
		return mtl_num.
		If not found, return -1;
		Can use binary_search to speed up.
		*/
		int FindMinPointsNum(const std::vector<Point> &err_profile, double err);


		void BuildErrorProfile1Dim(const std::vector<CA> &cur_col, std::vector<Point> &o_err_profile, int ADJUST_WAY = CLB_ADJUST_MINVAR);

		double InferErrorByProfile(std::vector<Point> err_profile, int mtl_num);

		double BinsearchMaxError(const std::vector<std::vector<Point>> &err_profiles, std::vector<ME> &left_mtl, std::vector<ME> &right_mtl, bool &right_over_int);

		void BuildErrorProfiles(std::vector<std::vector<CA>> casample, std::vector<std::vector<Point>> &o_err_profiles, int ADJUST_WAY = CLB_ADJUST_MINVAR);

	};
}
