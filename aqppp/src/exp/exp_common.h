#pragma once
#include<assert.h>
#include <fstream>
#include "../aqpp/common_content.h"
#include"../aqpp/tool.h"
#include<vector>
#include"../aqpp/sql_interface.h"
namespace expDemo {
	void WritePar(FILE *par_file, aqppp::Settings par);

	std::pair<double, double> ReadSamples(SQLHANDLE &sqlconnectionhandle, aqppp::Settings& o_PAR, int run_num, std::vector<std::vector<double>>& o_sample, std::vector<std::vector<double>>& o_small_sample);

	//pair<double, double> query_real_value4sum(vector<Condition>& cur_q, string table_name, MYSQL* conn, const Settings PAR);

	double QueryRealValue(const std::vector<aqppp::Condition>& cur_q, std::string table_name, SQLHANDLE &sqlconnectionhandle, const aqppp::Settings PAR, std::string agg_action, std::vector<std::unordered_map<int, std::string>> &distinct_itos= std::vector<std::unordered_map<int, std::string>>(),bool CLEANCACHE=false);

	//double count_db(string table_name, SQLHANDLE &sqlstatementhandle, string db_name);

	bool IsFileExists(const std::string& filename);

	void ReadDirectQueries(std::string direct_file_name, const int DIM, std::vector<std::vector<double>> &o_direct_query_res);

	std::vector<double> GetDirectQueries(int qid, const std::vector<aqppp::Condition> &cur_query, const std::vector<std::vector<double>> &direct_query_res);

	static void fprintSample(FILE* out_file, std::vector<std::vector<double>> sample, std::string start_inf, std::string end_inf, int row_limit = INT_MAX);

}