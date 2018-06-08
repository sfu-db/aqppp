#pragma once
#include "aqpp.h"

namespace aqppp {

	Aqpp::Aqpp(int sample_row_num, double sample_rate, double ci_index)
	{
		this->PAR.SAMPLE_ROW_NUM = sample_row_num;
		this->PAR.SAMPLE_RATE = sample_rate;
		this->PAR.CI_INDEX = ci_index;
	}

	std::pair<double, double> Aqpp::AqppSumQuery(int query_id, FILE* info_file, const std::vector<std::vector<double>> &sample, const std::vector<std::vector<double>> &small_sample, const std::vector<std::vector<CA>>& mtl_points, const MTL_STRU& mtl_res, const std::vector<Condition>& user_demands)
	{
		const int DIM = mtl_points.size();
		std::vector<RGs> all_rgs = std::vector<RGs>();
		GenAllRanges(mtl_points, user_demands, all_rgs);

		//in case cannot find mtl range related to user query.
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
				return Sampling(this->PAR.SAMPLE_RATE, this->PAR.CI_INDEX).SamplingForSumQuery(sample, user_demands);
			}
		}

		std::vector<Condition> mtl_choices = std::vector<Condition>(all_rgs.size());
		std::vector<Condition> final_mtl_choices = std::vector<Condition>();
		double min_ci = FLT_MAX;


		FillMaterializeChoices(0, small_sample, all_rgs, user_demands, min_ci, mtl_choices, final_mtl_choices);

		
		std::vector<std::pair<double, double>> final_diff_res = std::vector<std::pair<double, double>>();
		FinalComputeDifference(sample, user_demands, final_mtl_choices, final_diff_res);
		//pair<double, double> spl_sum_ci = Sampling::SamplingForSumQuery(sample, user_demands, sample_ratio);
		std::pair<double, double> spl_sum_ci = final_diff_res[0];
		std::pair<double, double> hybrid_sum_ci = final_diff_res[1];
		if (spl_sum_ci.second < hybrid_sum_ci.second) return spl_sum_ci;

		std::vector<int> inds = std::vector<int>(DIM);
		if (mtl_res.size() == 0)
		{
			hybrid_sum_ci.first = -1;
			return hybrid_sum_ci;
		}
		double sum2 = 0;
		ComputeMquery(0, inds, mtl_res, final_mtl_choices, sum2, 0);
		hybrid_sum_ci.first += sum2;
		return hybrid_sum_ci;
	}


	//*********just use ub-lb rather ub-(lb-1) to get the range sum, because the mtl point is sparse. In this case, the result is sum(lb_id,ub_id], note '(]'.
	/*ans need to be inti 0.
	inds need to be init vector<int>(mtl_res.dim).
	mquery means mtl query.
	cpt given mtl_query in mtl result.
	mtl_choice[i] is the id of i-dimension mtl points in mtl result.
	*/
	void Aqpp::ComputeMquery(const int col, std::vector<int>& inds, const MTL_STRU& mtl_res, const std::vector<Condition>& mtl_choices, double& o_sum, int minus_value)
	{
		if (col >= mtl_choices.size()) //dim=mtl_choice.size
		{
			//the following is one term of the final ans.
			int t = 1;
			for (int i = 0; i < inds.size(); i++)
			{
				if (inds[i] < 0) return;
				if (inds[i] == mtl_choices[i].lb_id - minus_value) t = t*(-1);
			}
			if (mtl_res.count(inds) == 1)
			{
				o_sum += t*mtl_res.at(inds);
			}
			return;
		}

		for (int i = 0; i < 2; i++)
		{
			if (i == 0) inds[col] = mtl_choices[col].lb_id - minus_value;
			else inds[col] = mtl_choices[col].ub_id;
			ComputeMquery(col + 1, inds, mtl_res, mtl_choices, o_sum, minus_value);
		}

		return;
	}


	/*
	used for function GenAllRanges
	*/
	Condition Aqpp::GenCondition(const std::vector<CA>& mtl_points, int lbid, int ubid)
	{
		//lbid<0 is valid, but ubid<0 is invalid. but when ubid<0 isn't a valid RG, and shouldn't GenCondition. 
		assert(ubid >= 0 && ubid<mtl_points.size());
		assert(lbid < mtl_points.size());
		Condition d = Condition();
		d.lb_id = lbid;
		d.ub_id = ubid;
		d.lb = lbid < 0 ? -FLT_MAX : mtl_points[lbid].condition_value;
		d.ub = mtl_points[ubid].condition_value;
		return d;

	}

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
	void Aqpp::GenAllRanges(const std::vector<std::vector<CA>>& mtl_points, const std::vector<Condition>& user_demands, std::vector<RGs>& o_all_rgs)
	{
		
		o_all_rgs = std::vector<RGs>(mtl_points.size());
		for (int ci = 0; ci < mtl_points.size(); ci++)
		{
			RGs rgs = RGs();
			CA user_lb = CA();
			CA user_ub = CA();
			if (ci >= user_demands.size())
			{
				user_lb.condition_value = -FLT_MAX;
				user_ub.condition_value = FLT_MAX;
			}
			else
			{
				user_lb.condition_value = user_demands[ci].lb;
				user_ub.condition_value = user_demands[ci].ub;
			}

			int lb_lbid = -1;
			int lb_ubid = -1;
			int ub_lbid = -1;
			int ub_ubid = -1;
			std::vector<CA>::const_iterator it;
			it = upper_bound(mtl_points[ci].begin(), mtl_points[ci].end(), user_lb, CA_compare); //less&equal
			if (it > mtl_points[ci].begin() && it <= mtl_points[ci].end()) //note that the position of '='.
			{
				lb_lbid = it - mtl_points[ci].begin() - 1;
			}
			else lb_lbid = -1;

			it = lower_bound(mtl_points[ci].begin(), mtl_points[ci].end(), user_lb, CA_compare);//great&equal
			if (it >= mtl_points[ci].begin() && it < mtl_points[ci].end())
			{
				lb_ubid = it - mtl_points[ci].begin();
			}
			else lb_ubid = -1;
			//cout << "userlb *it lb_ubid:    "<<user_lb.condition_value << " " << (*it).condition_value<<" "<<lb_ubid << " " << endl;

			it = upper_bound(mtl_points[ci].begin(), mtl_points[ci].end(), user_ub, CA_compare); //less&equal
			if (it>mtl_points[ci].begin() && it <= mtl_points[ci].end())
			{
				ub_lbid = it - mtl_points[ci].begin() - 1;
			}
			else ub_lbid = -1;

			it = lower_bound(mtl_points[ci].begin(), mtl_points[ci].end(), user_ub, CA_compare);//great&equal
			if (it >= mtl_points[ci].begin() && it < mtl_points[ci].end())
			{
				ub_ubid = it - mtl_points[ci].begin();
			}
			else ub_ubid = -1;


			/*----------in case that only one bound is found and cannot construct a mtl range. Especially when do equal query.*/
			if (lb_lbid == lb_ubid && lb_lbid != -1)
			{
				lb_lbid--; //note that -1 is valid for lbid.
			}

			if (ub_lbid == ub_ubid && ub_lbid != -1)
			{
				if (ub_ubid + 1 <= mtl_points[ci].size() - 1) ub_ubid++;
				else ub_lbid--;
			}
			/*----------------------------------*/

			if (lb_lbid<ub_lbid) rgs.push_back(GenCondition(mtl_points[ci], lb_lbid, ub_lbid));
			if (lb_lbid<ub_ubid) rgs.push_back(GenCondition(mtl_points[ci], lb_lbid, ub_ubid));
			if (lb_ubid != -1 && lb_ubid<ub_lbid) rgs.push_back(GenCondition(mtl_points[ci], lb_ubid, ub_lbid));
			if (lb_ubid != -1 && lb_ubid<ub_ubid) rgs.push_back(GenCondition(mtl_points[ci], lb_ubid, ub_ubid));
			o_all_rgs[ci] = rgs;
		}
		return;
	}

	//used for function FillMaterializeChoices.
	// should the row num is the real row num or distinct value of row num? 
	double Aqpp::ComputeDifference4Sum(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands, const std::vector<Condition>& mtl_choices)
	{
		double sum = 0;
		double sum2 = 0;
		const int ROW_NUM = sample[0].size();
		const int USER_CONDITION_DIM = user_demands.size();
		for (int ri = 0; ri < ROW_NUM; ri++)
		{
			bool umeet = true;
			bool mmeet = true;
			for (int ci = 1; ci <user_demands.size() + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLess(con_data, user_demands[ci - 1].lb) || DoubleGreater(con_data, user_demands[ci - 1].ub))
				{
					umeet = false;
					break;
				}
			}
			for (int ci = 1; ci < mtl_choices.size() + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLeq(con_data, mtl_choices[ci - 1].lb) || DoubleGreater(con_data, mtl_choices[ci - 1].ub))
				{
					mmeet = false;
					break;
				}
			}

			double udata = 0;
			double mdata = 0;
			if (umeet) udata = sample[0][ri];
			if (mmeet) mdata = sample[0][ri];
			double data = udata - mdata;
			sum += data;
			sum2 += data*data;
		}
		double variance = 0;
		variance = sum2 / ROW_NUM - (sum / ROW_NUM)*(sum / ROW_NUM);
		double ci = this->PAR.CI_INDEX*sqrt(variance / ROW_NUM)*ROW_NUM / this->PAR.SAMPLE_RATE;
		return  ci;

	}






	/*when got mtl choice, the final compute the diffrence to chose spling or hybrid*/
	void Aqpp::FinalComputeDifference(const std::vector<std::vector<double>>&sample, const std::vector<Condition>& user_demands, const std::vector<Condition>& mtl_choices, std::vector<std::pair<double, double>>& final_diff_res)
	{
		double hsum = 0;//h for hybrid
		double hsum2 = 0;
		double ssum = 0;//s for sampling
		double ssum2 = 0;
		const int ROW_NUM = sample[0].size();
		for (int ri = 0; ri < ROW_NUM; ri++)
		{
			bool umeet = true;
			bool mmeet = true;
			for (int ci = 1; ci <user_demands.size() + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLess(con_data, user_demands[ci - 1].lb) || DoubleGreater(con_data, user_demands[ci - 1].ub))
				{
					umeet = false;
					break;
				}
			}

			for (int ci = 1; ci < mtl_choices.size() + 1; ci++)
			{
				double con_data = sample[ci][ri];
				if (DoubleLeq(con_data, mtl_choices[ci - 1].lb) || DoubleGreater(con_data, mtl_choices[ci - 1].ub))
				{
					mmeet = false;
					break;
				}
			}

			double udata = 0;
			double mdata = 0;
			if (umeet) udata = sample[0][ri];
			if (mmeet) mdata = sample[0][ri];
			double data = udata - mdata;
			hsum += data; //h for hybrid. 
			hsum2 += data*data;

			ssum += udata; //s for sampling.
			ssum2 += udata*udata;
		}
		double hvariance = hsum2 / ROW_NUM - (hsum / ROW_NUM)*(hsum / ROW_NUM);
		double hci = this->PAR.CI_INDEX*sqrt(hvariance / ROW_NUM)*ROW_NUM / this->PAR.SAMPLE_RATE;
		double svariance = ssum2 / ROW_NUM - (ssum / ROW_NUM)*(ssum / ROW_NUM);
		double sci = this->PAR.CI_INDEX*sqrt(svariance / ROW_NUM)*ROW_NUM / this->PAR.SAMPLE_RATE;

		final_diff_res = std::vector<std::pair<double, double>>();
		final_diff_res.push_back(std::pair<double, double>(ssum / this->PAR.SAMPLE_RATE, sci));
		final_diff_res.push_back(std::pair<double, double>(hsum / this->PAR.SAMPLE_RATE, hci));
		return;

	}


	/*
	note that this function doesn't consider the case that don't find the mtl range.
	o_min_ci and mtl_choice should be init outside.
	It choice the mtl solution in small sample, and need to further
	*/
	void Aqpp::FillMaterializeChoices(const int col, const std::vector<std::vector<double>>& small_sample, const std::vector<RGs>& all_rgs, const std::vector<Condition>& user_demands, double &o_min_ci, std::vector<Condition >& mtl_choices, std::vector<Condition>& final_mtl_choices)
	{
		if (col >= all_rgs.size())
		{
			//to do something.

			double ci = ComputeDifference4Sum(small_sample, user_demands, mtl_choices);
			if (ci< o_min_ci)
			{
				o_min_ci = ci;
				final_mtl_choices = mtl_choices;
			}
			return;
		}

		const int CUR_SIZE = all_rgs[col].size();
		for (int i = 0; i <CUR_SIZE; i++)
		{
			mtl_choices[col] = all_rgs[col][i];
			FillMaterializeChoices(col + 1, small_sample, all_rgs, user_demands, o_min_ci, mtl_choices, final_mtl_choices);
		}
	}


}

