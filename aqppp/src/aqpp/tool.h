#pragma once
#include<direct.h>
#include<assert.h>
#include <iostream>
#include<fstream>
#include <vector>
#include <iterator>
#include<algorithm>
#include <sstream>
#include<string>
#include<random>
#include"common_content.h"
#include<unordered_set>
namespace aqppp
{
	class Tool {
	public:

		template<typename Out>
		static void split(const std::string &s, char delim, Out result) {
			std::stringstream ss;
			ss.str(s);
			std::string item;
			while (std::getline(ss, item, delim)) {
				*(result++) = item;
			}
		}

		static bool notAllGroupHas(const std::vector<Condition> &demands, const std::unordered_map<std::vector<int>, Group, VectorHash> &groups);
		static  int  MapMultidimTo1Dim(const std::vector<int> &pos, const std::vector<int>& DIM_SIZES);
		static int Map1DimToMultidim(int id, const std::vector<int> &DIM_SIZES, std::vector<int> &o_pos);
		static void add(std::vector<double> &o_vec, double t);
		static  void add(std::vector<double> &o_vec1, const std::vector<double> &vec2);
		static void multpily(const std::vector<double> &vec, double t, std::vector<double> &o_res);
		static double get_sum(const std::vector<double> &data);
		static double get_avg(const std::vector<double> &data);
		static double get_var(const std::vector<double> &data);
		static double get_percentile(const std::vector<double> &data, double percent);
		static double ComputeCovariance(const std::vector<double>& A, const std::vector<double>& Y);
		static double ComputeCorrelation(std::vector<double>& X, std::vector<double>& Y);

		static void MkDirRecursively(std::string dirpath);

		static std::vector<std::string> split(const std::string &s, char delim);
		static void SaveQueryFile(std::string query_file_full_name, std::vector<std::vector<Condition>> &o_user_queries);
		static void ReadQueriesFromFile(std::string query_file_full_name, int query_dim, std::vector<std::vector<Condition>> &o_user_queries);
		/*
		CA_sample doesn't include fake point.
		The duplicate conditions in each col has only kept one condition, and the aggreate exists of others has been added to that one.
		trans original sample into CAsample.
		note the CAsample doesn't have the accumulation column because it has the struct of condition attribute combined with accumulation arribute.
		each col of CAsample is the combination of the accumulatation attribute and condition attribute of the original sample. Each column of it is sorted by condition, then aggreate data.

		*/
		static void TransSample(const std::vector<std::vector<double>> &sample, std::vector<std::vector<CA>> &o_CAsample);

		static std::pair<int, double> EstimateSelectively(const std::vector<std::vector<double>> &sample, const std::vector<Condition> &demands);
		static void GenUserQuires(const std::vector<std::vector<double>> &sample, const std::vector<std::vector<CA>> &casample, const int seed, const int query_num, std::pair<double, double> QUERY_SELECTIVELY_RANGE, std::vector<std::vector<Condition> > &o_user_queries, const std::unordered_map<std::vector<int>, Group, VectorHash> &groups= std::unordered_map<std::vector<int>, Group, VectorHash>());
	};

}