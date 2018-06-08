/**
note that the defination of MTL_POINT_NUM is different New_Climb. It is the true MTL_POINT_NUM (but pid 0 will be fixed) in this file and doesn't include fake point now.
The point that can move is MTL_POINT_NUMN-1 in this file, while MTL_POINT_NUM in New_climb.
bascially MTL_POINT_NUM in this file is equal to MTL_POINT_NUM-1 in New_climb file.
*/
//#include"stdafx.h"
#include"hill_climbing.h"
namespace aqppp {
	HillClimbing::HillClimbing(int sample_row_num, double sample_rate, double ci_index, int climb_max_iter, bool init_distinct_even)
	{
		srand(0);
		this->PAR.SAMPLE_ROW_NUM = sample_row_num;
		this->PAR.SAMPLE_RATE = sample_rate;
		this->PAR.CI_INDEX = ci_index;
		this->PAR.CLIMB_MAX_ITER = climb_max_iter;
		this->PAR.INIT_DISTINCT_EVEN = init_distinct_even;
	}


	//find the mtl points of all columns of CAsample under no-uniform distribution.
	///return all the mtl points in CA form, which is by condition arribute.
	void HillClimbing::ChoosePoints(const std::vector<std::vector<CA>>&CAsample, const std::vector<int> &mtl_nums, std::vector<std::vector<CA>>& o_mtl_points, std::vector<double> &o_max_errs, std::vector<int> &o_iter_nums, int INIT_WAY, int ADJUST_WAY)
	{
		const int DIM = CAsample.size();
		o_mtl_points = std::vector<std::vector<CA>>(DIM);
		o_max_errs = std::vector<double>(DIM);
		o_iter_nums = std::vector<int>(DIM);
		for (int di = 0; di < CAsample.size(); di++)
		{
			int cur_mtl_num = min((int)CAsample[di].size(), mtl_nums[di]);
			o_max_errs[di] = ChoosePoints1Dim(CAsample[di], cur_mtl_num, o_mtl_points[di], &o_iter_nums[di], 0,INIT_WAY, ADJUST_WAY);
		}
		return;
	}

	


	double  HillClimbing::ComputeMaxError(const std::vector<CA>& cur_col, const std::vector<CA>& cur_mtl_ca)
	{
		if (cur_col.size() == cur_mtl_ca.size()) return 0.0;
		const int MTL_POINT_NUM = cur_mtl_ca.size();
		std::vector<int> cur_mtl = std::vector<int>();
		for (int i = 0;i < MTL_POINT_NUM;i++)
			cur_mtl.push_back(cur_mtl_ca[i].id);    //note that cur_mtl_ca[0] is a fake point, and shouldn't be add in cur_mtl.
													//cur_mtl doesn't include fake point, and its size is equal to MTL_POINT_NUM.

		std::vector < double> sum_acc = std::vector<double>();
		std::vector<double> sqrsum_acc = std::vector<double>();
		ComputeSums(cur_col, sum_acc, sqrsum_acc);

		//pieces: key(sid,tid) value:(local_max_id,local_max_var)
		Piece_type pieces = Piece_type();
		pieces[{-1, cur_mtl[0]}] = FindLocalTwoMaxVariances(-1, cur_mtl[0], sum_acc, sqrsum_acc);  //pieces before cur_mtl[0]
		for (int i = 1;i < MTL_POINT_NUM;i++)
		{
			int pid = cur_mtl[i];
			int pre_id = cur_mtl[i - 1];
			pieces[{pre_id, pid}] = FindLocalTwoMaxVariances(cur_mtl[i - 1], cur_mtl[i], sum_acc, sqrsum_acc);
		}
		pieces[{cur_mtl[MTL_POINT_NUM - 1], -1}] = FindLocalTwoMaxVariances(cur_mtl[MTL_POINT_NUM - 1], -1, sum_acc, sqrsum_acc); //piece after cur-mtl[last].	
		std::vector<std::pair<int, double>> cur_max_vars;
		double max_err = ComputeMaxError(pieces, cur_max_vars);
		return max_err;
	}

	// force mtl last point. DONOT have fake mtl point.
	void  HillClimbing::InitPointsEven(const std::vector<CA>&CAsample, const int MTL_POINT_NUM, std::vector<CA>& o_mtl_points)
	{
		int mtl_num = MTL_POINT_NUM;
		if (mtl_num > CAsample.size()) mtl_num = CAsample.size();
		o_mtl_points = std::vector<CA>();
		const int casample_size = CAsample.size();
		double GAP = max(1.0, (double)this->PAR.SAMPLE_ROW_NUM / (double)mtl_num);
		double real_count = 0;
		double ideal_count = GAP;
		for (int ri = 0;ri < casample_size;ri++)
		{
			real_count += CAsample[ri].count;
			if (DoubleGeq(real_count, ideal_count) || (casample_size - ri <= mtl_num - o_mtl_points.size()))    //later cond is when left points is smaller than left mtl_points, they all should be mtled.
			{
				o_mtl_points.push_back(CAsample[ri]);
				if (o_mtl_points.size() >= mtl_num) break;
				ideal_count = GAP*(double)(o_mtl_points.size() + 1);
			}
		}
		assert(o_mtl_points.size() == mtl_num);
		o_mtl_points[o_mtl_points.size() - 1] = CAsample[casample_size - 1];   //force mtl the last point.
		return;
	}

	void HillClimbing::InitPointsRandom(const std::vector<CA>&cur_col, const int MTL_POINT_NUM, std::vector<CA>& o_mtl_points)
	{
		std::mt19937 rand_gen(rand());
		int ROW_NUM = cur_col.size();
		int MTL_NUM = min(MTL_POINT_NUM, ROW_NUM);
		o_mtl_points = std::vector<CA>();
		std::vector<int> ids = std::vector<int>();
		for (int id = 0; id < ROW_NUM - 1; id++) ids.push_back(id);
		std::shuffle(ids.begin(), ids.end(), rand_gen);
		for (int i = 0; i < MTL_NUM - 1; i++)
		{
			o_mtl_points.push_back(cur_col[ids[i]]);
		}
		o_mtl_points.push_back(cur_col[ROW_NUM - 1]); //force mtl last point.
		sort(o_mtl_points.begin(), o_mtl_points.end(), CA_compare);
		assert(o_mtl_points.size() == MTL_POINT_NUM);
		return;
	}

	void HillClimbing::InitPointsMaxVariance(const std::vector<CA>&cur_col, int MTL_POINT_NUM, std::vector<CA>& o_mtl_points)
	{
		int ROW_NUM = cur_col.size();
		int MTL_NUM = min(MTL_POINT_NUM, ROW_NUM);
		o_mtl_points = std::vector<CA>();
		std::vector<double> sum_acc;
		std::vector<double> sum2_acc;
		ComputeSums(cur_col, sum_acc, sum2_acc);
		std::map<std::pair<int, int>, double> rg_var;
		rg_var.insert({ { 0,ROW_NUM - 1 }, 0 });
		std::set<int> mtl_ids = std::set<int>();
		for (int i = 0; i < MTL_NUM; i++)
		{
			auto cur_rg = std::max_element(rg_var.begin(), rg_var.end())->first;
			std::vector<std::pair<int, double>> pos_vars = FindLocalTwoMaxVariances(cur_rg.first, cur_rg.second, sum_acc, sum2_acc);
			std::pair<int, double> pos_var;
			if (pos_vars.size() > 0) pos_var = pos_vars[0];
			else pos_var = { (cur_rg.first + cur_rg.second) / 2,-1 };

			std::vector<std::pair<int, double>>  up_pos_vars = FindLocalTwoMaxVariances(cur_rg.first, pos_var.first, sum_acc, sum2_acc);
			std::vector<std::pair<int, double>>  down_pos_vars = FindLocalTwoMaxVariances(pos_var.first, cur_rg.second, sum_acc, sum2_acc);
			double up_var = (up_pos_vars.size() > 0) ? up_pos_vars[0].second : -1;
			double down_var = (down_pos_vars.size() > 0) ? down_pos_vars[0].second : -1;

			rg_var.erase(cur_rg);
			rg_var.insert({ { cur_rg.first, pos_var.first }, up_var });
			rg_var.insert({ { pos_var.first,cur_rg.second }, down_var });
			mtl_ids.insert(pos_var.first);
		}

		while (mtl_ids.size() < MTL_POINT_NUM)
		{
			int id = rand() % ROW_NUM;
			mtl_ids.insert(id);
		}

		for (int id:mtl_ids)
		{
			o_mtl_points.push_back(cur_col[id]);
		}
		
		o_mtl_points.back() = cur_col[ROW_NUM - 1];
		sort(o_mtl_points.begin(), o_mtl_points.end(), CA_compare);
		assert(o_mtl_points.size() == MTL_POINT_NUM);
	}


	void HillClimbing::CoutMaxError(std::vector<double> &rlv_var, std::vector<double> &choice, std::vector<int> &maxids, std::string s)
	{

		std::cout << std::endl << s << std::endl;
		std::cout << "max1: " << maxids[0] << " " << choice[maxids[0]] << " " << rlv_var[maxids[0]] << std::endl;
		std::cout << "max2: " << maxids[1] << " " << choice[maxids[1]] << " " << rlv_var[maxids[1]] << std::endl;
		std::cout << std::endl;
	}


	/**
	compute the accumulate sum and sqr-sum array for one column.
	*/
	void HillClimbing::ComputeSums(const std::vector<CA>& cur_col, std::vector<double>& o_acc_sum, std::vector<double>& o_acc_sqrsum)
	{
		o_acc_sum = std::vector<double>();
		o_acc_sqrsum = std::vector<double>();
		o_acc_sum.push_back(cur_col[0].sum);
		o_acc_sqrsum.push_back(cur_col[0].sqrsum);
		for (int i = 1; i < cur_col.size(); i++)
		{
			o_acc_sum.push_back(o_acc_sum[i - 1] + cur_col[i].sum);
			o_acc_sqrsum.push_back(o_acc_sqrsum[i - 1] + cur_col[i].sqrsum);
		}
		return;
	}

	/**
	compute the variance in given range for one column.
	*/
	double HillClimbing::ComputeRangeVariance(const std::pair<int, int> rg, const std::vector <double>& sum_acc, const std::vector<double>& sqrsum_acc)
	{
		assert(rg.first <= rg.second);
		double sum = 0.0;
		double sqrsum = 0.0;
		if (rg.first == 0)
		{
			sum = sum_acc[rg.second];
			sqrsum = sqrsum_acc[rg.second];
		}
		else
		{
			sum = sum_acc[rg.second] - sum_acc[rg.first - 1];
			sqrsum = sqrsum_acc[rg.second] - sqrsum_acc[rg.first - 1];
		}

		double var = sqrsum / this->PAR.SAMPLE_ROW_NUM - (sum / this->PAR.SAMPLE_ROW_NUM)*(sum / this->PAR.SAMPLE_ROW_NUM);

		if (var < 0)
		{
			var = 0;
			std::cout << "variance<0 , make it to 0. Maybe 'double' doesn't satisify the precision !" << std::endl;
		}

		return var;
	}




	//cpt rlt var, given mtl point up_id, down_id, and point pid.
	double HillClimbing::ComputeRelativeVariance(int up_id, int down_id, int pid, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc)
	{
		assert(pid >= 0);
		assert(up_id >= 0 || down_id >= 0);
		assert(up_id < 0 || (up_id < pid));
		assert(down_id<0 || down_id>pid);
		if (down_id < 0) return ComputeRangeVariance({ up_id + 1,pid }, sum_acc, sqrsum_acc);
		if (up_id < 0) up_id = -1;
		std::pair<int, int> rg1 = std::pair<int, int>(up_id + 1, pid);
		std::pair<int, int> rg2 = std::pair<int, int>(pid, down_id - 1);
		double var1 = ComputeRangeVariance(rg1, sum_acc, sqrsum_acc);
		double var2 = ComputeRangeVariance(rg2, sum_acc, sqrsum_acc);
		return min(var1, var2);

	}





	/*
	find the max 2 rlt_var points from [up_id+1,down_id-1], given mtl point up_id and down_id.
	If only one point or no point found, the size of return vec will be 1 or 0.
	*/
	std::vector<std::pair<int, double>> HillClimbing::FindLocalTwoMaxVariances(int up_id, int down_id, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc)
	{
		if (up_id < 0 && down_id < 0) return  std::vector<std::pair<int, double>>();
		int p_start = (up_id < 0) ? 0 : up_id + 1;
		int p_end = (down_id < 0) ? sum_acc.size() : down_id;         			//note p_end isn't supposed to be included when enum.

		std::vector<std::pair<int, double>> max_vars = std::vector<std::pair<int, double>>();
		std::pair<int, double> max1 = { -1,-FLT_MAX };
		std::pair<int, double> max2 = { -1,-FLT_MAX };

		for (int pid = p_start;pid < p_end;pid++)
		{
			double rlt_var = ComputeRelativeVariance(up_id, down_id, pid, sum_acc, sqrsum_acc);
			if (rlt_var > max1.second)
			{
				max2 = std::pair<int, double>(max1);
				max1 = { pid,rlt_var };
			}
			else if (rlt_var > max2.second)
			{
				max2 = { pid,rlt_var };
			}
		}
		if (max1.first >= 0) max_vars.push_back(max1);
		if (max2.first >= 0) max_vars.push_back(max2);
		return max_vars;
	}



	/*
	//note that -1 denotes the first piece or the last piece, is valid for pieces.
	max_pieces:
	return the max 2 pieces who has the max 2 variance points.
	If the max 2 variance points are in the same piece, the size of return vec will be 1.
	If only 1 piece or no piece is found, size of return vec will be 1 or 0.
	max_vars_info:
	return max var 1, max_var 2 and related point ids.
	*/
	void HillClimbing::FindTwoMaxPieces(const Piece_type &pieces, std::vector<std::pair<int, int>> &o_max_pieces, std::vector<std::pair<int, double>> &o_max_vars_info)
	{

		std::pair<int, int> maxp1 = { -2,-2 };
		std::pair<int, int> maxp2 = { -2,-2 };
		std::pair<int, double> max_var1 = { -1, -FLT_MAX };
		std::pair<int, double> max_var2 = { -1,-FLT_MAX };

		for (auto &it : pieces)
		{
			for (auto &v : it.second)
			{
				if (v.second > max_var1.second)
				{
					max_var2 = std::pair<int, double>(max_var1);
					max_var1 = std::pair<int, double>(v);
					maxp2 = std::pair<int, int>(maxp1);
					maxp1 = std::pair<int, int>(it.first);
				}
				else if (v.second > max_var2.second)
				{
					max_var2 = std::pair<int, double>(v);
					maxp2 = std::pair<int, int>(it.first);
				}
			}
		}

		o_max_vars_info = std::vector<std::pair<int, double>>();
		if (max_var1.first >= 0 && max_var1.second >= 0) o_max_vars_info.push_back(max_var1);
		if (max_var2.first >= 0 && max_var2.second >= 0) o_max_vars_info.push_back(max_var2);

		o_max_pieces = std::vector<std::pair<int, int>>();
		if (maxp1.first >= -1 && maxp1.second >= -1) o_max_pieces.push_back(maxp1);      //note that -1 denotes the first piece or the last piece.  
		if (maxp2 != maxp1 && maxp2.first >= -1 && maxp2.second >= -1) o_max_pieces.push_back(maxp2);
	}


	double HillClimbing::ComputeMaxError(const Piece_type &pieces, std::vector<std::pair<int, double>> &o_max_vars)
	{
		o_max_vars = std::vector<std::pair<int, double>>();
		o_max_vars.push_back({ -1,-FLT_MAX });
		o_max_vars.push_back({ -1,-FLT_MAX });
		for (auto &cur_pie : pieces)
		{
			for (auto &rg_info : cur_pie.second)
			{
				if (rg_info.second > o_max_vars[0].second)
				{
					o_max_vars[1] = std::pair<int, double>(o_max_vars[0]);
					o_max_vars[0] = std::pair<int, double>(rg_info);
				}
				else if (rg_info.second > o_max_vars[1].second)
				{
					o_max_vars[1] = std::pair<int, double>(rg_info);
				}
			}
		}

		double max1 = o_max_vars[0].second > 0 ? o_max_vars[0].second : 0;
		double max2 = o_max_vars[1].second > 0 ? o_max_vars[1].second : 0;
		return this->PAR.CI_INDEX*(sqrt(max1 / this->PAR.SAMPLE_ROW_NUM) + sqrt(max2 / this->PAR.SAMPLE_ROW_NUM))*(double)this->PAR.SAMPLE_ROW_NUM / (double)this->PAR.SAMPLE_RATE;
	}


	//find remove_pid with min var, but except ids in except_id
	int HillClimbing::FindRemovePoint(const Remove_type  &remove_max_var, std::set<int>& except_id)
	{
		double min_var = DBL_MAX;
		int min_pid = -1;
		for (auto &it : remove_max_var)
		{
			int cur_pid = it.first;
			if (it.second.size() == 0 || except_id.count(cur_pid) > 0) continue;
			double var = (it.second)[0].second;
			if (var < min_var)
			{
				min_var = var;
				min_pid = cur_pid;
			}
		}
		//assert(min_pid >=0);
		return min_pid;
	}


	void HillClimbing::RemovePoint(int pid, Piece_type &pieces, Pre_next_type &mtl_pre_next_id, Remove_type &remove_max_var, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc)
	{
		assert(pid >= 0);

		int pre_id = mtl_pre_next_id[pid].first;
		int next_id = mtl_pre_next_id[pid].second;
		pieces.erase({ pre_id, pid });
		pieces.erase({ pid, next_id });
		std::pair<int, int> new_key(pre_id, next_id);
		std::vector<std::pair<int, double>> new_rg_info(remove_max_var.at(pid));
		pieces.insert({ new_key, new_rg_info });

		mtl_pre_next_id.erase(pid);
		if (pre_id >= 0) mtl_pre_next_id.at(pre_id).second = next_id;
		if (next_id >= 0) mtl_pre_next_id.at(next_id).first = pre_id;

		remove_max_var.erase(pid);
		if (pre_id >= 0) remove_max_var.at(pre_id) = FindLocalTwoMaxVariances(mtl_pre_next_id.at(pre_id).first, mtl_pre_next_id.at(pre_id).second, sum_acc, sqrsum_acc);
		if (next_id >= 0)  remove_max_var.at(next_id) = FindLocalTwoMaxVariances(mtl_pre_next_id.at(next_id).first, mtl_pre_next_id.at(next_id).second, sum_acc, sqrsum_acc);
		return;

	}

	void HillClimbing::AddPoint(int pid, std::pair<int, int> rg, Piece_type &pieces, Pre_next_type &mtl_pre_next_id, Remove_type &remove_max_var, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc)
	{
		assert(pid >= 0);
		//has considered add into the first piece and the last piece.			
		assert((pid > rg.first && pid < rg.second) || (rg.first < 0) || (rg.second < 0));

		int pre_id = rg.first;
		int next_id = rg.second;
		auto rg_it = pieces.find(rg);

		assert(rg_it != pieces.end());

		std::vector<std::pair<int, double>> rg_info(rg_it->second);
		pieces.erase(rg_it);
		std::pair<int, int> new_key1(pre_id, pid);
		std::pair<int, int> new_key2(pid, next_id);
		std::vector<std::pair<int, double>> new_rg_info1 = FindLocalTwoMaxVariances(pre_id, pid, sum_acc, sqrsum_acc);
		std::vector<std::pair<int, double>> new_rg_info2 = FindLocalTwoMaxVariances(pid, next_id, sum_acc, sqrsum_acc);
		pieces.insert({ new_key1,new_rg_info1 });
		pieces.insert({ new_key2, new_rg_info2 });

		if (pre_id >= 0) mtl_pre_next_id.at(pre_id).second = pid;
		if (next_id >= 0) mtl_pre_next_id.at(next_id).first = pid;
		mtl_pre_next_id.insert({ pid, rg });

		remove_max_var.insert({ pid,rg_info });
		if (pre_id >= 0) remove_max_var.at(pre_id) = FindLocalTwoMaxVariances(mtl_pre_next_id.at(pre_id).first, mtl_pre_next_id.at(pre_id).second, sum_acc, sqrsum_acc);
		if (next_id >= 0)  remove_max_var.at(next_id) = FindLocalTwoMaxVariances(mtl_pre_next_id.at(next_id).first, mtl_pre_next_id.at(next_id).second, sum_acc, sqrsum_acc);

		return;

	}


	/*try to move or real move remove_id to add_id
	return pair(add_id,max_err)
	*/
	std::pair<int, double> HillClimbing::Move(int remove_id, Piece_type &pieces, Pre_next_type &mtl_pre_next_id, Remove_type &remove_max_var, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc, bool real_move)
	{
		std::pair<int, int> rg(mtl_pre_next_id.at(remove_id));


#ifdef _DEBUG
		std::vector<std::pair<int, double>> init_max_vars_info;
		std::vector<std::pair<int, int>>  init_max_pieces;
		FindTwoMaxPieces(pieces, init_max_pieces, init_max_vars_info);
#endif


		RemovePoint(remove_id, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc);


#ifdef _DEBUG
		std::vector<std::pair<int, double>> remove_max_vars_info;
		std::vector<std::pair<int, int>>  remove_max_pieces;
		FindTwoMaxPieces(pieces, remove_max_pieces, remove_max_vars_info);
#endif



		/*------------try to move to max var 1 or max var 2, depend on which move brings smallest max_err--------------*/
		int add_id = -1;
		std::pair<int, int> id_pie = { -1,-1 };
		double min_err = DBL_MAX;
		std::vector<std::pair<int, int>> max_pieces;
		FindTwoMaxPieces(pieces, max_pieces, std::vector<std::pair<int, double>>());
		if ((max_pieces.size() == 1))
		{
			std::pair<int, int> maxp = max_pieces[0];
			std::vector<std::pair<int, double>> maxp_info = pieces.at(maxp);
			for (int i = 0;i < maxp_info.size();i++)
			{
				int t_add_id = maxp_info[i].first;
				AddPoint(t_add_id, maxp, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc);
				double t_max_err = ComputeMaxError(pieces);
				RemovePoint(t_add_id, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc);
				if (t_max_err < min_err)
				{
					add_id = t_add_id;
					id_pie = maxp;
					min_err = t_max_err;
				}
			}
		}
		else
		{
			for (auto maxp : max_pieces)
			{
				int t_add_id = (pieces.at(maxp))[0].first;
				AddPoint(t_add_id, maxp, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc);
				double t_max_err = ComputeMaxError(pieces);
				if (t_max_err < min_err)
				{
					add_id = t_add_id;
					id_pie = maxp;
					min_err = t_max_err;
				}
				RemovePoint(t_add_id, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc);
			}
		}


		/*add_id is the real move point, this is to do real move.*/
		AddPoint(add_id, id_pie, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc);


#ifdef _DEBUG
		std::vector<std::pair<int, double>> add_max_vars_info;
		std::vector<std::pair<int, int>>  add_max_pieces;
		FindTwoMaxPieces(pieces, add_max_pieces, add_max_vars_info);
#endif


		//if not real move, do the recover.
		if (!real_move)
		{
			RemovePoint(add_id, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc);
			AddPoint(remove_id, rg, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc);
		}

		return std::pair<int, double>(add_id, min_err);
	}


	void HillClimbing::GenPoints(const Remove_type &remove_max_var, std::vector<int> &o_mtl)
	{
		o_mtl = std::vector<int>(remove_max_var.size());
		int id = 0;
		for (auto &pid : remove_max_var)
		{
			o_mtl[id] = pid.first;
			id++;
		}
		sort(o_mtl.begin(), o_mtl.end());
	}

	//	to do: each piece should save max1 and max2, in case they appear in the same piece.

	//find the mtl points of  single column of CAsample under no-uniform distribution.
	///return all the mtl points in CA form, which is by condition arribute.
	double HillClimbing::ChoosePoints1Dim(const std::vector<CA>& cur_col, const int MTL_POINT_NUM, std::vector<CA>& o_mtl_points, int *pIter_num, double *puf_err,int INIT_WAY,int ADJUST_WAY)
	{
		if (MTL_POINT_NUM >= cur_col.size())
		{
			o_mtl_points = std::vector<CA>(cur_col);
			return 0.0;
		}
		assert(MTL_POINT_NUM > 0);
		std::vector<CA> cur_mtl_ca = std::vector<CA>();

		switch (INIT_WAY)
		{
		case CLB_EVEN_INIT:
			InitPointsEven(cur_col, MTL_POINT_NUM, cur_mtl_ca);
			break;
		case CLB_RANDOM_INIT:
			InitPointsRandom(cur_col,MTL_POINT_NUM,cur_mtl_ca);
			break;
		case CLB_MAXVAR_INIT:
			InitPointsMaxVariance(cur_col,MTL_POINT_NUM,cur_mtl_ca);
			break;
		default:
			break;
		}



		std::vector<int> cur_mtl = std::vector<int>();
		for (int i = 0;i < cur_mtl_ca.size();i++)
			cur_mtl.push_back(cur_mtl_ca[i].id);
		assert(cur_mtl.size() == MTL_POINT_NUM);

		Pre_next_type mtl_pre_next_id = std::unordered_map<int, std::pair<int, int>>();

		mtl_pre_next_id[cur_mtl[0]] = MTL_POINT_NUM > 1 ? std::pair<int, int>(-1, cur_mtl[1]) : std::pair<int, int>(-1, -1);
		if (MTL_POINT_NUM > 1)
		{
			mtl_pre_next_id[cur_mtl[MTL_POINT_NUM - 1]] = std::pair<int, int>(cur_mtl[MTL_POINT_NUM - 2], -1);
		}
		for (int i = 1;i < MTL_POINT_NUM - 1;i++)
			mtl_pre_next_id[cur_mtl[i]] = std::make_pair(cur_mtl[i - 1], cur_mtl[i + 1]);


		std::vector < double> sum_acc = std::vector<double>();
		std::vector<double> sqrsum_acc = std::vector<double>();
		ComputeSums(cur_col, sum_acc, sqrsum_acc);

		//pieces: key(sid,tid) value:(local_max_id,local_max_var)
		Piece_type pieces = Piece_type();
		pieces[{-1, cur_mtl[0]}] = FindLocalTwoMaxVariances(-1, cur_mtl[0], sum_acc, sqrsum_acc);  //pieces before cur_mtl[0]
		for (int i = 1;i < MTL_POINT_NUM;i++)
		{
			int pid = cur_mtl[i];
			int pre_id = cur_mtl[i - 1];
			pieces[{pre_id, pid}] = FindLocalTwoMaxVariances(cur_mtl[i - 1], cur_mtl[i], sum_acc, sqrsum_acc);
		}
		pieces[{cur_mtl[MTL_POINT_NUM - 1], -1}] = FindLocalTwoMaxVariances(cur_mtl[MTL_POINT_NUM - 1], -1, sum_acc, sqrsum_acc); //piece after cur-mtl[last].


																															  //remove_max_var[pid] means max var after remove mtl point pid.
		Remove_type remove_max_var = Remove_type();
		for (int i = 0;i < MTL_POINT_NUM;i++)
		{
			std::pair<int, int> pre_next = mtl_pre_next_id[cur_mtl[i]];
			remove_max_var[cur_mtl[i]] = FindLocalTwoMaxVariances(pre_next.first, pre_next.second, sum_acc, sqrsum_acc);
		}

		std::vector<std::pair<int, double>> cur_max_vars;
		double cur_err = ComputeMaxError(pieces, cur_max_vars);
		double init_err = cur_err;
		if (puf_err != NULL) *puf_err = init_err;
		//double cur_max1 = cur_max_vars[0].second;
		//cout << "init maxerr: " << cur_err << endl;

		int iter = 0;
		if (pIter_num != NULL) *pIter_num = 0;
		std::vector<int> force_mtl_points = std::vector<int>({ cur_mtl[MTL_POINT_NUM - 1] });  // save points force mtled. the pid is the id in cur_mtl_ca(which is init mtl state).
		for (iter = 0;iter < this->PAR.CLIMB_MAX_ITER; iter++)
		{

			std::vector<std::pair<int, int>> max_pieces = std::vector<std::pair<int, int>>();
			FindTwoMaxPieces(pieces, max_pieces, std::vector<std::pair<int, double>>());

			std::set<int> move_candicate = std::set<int>();
			for (int i = 0;i < max_pieces.size();i++)
			{
				move_candicate.insert(max_pieces[i].first);
				move_candicate.insert(max_pieces[i].second);
			}
			for (int pid : force_mtl_points)
			{
				move_candicate.insert(pid);  //to make sure that they won't be found in FindRemovePoint.
			}

			if (ADJUST_WAY == CLB_ADJUST_MINVAR)
			{
				int remove_pid = FindRemovePoint(remove_max_var, move_candicate);
				move_candicate.insert(remove_pid);
			}

			move_candicate.erase(-1);
			for (int pid : force_mtl_points)
			{
				move_candicate.erase(pid); //to make force_mtl_points fixed.
			}
			std::pair<int, double> min_err = { -1, DBL_MAX };
			int min_id = -1;
			for (auto &try_id : move_candicate)
			{
				std::pair<int, double> try_err = Move(try_id, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc, false);
				if (try_err.second < min_err.second)
				{
					min_err = std::pair<int, double>(try_err);
					min_id = try_id;
				}
			}
			if (DoubleLess(min_err.second, cur_err) || (DoubleEqual(min_err.second, cur_err) && min_err.first != min_id))     // if max_err didn't change but remove_id changed then do this move. Because min_id will move to local maxid.
			{
				cur_err = Move(min_id, pieces, mtl_pre_next_id, remove_max_var, sum_acc, sqrsum_acc, true).second;
				//cur_maxerr = ComputeMaxError(pieces);
				if (pIter_num != NULL) *pIter_num = iter + 1;

			}
			else break;
		}


		std::vector<int> final_mtl = std::vector<int>();
		GenPoints(remove_max_var, final_mtl);

		//	cout << "iter num: " << iter << endl;

		//	cout << "no-uniform max error & iter_num " << cur_err <<" "<<iter<<endl << endl;
		//save the result.
		o_mtl_points = std::vector<CA>();

		/*
		CA mInf = CA();
		mInf.condition_value = -FLT_MAX;
		mInf.aggregate_value = 0;
		o_mtl_points.push_back(mInf); // add a fake point which is used for GenAllRanges func in hybrid.hpp
		*/

		//cout << endl << "climb method mtl point pos";
		for (int i = 0; i < final_mtl.size(); i++)
		{
			int pos = final_mtl[i];
			o_mtl_points.push_back(cur_col[pos]);
			//	cout << cur_col[pos].condition_value << " ";
		}


#ifdef _DEBUG

		//cout << endl << endl << "init_err/final_err/iternum  " << init_err << " " << cur_err << " " << iter << endl << "---------end--------" << endl << endl;
#endif // def _DEBUG


		return cur_err;
	}

}