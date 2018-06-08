#pragma once
#include<vector>
#include<string>
#include<time.h>
#include<iostream>
#include<assert.h>
#include <windows.h>
#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>
#include<map>

namespace aqppp {
	class SqlInterface
	{
	public:

		static int SqlInterface::ConnectDb(SQLHANDLE &sqlconnectionhandle, std::string dsn, std::string user, std::string pwd);
		static void ShowError(unsigned int handletype, const SQLHANDLE& handle);
		static void SqlInterface::MakeSqlConnection(std::string odbc_name, std::string user_name, std::string pwd, SQLHANDLE &sqlconnectionhandle);
		/*
		return a query result of given string query.
		*/
		static void SqlQuery(std::string query, SQLHANDLE &sqlstatementhandle);

		static std::string ComputeRandStr(int seed, double sample_rate, std::vector<std::string> num_dim4rand = std::vector<std::string>(), std::vector<std::string> ctg_dim4rand= std::vector<std::string>());
		/*create sample and small_sample table in MySQL database.
		*/
		static std::pair<double, double> CreateDbSamples(SQLHANDLE &sqlconnectionhandle, int seed, std::string db_name, std::string table_name, std::pair<double, double> sample_rates, std::pair<std::string, std::string> sample_names, std::vector<std::string> num_dim4rand = std::vector<std::string>(), std::vector<std::string> ctg_dim4rand = std::vector<std::string>());
		static double CreateDbSample(SQLHANDLE &sqlconnectionhandle, int seed, std::string db_name, std::string table_name, double sample_rate, std::string sample_name, std::vector<std::string> num_dim4rand = std::vector<std::string>(), std::vector<std::string> ctg_dim4rand = std::vector<std::string>());

		static double ReadDb(SQLHANDLE &sqlconnectionhandle, std::vector<std::vector<double>> &o_table, std::string db_name, std::string table_name, std::string AGGREGATE_NAME, std::vector<std::string> CONDITION_NAMES);
		static double Column2Numeric(SQLHANDLE &sqlstatementhandle, int col_id, std::string col_name);
		static std::string ReadTableStr(std::string db_name, std::string table_name, std::string AGGREGATE_NAME, std::vector<std::string> CONDITION_NAMES);

	};
}
