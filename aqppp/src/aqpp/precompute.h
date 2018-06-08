#pragma once
#include<mutex>
#include<vector>
#include<unordered_map>
#include<string>
#include<typeinfo>
#include"common_content.h"
#include "sql_interface.h"
#include"tool.h"
#include<algorithm>
#include<assert.h>

namespace aqppp {
	class Precompute
	{
	private: struct Par
	{
		std::string DB_NAME;
		std::string TABLE_NAME;
		std::string AGGREGATE_NAME;
		std::vector<std::string> CONDITION_NAMES;
	} PAR;

	public:
		Precompute(std::string db_name, std::string table_name, std::string agg_name, std::vector<std::string> condition_names);
		static int MyLowerBound(const std::vector<CA> &cur_col, const CA &key);
		void InitCube(int a, std::vector<int> &cur_indx, MTL_STRU &o_mtl_data, const std::vector<std::vector<CA>>& mtl_points);
		double GetPrefixSumCube(const std::vector<std::vector<CA>>& mtl_points, SQLHANDLE &sqlconnectionhandle, MTL_STRU & o_mtl_res, std::string query_type, const DistId &distinct_ids = DistId());
		double GetPrefixSumCube(const std::vector<int>& size_of_dimension, const MTL_STRU& mtl_data, MTL_STRU& o_mtl_res);
	private:
		double ComputeDataCube(const std::vector<std::vector<CA>>& mtl_points, SQLHANDLE &sqlconnectionhandle, MTL_STRU& o_mtl_data, std::string query_type, const DistId &distinct_ids = DistId());
		void ComputeSumCube(int a, int b, const std::vector<int>& size_of_dimension, std::vector<int>& cur_ind, MTL_STRU& o_mtl_res);
		static void  Precompute::ComputeDataCubeOneThread(int thread_id, int start_row, int end_row, int CONDITION_DIM, const std::vector<std::vector<CA>> &mtl_points, double(&table)[11][1000000], std::vector<double>& cube, std::vector<int>& cube_sizes, std::vector<std::mutex>& cube_locks);
		static void  Precompute::ReadColumnOneThread(SQLHANDLE &sqlstatementhandle);
	};
}