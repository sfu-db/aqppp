#pragma once
#include<vector>
#include"common_content.h"
#include"aqpp.h"

namespace aqppp
{
	class QueryTemplate
	{
	public:
		typedef std::vector<Condition> RGs;

		static double ComputeDifferenceForAvg(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands, const std::vector<Condition>& mtl_choices, double CI_INDEX, double ufactor);

		static double ComputeUfactor(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands);

		static void FinalComputeDifferenceForAvg(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands, const std::vector<Condition>& mtl_choices, double ufactor, double CI_INDEX, std::vector<std::pair<double, double>>& final_diff_res);

		static void FillPointChoicesForAvg(const int col, double CI_INDEX, double ufactor, const std::vector<std::vector<double>>& small_sample, const std::vector<RGs>& all_rgs, const std::vector<Condition>& user_demands, double &o_min_ci, std::vector<Condition >& mtl_choices, std::vector<Condition>& final_mtl_choices);

		static std::pair<double, double> SamplingForAvg(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands, double ufactor, double CI_INDEX);

		static std::pair<double, double> AqppQueryForAvg(int query_id, FILE* info_file, Settings & PAR, const std::vector<std::vector<double>> &sample, const std::vector<std::vector<double>> &small_sample, const std::vector<std::vector<CA>>& mtl_points, const MTL_STRU& mtlResAvgSum, const MTL_STRU& mtlResAvgCount, const std::vector<Condition>& user_demands);

		static double ComputeDataCubeForAvg(Settings &PAR, const std::vector<std::vector<CA>>& mtl_points, SQLHANDLE &sqlstatementhandle, MTL_STRU& o_mtl_data4sum, MTL_STRU& o_mtl_data4count);
	};
}
