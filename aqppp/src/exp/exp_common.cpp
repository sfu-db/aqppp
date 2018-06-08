//#include "stdafx.h"
#include "exp_common.h"

namespace expDemo {

	void WritePar(FILE *par_file, aqppp::Settings par)
	{
		fprintf(par_file, "-----------setting pars------------\n");
		fprintf(par_file, "DB_NAME\t%s\n", par.DB_NAME.c_str());
		fprintf(par_file, "CREATE_DB_SAMPLES\t%s\n", par.CREATE_DB_SAMPLES ? "true" : "false");
		fprintf(par_file, "SAMPLE_RATE\t%f\n", par.SAMPLE_RATE);
		fprintf(par_file, "SUB_SAMPLE_RATE\t%f\n", par.SUB_SAMPLE_RATE);
		fprintf(par_file, "AGGREGATE_NAME\t%s\n", par.AGGREGATE_NAME.c_str());
		fprintf(par_file, "CONDITION_NAMES\t");
		for (auto st : par.CONDITION_NAMES)
		{
			fprintf(par_file, " %s", st.c_str());
		}
		fprintf(par_file, "\n");
		fprintf(par_file, "CI_INDEX\t%f\n", par.CI_INDEX);
		fprintf(par_file, "NF_MAX_ITER\t%d\n", par.NF_MAX_ITER);
		fprintf(par_file, "ALL_MTL_POINTS\t%d\n", par.ALL_MTL_POINTS);
		fprintf(par_file, "EP_PIECE_NUM\t%d\n", par.EP_PIECE_NUM);
		fprintf(par_file, "INIT_DISTINCT_EVEN\t%s\n", par.INIT_DISTINCT_EVEN ? "true" : "false");
		fprintf(par_file, "RAND_SEED\t%d\n", par.RAND_SEED);
		fprintf(par_file, "SAMPLE_ROW_NUM\t%d\n", par.SAMPLE_ROW_NUM);
		fprintf(par_file, "SAMPLE_NAME\t%s\n", par.SAMPLE_NAME.c_str());
		fprintf(par_file, "SUB_SAMPLE_NAME\t%s\n", par.SUB_SAMPLE_NAME.c_str());
		fprintf(par_file, "----------------------\n");
	}



	std::pair<double, double> ReadSamples(SQLHANDLE &sqlconnectionhandle,aqppp::Settings& o_PAR,int run_num, std::vector<std::vector<double>>& o_sample, std::vector<std::vector<double>>& o_small_sample)
	{
		o_sample = std::vector<std::vector<double>>();
		o_small_sample = std::vector<std::vector<double>>();
		std::vector<double> read_sample_times = std::vector<double>();
		std::vector<double> read_small_sample_times = std::vector<double>();
		for (int i = 0;i < run_num;i++)
		{
			double cur_sample_time = aqppp::SqlInterface::ReadDb(sqlconnectionhandle, o_sample, o_PAR.DB_NAME, o_PAR.SAMPLE_NAME, o_PAR.AGGREGATE_NAME, o_PAR.CONDITION_NAMES);
			double cur_small_smaple_time = aqppp::SqlInterface::ReadDb(sqlconnectionhandle, o_small_sample, o_PAR.DB_NAME, o_PAR.SUB_SAMPLE_NAME, o_PAR.AGGREGATE_NAME, o_PAR.CONDITION_NAMES);
			read_sample_times.push_back(cur_sample_time);
			read_small_sample_times.push_back(cur_small_smaple_time);
		}
		double time_read_sample = aqppp::Tool::get_percentile(read_sample_times, 0.5);
		double time_read_small_sample = aqppp::Tool::get_percentile(read_small_sample_times, 0.5);
		o_PAR.SAMPLE_ROW_NUM = o_sample[0].size();
		return{ time_read_sample,time_read_small_sample };
	}

	double QueryRealValue(const std::vector<aqppp::Condition>& cur_q, std::string table_name, SQLHANDLE &sqlconnectionhandle, const aqppp::Settings PAR, std::string agg_action, std::vector<std::unordered_map<int, std::string>> &distinct_itos, bool CLEANCACHE) {
		SQLHANDLE sqlstatementhandle = NULL;
		if (SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle) != SQL_SUCCESS) return -1;
		if (CLEANCACHE) aqppp::SqlInterface::SqlQuery("DBCC DROPCLEANBUFFERS;", sqlstatementhandle);
		std::string bk = "";
		int GP_LEN = distinct_itos.size();
		int DIM = cur_q.size();
		for (int ci = 0; ci < DIM; ci++)
		{
			std::string t = " AND ";
			if (ci != 0) bk += t;
			std::string lb_str = "";
			std::string ub_str = "";
			int id = ci - (DIM - GP_LEN);
			double lb_data = cur_q[ci].lb;
			double ub_data = cur_q[ci].ub;
			if (id >= 0)
			{
				lb_str = distinct_itos[id].at(long int(round(lb_data)));
				ub_str = distinct_itos[id].at(long int(round(ub_data)));
			}
			else
			{
				lb_str = aqppp::DoubleEqual(round(lb_data),lb_data)? std::to_string(long long int(round(lb_data))): std::to_string(lb_data);
				ub_str = aqppp::DoubleEqual(round(ub_data), ub_data) ? std::to_string(long long int(round(ub_data))) : std::to_string(ub_data);
				if (PAR.CONDITION_NAMES[ci] == "pickup_time" || PAR.CONDITION_NAMES[ci] == "dropoff_time")
				{
					while (lb_str.size() < 6) lb_str = "0" + lb_str;
					while (ub_str.size() < 6) ub_str = "0" + ub_str;
					std::string tp_lb = "";
					std::string tp_ub = "";
					for (int i = 0; i < 6; i++)
					{
						tp_lb += lb_str[i];
						tp_ub += ub_str[i];
						if ((i != 5) && ((i+1) % 2 == 0))
						{
							tp_lb += ":";
							tp_ub += ":";
						}
					}
					lb_str = tp_lb;
					ub_str = tp_ub;
				}

			}
			lb_str = "'" + lb_str + "'"; //' is for catagory data and date. It is also works for numeric data.
			ub_str = "'" + ub_str + "'";
			bk += PAR.CONDITION_NAMES[ci] + ">=" + lb_str + " AND " + PAR.CONDITION_NAMES[ci] + "<=" + ub_str;
		}
		std::string s = "select " + agg_action + "(" + PAR.AGGREGATE_NAME + ") from " + PAR.DB_NAME + "." + table_name + " where " + bk + ";";
		std::cout << s << std::endl;
		aqppp::SqlInterface::SqlQuery(s, sqlstatementhandle);
		double value = 0;
		while (SQLFetch(sqlstatementhandle) == SQL_SUCCESS)
		{
			SQLGetData(sqlstatementhandle, 1, SQL_C_DOUBLE, &value, 0, NULL);
		}

		SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle);
		return value;
	}

	/*
	double count_db(string table_name, MYSQL* conn, string db_name) {
		string s = "select  count(*) from " + db_name + "." + table_name + ";";
		//cout << s << endl;
		MYSQL_RES* res = SqlInterface::SqlQuery(s, conn);
		double value = 0;
		MYSQL_ROW row = mysql_fetch_row(res);
		if (row[0] != NULL) value = atof(row[0]);
		mysql_free_result(res);
		return value;
	}
	*/


	bool IsFileExists(const std::string& filename) {
		std::ifstream ifile(filename.c_str());
		return (bool)ifile;
	}

	void ReadDirectQueries(std::string direct_file_name, const int DIM, std::vector<std::vector<double>> &o_direct_query_res)
	{
		o_direct_query_res = std::vector<std::vector<double>>();
		std::ifstream infile(direct_file_name);
		double tp = -1;
		int counter = 0;
		while (infile >> tp)
		{
			if (counter % (4 + 2 * DIM) == 0) o_direct_query_res.push_back(std::vector<double>());
			o_direct_query_res[o_direct_query_res.size() - 1].push_back(tp);
			counter++;
		}
		infile.close();
		return;
	}
	std::vector<double> GetDirectQueries(int qid, const std::vector<aqppp::Condition> &cur_query, const std::vector<std::vector<double>> &direct_query_res)
	{
		if (qid >= direct_query_res.size()) return{ -1,-1,-1 };
		const int DIM = cur_query.size();
		if (direct_query_res[qid].size() != 4 + 2 * DIM) return{ -1,-1,-1 };
		if (!aqppp::DoubleEqual(direct_query_res[qid][0], qid)) return{ -1,-1,-1 };   //query_id
		for (int i = 0;i < DIM;i++)
		{
			if (cur_query[i].lb != direct_query_res[qid][1 + i * 2]) return{ -1,-1,-1 };
			if (cur_query[i].ub != direct_query_res[qid][2 + i * 2]) return{ -1,-1,-1 };
		}
		std::vector<double> res = std::vector<double>(3);
		res[0] = direct_query_res[qid][1 + 2 * DIM];//real value
		res[1] = direct_query_res[qid][2 + 2 * DIM];//selectively
		res[2] = direct_query_res[qid][3 + 2 * DIM];//direct time
		return res;
	}

	static void fprintSample(FILE* out_file, std::vector<std::vector<double>> sample, std::string start_inf, std::string end_inf, int row_limit)
	{
		fprintf(out_file, "\n-----------%s--------------\n", start_inf);
		const int ROW_NUM = sample[0].size();
		const int COL_NUM = sample.size();
		for (int ri = 0; ri < min(row_limit, ROW_NUM); ri++)
		{
			for (int ci = 0; ci < COL_NUM; ci++)
				fprintf(out_file, "%f\t", sample[ci][ri]);
			fprintf(out_file, "\n");
		}
		fprintf(out_file, "-----------%s--------------\n\n", end_inf);
	}

}