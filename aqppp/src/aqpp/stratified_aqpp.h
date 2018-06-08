#pragma once
#include<unordered_map>
#include<algorithm>
#include"common_content.h"
#include"sql_interface.h"
#include "tool.h"
#include "precompute.h"
#include"stratified_sampling.h"
#include <cstdio>
#include<iostream>
#include<vector>

namespace aqppp {
	class StratifiedAqpp {
	private:
		struct par
		{
			double CI_INDEX;
		} PAR;

	public:
		StratifiedAqpp(double ci_index);

		/*save the range(represente by index) of all columns in mtl data, each column is indivual and sorted by related demand*/
		typedef std::vector<Condition> RGs;

		std::pair<double, double> AqppSumQuery(int query_id, FILE* info_file, const std::vector<std::vector<double>> &sample,const std::vector<double> &sample_prob, const std::vector<std::vector<double>> &small_sample, const std::vector<double> &small_sample_prob, const std::vector<std::vector<CA>>& mtl_points, const MTL_STRU& mtl_res, const std::vector<Condition>& user_demands);


		//*********just use ub-lb rather ub-(lb-1) to get the range sum, because the mtl point is sparse. In this case, the result is sum(lb_id,ub_id], note '(]'.
		/*ans need to be inti 0.
		inds need to be init vector<int>(mtl_res.dim).
		mquery means mtl query.
		cpt given mtl_query in mtl result.
		mtl_choice[i] is the id of i-dimension mtl points in mtl result.
		*/
		void ComputeMquery(const int col, std::vector<int>& inds, const MTL_STRU& mtl_res, const std::vector<Condition>& mtl_choices, double& o_sum, int minus_value);


		/*
		used for function GenAllRanges
		*/
		Condition GenCondition(const std::vector<CA>& mtl_points, int lbid, int ubid);

		/*
		//used for func GenAllRanges.
		static void set_insert(set<int>& id_set,const int id,const vector<CA>& cur_col)
		{
		if (id == -1) return;
		if (cur_col[id].dual == false) id_set.insert(id);
		else id_set.insert(id + 1);
		return;
		}
		*/


		//consider when no mtl point beyond the first line, add point 0???    have add point 0 to the ids arr.
		//note that both Condition.lb and Condition.lb_id need got.
		//note that there maybe no some column cannot find range. 
		void GenAllRanges(const std::vector<std::vector<CA>>& mtl_points, const std::vector<Condition>& user_demands, std::vector<RGs>& o_all_rgs);

		//used for function FillMaterializeChoices.
		// should the row num is the real row num or distinct value of row num? 
		double ComputeDifference4Sum(const std::vector<std::vector<double>>&sample, const std::vector<double> &sample_prob, const std::vector<Condition>& user_demands, const std::vector<Condition>& mtl_choices);






		/*when got mtl choice, the final compute the diffrence to chose spling or hybrid*/
		void FinalComputeDifference(const std::vector<std::vector<double>>&sample, const std::vector<double> &sample_prob, const std::vector<Condition>& user_demands, const std::vector<Condition>& mtl_choices, std::vector<std::pair<double, double>>& final_diff_res);


		/*
		note that this function doesn't consider the case that don't find the mtl range.
		o_min_ci and mtl_choice should be init outside.
		It choice the mtl solution in small sample, and need to further
		*/
		void FillMaterializeChoices(const int col, const std::vector<std::vector<double>>& small_sample, const std::vector<double> &small_sample_prob, const std::vector<RGs>& all_rgs, const std::vector<Condition>& user_demands, double &o_min_ci, std::vector<Condition >& mtl_choices, std::vector<Condition>& final_mtl_choices);
	};
}

