/**
note that the defination of MTL_POINT_NUM is different New_Climb. It is the true MTL_POINT_NUM (but pid 0 will be fixed) in this file and doesn't include fake point now.
The point that can move is MTL_POINT_NUMN-1 in this file, while MTL_POINT_NUM in New_climb.
bascially MTL_POINT_NUM in this file is equal to MTL_POINT_NUM-1 in New_climb file.
*/

#pragma once
#define CLB_EVEN_INIT 0
#define CLB_RANDOM_INIT 1
#define CLB_MAXVAR_INIT 2
#define CLB_ADJUST_MINVAR 3
#define CLB_ADJUST_NEARONLY 4
#include<random>
#include<iostream>
#include<time.h>
#include<set>
#include <unordered_map>
#include "common_content.h"
#include <cstdio>
#include<string>
#include<vector>
#include <algorithm>
#include"sql_interface.h"
#include"Tool.h"
#include<cassert>

namespace aqppp {
	class HillClimbing {
	private:

		struct Par
		{
			double SAMPLE_RATE;
			int SAMPLE_ROW_NUM;
			double CI_INDEX;
			int CLIMB_MAX_ITER;
			bool INIT_DISTINCT_EVEN;
		};

		struct Par PAR;

	public:

		HillClimbing(int sample_row_num, double sample_rate, double ci_index, int climb_max_iter, bool init_distinct_even);

		//	to do: each piece should save max1 and max2, in case they appear in the same piece.

		//find the mtl points of  single column of CAsample under no-uniform distribution.
		///return all the mtl points in CA form, which is by condition arribute.
		double  ChoosePoints1Dim(const std::vector<CA>& cur_col, const int MTL_POINT_NUM, std::vector<CA>& o_mtl_points, int *pIter_num = NULL, double *puf_err = NULL,int init_way=CLB_EVEN_INIT,int CLIMB_WAY=CLB_ADJUST_MINVAR);

		//find the mtl points of all columns of CAsample under no-uniform distribution.
		///return all the mtl points in CA form, which is by condition arribute.
		void ChoosePoints(const std::vector<std::vector<CA>>&CAsample, const std::vector<int> &mtl_nums, std::vector<std::vector<CA>>& o_mtl_points, std::vector<double> &o_max_errs, std::vector<int> &o_iter_nums, int init_way = CLB_EVEN_INIT, int CLIMB_WAY = CLB_ADJUST_MINVAR);

		static void ComputeSums(const std::vector<CA>& cur_col, std::vector<double>& o_acc_sum, std::vector<double>& o_acc_sqrsum);

	private:
		struct PairHash {
			template <class T1, class T2>
			std::size_t operator () (const std::pair<T1, T2> &p) const {
				auto h1 = std::hash<T1>{}(p.first);
				auto h2 = std::hash<T2>{}(p.second);
				std::size_t seed = 0;
				HashCombine(seed, h1);
				HashCombine(seed, h2);
				return seed;
			}
		};

		typedef std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, double>>, PairHash> Piece_type;
		typedef std::unordered_map<int, std::vector<std::pair<int, double>>> Remove_type;
		typedef std::unordered_map<int, std::pair<int, int>> Pre_next_type;



		double  ComputeMaxError(const std::vector<CA>& cur_col, const std::vector<CA>& cur_mtl_ca);

		// force mtl last point. DONOT have fake mtl point.
		void InitPointsEven(const std::vector<CA>&CAsample, int MTL_POINT_NUM, std::vector<CA>& o_mtl_points);
		void InitPointsRandom(const std::vector<CA>&CAsample, int MTL_POINT_NUM, std::vector<CA>& o_mtl_points);
		void InitPointsMaxVariance(const std::vector<CA>&CAsample, int MTL_POINT_NUM, std::vector<CA>& o_mtl_points);

		typedef std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, double>>, PairHash> Piece_type;
		typedef std::unordered_map<int, std::vector<std::pair<int, double>>> Remove_type;
		typedef std::unordered_map<int, std::pair<int, int>> Pre_next_type;

		static void CoutMaxError(std::vector<double> &rlv_var, std::vector<double> &choice, std::vector<int> &maxids, std::string s = "");


		/**
		compute the accumulate sum and sqr-sum array for one column.
		*/

		/**
		compute the variance in given range for one column.
		*/
		double ComputeRangeVariance(const std::pair<int, int> rg, const std::vector <double>& sum_acc, const std::vector<double>& sqrsum_acc);





		//cpt rlt var, given mtl point up_id, down_id, and point pid.
		double ComputeRelativeVariance(int up_id, int down_id, int pid, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc);





		/*
		find the max 2 rlt_var points from [up_id+1,down_id-1], given mtl point up_id and down_id.
		If only one point or no point found, the size of return vec will be 1 or 0.
		*/
		std::vector<std::pair<int, double>> FindLocalTwoMaxVariances(int up_id, int down_id, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc);



		/*
		//note that -1 denotes the first piece or the last piece, is valid for pieces.
		max_pieces:
		return the max 2 pieces who has the max 2 variance points.
		If the max 2 variance points are in the same piece, the size of return vec will be 1.
		If only 1 piece or no piece is found, size of return vec will be 1 or 0.
		max_vars_info:
		return max var 1, max_var 2 and related point ids.
		*/
		static void FindTwoMaxPieces(const Piece_type &pieces, std::vector<std::pair<int, int>> &o_max_pieces, std::vector<std::pair<int, double>> &o_max_vars_info);


		double ComputeMaxError(const Piece_type &pieces, std::vector<std::pair<int, double>> &o_max_vars = std::vector<std::pair<int, double>>());


		//find remove_pid with min var, but except ids in except_id
		int FindRemovePoint(const Remove_type  &remove_max_var, std::set<int>& except_id);


		void RemovePoint(int pid, Piece_type &pieces, Pre_next_type &mtl_pre_next_id, Remove_type &remove_max_var, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc);

		void AddPoint(int pid, std::pair<int, int> rg, Piece_type &pieces, Pre_next_type &mtl_pre_next_id, Remove_type &remove_max_var, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc);


		/*try to move or real move remove_id to add_id
		return pair(add_id,max_err)
		*/
		std::pair<int, double> Move(int remove_id, Piece_type &pieces, Pre_next_type &mtl_pre_next_id, Remove_type &remove_max_var, const std::vector<double> &sum_acc, const std::vector<double> &sqrsum_acc, bool real_move);


		static void GenPoints(const Remove_type &remove_max_var, std::vector<int> &o_mtl);
	};
}