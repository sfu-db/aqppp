//#include "stdafx.h"
#include "exp_groupby.h"

namespace Exp {
	string Exp_groupby::dirpath(void)
	{
		return "exp_result/exp_groupby/skew_s100_z2";
	}

	const Settings Exp_groupby::init_par()
	{
		Settings PAR;
		PAR.DB_NAME = "skew_s100_z2.dbo";
		PAR.SAMPLE_NAME = "sample4gb";
		PAR.SUB_SAMPLE_NAME = "sub_sample4gb";
		PAR.AGGREGATE_NAME = "L_EXTENDEDPRICE";
		PAR.CONDITION_NAMES = { "L_ORDERKEY","L_SUPPKEY","L_RETURNFLAG","L_LINESTATUS"};
		PAR.GROUPBY_LEN = 2;
		PAR.CREATE_DB_SAMPLES = false;
		return PAR;
	}

	void Exp_groupby::write_exp_par(FILE *par_file, EXP_PAR exp_par)
	{

		fprintf(par_file, "\n-----------experiment pars----------\n");
		fprintf(par_file, "COVER_DIRECT_RESULT_FILE\t%s\n", exp_par.COVER_DIRECT_RESULT_FILE ? "true" : "false");
		fprintf(par_file, "QUERY_DB_REAL_VALUE\t%s\n", exp_par.QUERY_DB_REAL_VALUE ? "true" : "false");
		fprintf(par_file, "EXP_NO_UNIFORM\t%s\n", exp_par.EXP_NO_UNIFORM ? "true" : "false");
		fprintf(par_file, "QUERY_NUM\t%d\n", exp_par.QUERY_NUM);
		fprintf(par_file, "MIN_QUERY_SELECTIVELY\t%f\n", exp_par.MIN_QUERY_SELECTIVELY);
		fprintf(par_file, "MAX_QUERY_SELECTIVELY\t%f\n", exp_par.MAX_QUERY_SELECTIVELY);
	}

	Exp_groupby::EXP_PAR Exp_groupby::get_exp_par()
	{
		EXP_PAR exp_par = EXP_PAR();
		exp_par.isMTL = false;
		exp_par.EXP_NO_UNIFORM = true;
		exp_par.QUERY_DB_REAL_VALUE = true;
		exp_par.COVER_DIRECT_RESULT_FILE = false;  //generally this should be false.
		exp_par.QUERY_NUM = 1000;
		exp_par.MIN_QUERY_SELECTIVELY = 0.005;
		exp_par.MAX_QUERY_SELECTIVELY = 0.05;
		return exp_par;
	}


	void Exp_groupby::gen_sample_rates_vec(const vector<vector<double>>& sample, const GpType4Int &gp_rates4sample,int GB_LEN,vector<double> &o_sample_rate)
	{
		int col_num = sample.size();
		int sample_row_num = sample[0].size();
		o_sample_rate = vector<double>(sample_row_num);
		for (int ri = 0; ri < sample_row_num; ri++)
		{
			vector<int> key = vector<int>();
			for (int ci = col_num - GB_LEN; ci < col_num; ci++)
			{
				key.push_back(sample[ci][ri]);
			}
			o_sample_rate[ri]=gp_rates4sample.at(key);
		}
	}

	double Exp_groupby::read_sample_str_format(SQLHANDLE &sqlconnectionhandle, vector<vector<string>> &o_table, string db_name, string table_name, string AGGREGATE_NAME, vector<string> CONDITION_NAMES)
	{
		const int MAX_CHAR_LEN = 300;
		SQLHANDLE sqlstatementhandle = NULL;
		if (SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle) != SQL_SUCCESS) return -1;
		double st = clock();
		o_table = vector<vector<string>>();
		string demand_str = AGGREGATE_NAME;
		for (int i = 0; i < CONDITION_NAMES.size(); i++)
			demand_str += ", CONVERT(varchar,  "+CONDITION_NAMES[i]+", 112)";
		string query = "SELECT " + demand_str + " FROM " + db_name + "." + table_name + ";";
		SQL_interface::sql_query(query, sqlstatementhandle);
		short int COL_NUM = 0;
		SQLNumResultCols(sqlstatementhandle, &COL_NUM);
		for (int i = 0; i < COL_NUM; i++)
			o_table.push_back(vector<string>());
		while (SQLFetch(sqlstatementhandle) == SQL_SUCCESS)
		{
			for (int ci = 0; ci < COL_NUM; ci++)
			{
				char data[MAX_CHAR_LEN] = {};
				SQLGetData(sqlstatementhandle, ci + 1, SQL_C_CHAR, data, MAX_CHAR_LEN, NULL);
				o_table[ci].push_back(data);
			}
		}
		double read_time = ((double)clock() - st) / CLOCKS_PER_SEC;
		SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle);
		return read_time;
	}

	void gen_stoi_mapping(const vector<vector<string>> &sample_str, const int group_len, vector<unordered_map<string, int>> &o_distinct_ids)
	{
		int col_num = sample_str.size();
		int row_num = sample_str[0].size();
		o_distinct_ids = vector<unordered_map<string, int>>();
		for (int ci = col_num - group_len; ci < col_num; ci++)
		{
			vector<string> cur_col = vector<string>(sample_str[ci]);
			sort(cur_col.begin(), cur_col.end());
			o_distinct_ids.push_back(unordered_map<string, int>());
			unordered_map<string, int> &cur_map = o_distinct_ids.back();
			for (int ri = 0; ri < row_num; ri++)
			{
				if (cur_map.find(cur_col[ri]) == cur_map.end())
				{
					cur_map.insert({ cur_col[ri], cur_map.size() });
				}
			}
		}
		assert(group_len == o_distinct_ids.size());
		return;
	}

	void gen_itos_mapping(const vector<unordered_map<string,int>> &stoi,vector<unordered_map<int,string>> &o_itos)
	{
		o_itos = vector<unordered_map<int, string>>(stoi.size());
		for (int i = 0; i < stoi.size(); i++)
		{
			const unordered_map<string, int> &cur_map = stoi[i];
			for (auto mapp : cur_map)
			{
				string str = mapp.first;
				int id = mapp.second;
				o_itos[i].insert({ id,str });
			}
		}
		return;
	}
	void trans_strsample_doublesample(const vector<unordered_map<string, int>> &group_ids,const vector<vector<string>> &sample_str, vector<vector<double>> &o_sample)
	{
		const int group_len = group_ids.size();
		int row_num = sample_str[0].size();
		int col_num = sample_str.size();
		o_sample = vector<vector<double>>(col_num);
		for (int ci = 0; ci < col_num; ci++)
		{
			o_sample[ci] = vector<double>(row_num);
			for (int ri = 0; ri < row_num; ri++)
			{
				if (ci >= col_num - group_len) o_sample[ci][ri] = group_ids[ci - col_num + group_len].at(sample_str[ci][ri]);
				else o_sample[ci][ri] = stod(sample_str[ci][ri]);
			}
		}
		return;
	}

	pair<double, double> Exp_groupby::read_str_samples(SQLHANDLE &sqlconnectionhandle, Settings& o_PAR, int run_num, vector<vector<string>>& o_sample_str, vector<vector<string>>& o_small_sample_str)
	{
		o_sample_str = vector <vector<string>>();
		o_small_sample_str = vector<vector<string>>();
		vector<double> read_sample_times = vector<double>();
		vector<double> read_small_sample_times = vector<double>();
		for (int i = 0; i < run_num; i++)
		{
			double cur_sample_time = read_sample_str_format(sqlconnectionhandle, o_sample_str, o_PAR.DB_NAME, o_PAR.SAMPLE_NAME, o_PAR.AGGREGATE_NAME, o_PAR.CONDITION_NAMES);
			double cur_small_smaple_time = read_sample_str_format(sqlconnectionhandle, o_small_sample_str, o_PAR.DB_NAME, o_PAR.SUB_SAMPLE_NAME, o_PAR.AGGREGATE_NAME, o_PAR.CONDITION_NAMES);
			read_sample_times.push_back(cur_sample_time);
			read_small_sample_times.push_back(cur_small_smaple_time);
		}
		double time_read_sample = Tool::get_percentile(read_sample_times, 0.5);
		double time_read_small_sample = Tool::get_percentile(read_small_sample_times, 0.5);
		o_PAR.SAMPLE_ROW_NUM = o_sample_str[0].size();
		return{ time_read_sample,time_read_small_sample };
	}

	pair<double, double> Exp_groupby::gb_create_db_samples(SQLHANDLE &sqlconnectionhandle, int seed, string db_name, string table_name, pair<double, double> sample_rates, pair<string, string> sample_names,const vector<string> &GROUP_CONDITIONS, pair<GpType4Str,GpType4Str> &o_group_rates)
	{
		double t1 = gb_create_db_sample(sqlconnectionhandle, seed + 100, db_name, table_name, sample_rates.first, sample_names.first,GROUP_CONDITIONS,o_group_rates.first);
		double t2 = gb_create_db_sample(sqlconnectionhandle, seed + 200, db_name, sample_names.first, sample_rates.second, sample_names.second,GROUP_CONDITIONS,o_group_rates.second);
		return { t1,t2 };
	}


	void Exp_groupby::get_group_sample_rates(SQLHANDLE &sqlconnectionhandle, double sample_rate,const vector<string> &GROUP_CONDITIONS, string db_name, string table_name, GpType4Str &o_group_rates)
	{
		o_group_rates = GpType4Str();
		GpType4Str group_sizes = GpType4Str();
		query_group(sqlconnectionhandle, GROUP_CONDITIONS, db_name, table_name, group_sizes);
		double table_size = 0;
		double group_number = group_sizes.size();
		for (auto t : group_sizes)
		{
			table_size += t.second;
		}
		double sample_row = table_size*sample_rate;

		int counter = 0;
		double left_rows = sample_row;
		for (auto t : group_sizes) //ask group_sizes need to be ordered by size. The order was guaranteed by query_group func.
		{
			double cur_spl_rows = left_rows / (group_number - counter);
			if (cur_spl_rows > t.second) cur_spl_rows = t.second;
			o_group_rates[t.first] = cur_spl_rows / double(t.second);
			left_rows -= cur_spl_rows;
			counter++;
		}
	}
	double Exp_groupby::gb_create_db_sample(SQLHANDLE &sqlconnectionhandle, int seed, string db_name, string table_name, double sample_rate, string sample_name,const vector<string> &GROUP_CONDITIONS, GpType4Str &o_group_rates)
	{
		get_group_sample_rates(sqlconnectionhandle, sample_rate,GROUP_CONDITIONS,db_name,table_name, o_group_rates);
		SQLHANDLE sqlstatementhandle = NULL;
		if (SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle) != SQL_SUCCESS) return -1;
		string sample_full_name = db_name + "." + sample_name;
		string table_full_name = db_name + "." + table_name;
		string drop_sample = "IF OBJECT_ID('" + sample_full_name + "', 'U') IS NOT NULL DROP TABLE " + sample_full_name + "; ";
		string create_sample = "";
		bool first_ele = true;
		for (auto gp : o_group_rates)
		{
			string st = "";
			for (int i = 0; i<GROUP_CONDITIONS.size(); i++)
			{
				st += " AND " + GROUP_CONDITIONS[i] + "='" + gp.first[i]+"'";
			}
			if (first_ele == true)
			{
				first_ele = false;
				create_sample += "SELECT *  INTO " + sample_full_name + " FROM " + table_full_name + " WHERE " + SQL_interface::cpt_rand_str(seed,gp.second) + st+";";
			}
			else
			{
				create_sample += "INSERT INTO " + sample_full_name + " SELECT * FROM " + table_full_name + " WHERE " + SQL_interface::cpt_rand_str(seed,gp.second) + st + ";";
			}
		}

		cout << drop_sample << endl;
		cout << create_sample << endl;
		SQL_interface::sql_query(drop_sample, sqlstatementhandle);
		double t1 = clock();
		SQL_interface::sql_query(create_sample, sqlstatementhandle);
		double create_sample_time = (clock() - t1) / CLOCKS_PER_SEC;
		SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle);
		return create_sample_time;
	}

	void Exp_groupby::query_group(SQLHANDLE &sqlconnectionhandle, const vector<string> &GROUP_CONDITIONS,string db_name,string table_name, GpType4Str &o_group_size)
	{
		o_group_size = GpType4Str();
		const int MAX_CHAR_LEN = 300;
		SQLHANDLE sqlstatementhandle = NULL;
		if (SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle) != SQL_SUCCESS) return;
		string cond = "";
		for (int i=0;i<GROUP_CONDITIONS.size();i++)
		{
			if (i != 0) cond += ", ";
			cond+= "CONVERT(varchar, "+ GROUP_CONDITIONS[i]+", 112)";
		}
		string query = "SELECT count(*) AS c, " + cond + "from " + db_name + "." + table_name + " GROUP BY " + cond + " ORDER BY c;";   //The order will affect a part of gb_create_db_sample and shouldn't be change.
		SQL_interface::sql_query(query, sqlstatementhandle);
		short int COL_NUM = 0;
		SQLNumResultCols(sqlstatementhandle, &COL_NUM);

		while (SQLFetch(sqlstatementhandle) == SQL_SUCCESS)
		{
			double gb_size = 0;
			SQLGetData(sqlstatementhandle, 1, SQL_C_DOUBLE, &gb_size, 0, NULL);
			vector<string> gb_ind = vector<string>();
			for (int ci = 1; ci < COL_NUM; ci++)
			{
				char data[MAX_CHAR_LEN];
				SQLGetData(sqlstatementhandle, ci + 1, SQL_C_CHAR, data, MAX_CHAR_LEN, NULL);
				gb_ind.push_back(data);
			}
			o_group_size[gb_ind] = gb_size;
		}

		SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle);
	}


	void Exp_groupby::trans_group_rates(const vector<unordered_map<string, int>> &distinct_ids, GpType4Str &rates4str, GpType4Int &o_rates4int)
	{
		o_rates4int = GpType4Int();
		for (auto gp : rates4str)
		{
			vector<int> key = vector<int>(gp.first.size());
			for (int i = 0; i < gp.first.size(); i++)
			{
				key[i] = distinct_ids[i].at(gp.first[i]);
			}
			o_rates4int.insert({ key, gp.second });
		}
		return;
	}


	void Exp_groupby::get_groups(const vector<vector<double>>& sample, const vector<vector<double>>& sub_sample, const GpType4Int &gp_rates4sample, const GpType4Int &gp_rates4sub_sample, const int GB_LEN, unordered_map<vector<int>, Group, vec_hash> &o_groups)
	{
		o_groups = unordered_map<vector<int>, Group, vec_hash>();
		//init group by
		for (auto gp : gp_rates4sample)
		{
			Group g = Group();
			g.sample = vector<vector<double>>(sample.size());
			g.small_sample = vector<vector<double>>(sub_sample.size());
			g.sample_rate = gp.second;
			g.small_sample_rate = gp_rates4sub_sample.at(gp.first);
			o_groups.insert({ gp.first,g });
		}
		int col_num = sample.size();
		int sample_row_num = sample[0].size();
		for (int ri = 0; ri < sample_row_num; ri++)
		{
			vector<int> key = vector<int>();
			for (int ci = col_num - GB_LEN; ci < col_num; ci++)
			{
				key.push_back(sample[ci][ri]);
			}
			Group &g = o_groups[key];
			for (int ci = 0; ci<col_num; ci++)
				g.sample[ci].push_back(sample[ci][ri]);
		}

		int sub_sample_row_num = sub_sample[0].size();
		for (int ri = 0; ri < sub_sample_row_num; ri++)
		{
			vector<int> key = vector<int>();
			for (int ci = col_num - GB_LEN; ci < col_num; ci++)
			{
				key.push_back(sub_sample[ci][ri]);
			}
			Group &g = o_groups[key];
			for (int ci = 0; ci<col_num; ci++)
				g.small_sample[ci].push_back(sub_sample[ci][ri]);
		}

		return;
	}

	


	int Exp_groupby::exp(SQLHANDLE& sqlconnectionhandle)
	{

		double s_exp = clock();
		EXP_PAR exp_par = get_exp_par();
		Settings PAR = init_par();
		FILE *info_file, *form_file, *par_file, *query_file,*group_form_file;
		Tool::mkdirRecursively(dirpath());
		fopen_s(&info_file, dirpath().append(string("/info.txt")).data(), "w");
		fopen_s(&form_file, dirpath().append(string("/form_res.txt")).data(), "w");
		fopen_s(&par_file, dirpath().append(string("/par.txt")).data(), "w");
		fopen_s(&query_file, dirpath().append(string("/queries.txt")).data(), "w");
		fopen_s(&group_form_file, dirpath().append(string("/group_form.txt")).data(), "w");

		

		/*----------creat sample, gen query.------------*/
		clock_t bt = clock();
		pair<double, double> time_create_db_samples;
		vector<string> GP_CONDITIONS = vector<string>(PAR.CONDITION_NAMES.end()-PAR.GROUPBY_LEN,PAR.CONDITION_NAMES.end());
		pair<GpType4Str, GpType4Str> gp_rates4str = pair<GpType4Str, GpType4Str>();
		if (PAR.CREATE_DB_SAMPLES)
		{
			cout << "creating sample.." << endl;
			time_create_db_samples = gb_create_db_samples(sqlconnectionhandle, PAR.RAND_SEED, PAR.DB_NAME, PAR.TABLE_NAME, { PAR.SAMPLE_RATE,PAR.SUB_SAMPLE_RATE }, { PAR.SAMPLE_NAME,PAR.SUB_SAMPLE_NAME }, GP_CONDITIONS, gp_rates4str);

		}
		else {
			cout << "get_groups_sample_rates...." << endl;
			get_group_sample_rates(sqlconnectionhandle, PAR.SAMPLE_RATE,GP_CONDITIONS, PAR.DB_NAME, PAR.TABLE_NAME, gp_rates4str.first);
			get_group_sample_rates(sqlconnectionhandle, PAR.SUB_SAMPLE_RATE,GP_CONDITIONS, PAR.DB_NAME, PAR.SAMPLE_NAME, gp_rates4str.second);
		}
			
			cout << "time create samples: " << time_create_db_samples.first << " " << time_create_db_samples.second << endl;
		/*----------end creat sample, gen query.---------*/
		double time_spl_offline = time_create_db_samples.first;

		/*---------read sample into memorary-----------*/
		double t1 = clock();
		vector <vector<string>> sample_str;
		vector <vector<string>> small_sample_str;
		pair<double, double> read_sample_times = read_str_samples(sqlconnectionhandle, PAR, 10, sample_str, small_sample_str);
		double time_read_sample = read_sample_times.first;
		double time_read_small_sample = read_sample_times.second;
		DistId distinct_stoi;
		vector<vector<double>> sample;
		vector<vector<double>> sub_sample;
		gen_stoi_mapping(sample_str, PAR.GROUPBY_LEN, distinct_stoi);
		vector<unordered_map<int, string>> distinct_itos;
		gen_itos_mapping(distinct_stoi,distinct_itos);

		trans_strsample_doublesample(distinct_stoi, sample_str, sample);
		trans_strsample_doublesample(distinct_stoi, small_sample_str, sub_sample);

		GpType4Int gp_rates_sample;
		GpType4Int gp_rates_subsample;
		trans_group_rates(distinct_stoi,gp_rates4str.first, gp_rates_sample);
		trans_group_rates(distinct_stoi,gp_rates4str.second, gp_rates_subsample);

		vector<double> sample_prob;
		vector<double> sub_sample_prob;
		gen_sample_rates_vec(sample, gp_rates_sample, PAR.GROUPBY_LEN, sample_prob);
		gen_sample_rates_vec(sub_sample, gp_rates_subsample, PAR.GROUPBY_LEN, sub_sample_prob);


		unordered_map<vector<int>, Group, vec_hash> groups;
		get_groups(sample, sub_sample, gp_rates_sample, gp_rates_subsample, PAR.GROUPBY_LEN, groups);




		fprintf(info_file, "sample_row_num:%f\n", (double)sample[0].size());
		cout << "sub_sample size:" << sub_sample[0].size() << endl;
		write_par(par_file, PAR);
		write_exp_par(par_file, exp_par);
		fclose(par_file);

		/*----------transfer sample into ca_sample--------*/
		double t3 = clock();
		vector<vector<CA>> CAsample = vector<vector<CA>>();
		Tool::trans_sample(sample, CAsample);
		double time_trans_sample = (clock() - t3) / CLOCKS_PER_SEC;
		/*------------------------------------------------*/

		cout << "sample size:" << sample[0].size() << endl;
		for (int i = 0; i < CAsample.size(); i++)
		{
			cout << "dim:" << i << " distinct value:" << CAsample[i].size() << endl;
		}


		/*--------for no_uniform distribution: find materalize points and compute materlize result--------*/
		vector<vector<CA>> NF_mtl_points;
		MTL_STRU	NF_mtl_res = MTL_STRU();
		double time_NF_mtl_res = 0;
		double time_NF_hybrid_offline = 0;
		double time_dist_mtl = 0;
		vector<int> mtl_nums;
		double time_build_err_profile;
		cout << "start mtl..." << endl;
		if (exp_par.EXP_NO_UNIFORM == true)
		{
			double t1 = clock();
			Dist_MTL_points(PAR, false).dist_mtl_points(CAsample, mtl_nums);
			time_dist_mtl = (clock() - t1) / CLOCKS_PER_SEC;

			double t2 = clock();
			NF_mtl_points = vector<vector<CA>>();
			vector<double> max_errs;
			vector<int> iter_nums;
			Climb_method(PAR.SAMPLE_ROW_NUM, PAR.SAMPLE_RATE, PAR.CI_INDEX, PAR.NF_MAX_ITER, PAR.INIT_DISTINCT_EVEN).mtl_NF(CAsample, mtl_nums, NF_mtl_points, max_errs, iter_nums);
			double time_NF_find_mtl_point = (clock() - t2) / CLOCKS_PER_SEC;
			fprintf(info_file, "sample size: %d\n", sample[0].size());
			for (int i = 0; i < CAsample.size(); i++)
			{
				cout << "dim:" << i << " mtl_num:" << mtl_nums[i] << endl;
				fprintf(info_file, "dim:%d\tdistinct_value:%d\tmtl_num:%d\tmax_err:%f\tclimb_iter_num:%d\n", i, CAsample[i].size(), mtl_nums[i], max_errs[i], iter_nums[i]);
			}

			double time_NF_mtl_res = 0;
			if (exp_par.isMTL) time_NF_mtl_res=Pre_comp(PAR.DB_NAME, PAR.TABLE_NAME, PAR.AGGREGATE_NAME, PAR.CONDITION_NAMES).get_mtl_res(NF_mtl_points, sqlconnectionhandle, NF_mtl_res, "sum", distinct_stoi);

			time_NF_hybrid_offline = time_dist_mtl + time_create_db_samples.first + time_create_db_samples.second + time_trans_sample + time_NF_find_mtl_point + time_NF_mtl_res;
		}
		/*-------------------------------------------------------------------*/





		cout << "gen query" << endl;
		/*
		vector<vector<double>> tp_sample = vector<vector<double>>();
		for (int ci = 0; ci < sample.size() - PAR.GROUPBY_LEN; ci++)
		{
			tp_sample.push_back(sample[ci]);
		}
		vector<vector<CA>> tp_casample;
		Tool::trans_sample(tp_sample, tp_casample);
		vector<vector<Condition>> user_queries = vector<vector<Condition>>();
		Tool::gen_user_quires(tp_sample, tp_casample, PAR.RAND_SEED, exp_par.QUERY_NUM, { exp_par.MIN_QUERY_SELECTIVELY,exp_par.MAX_QUERY_SELECTIVELY }, user_queries, groups);
		cout << "gen query num: " << user_queries.size() << endl;
		*/


		vector<vector<Condition>> user_queries = vector<vector<Condition>>();
		int query_num= exp_par.QUERY_NUM / groups.size();
		for (auto gpit : groups)
		{
			vector<vector<CA>> tp_casample;
			Tool::trans_sample(gpit.second.sample, tp_casample);
			vector<vector<Condition>> tp_user_queries = vector<vector<Condition>>();
			Tool::gen_user_quires(gpit.second.sample, tp_casample, PAR.RAND_SEED, query_num, { exp_par.MIN_QUERY_SELECTIVELY,exp_par.MAX_QUERY_SELECTIVELY }, tp_user_queries,groups);
			for (auto v : tp_user_queries)
			{
				user_queries.push_back(v);
			}

		}


		double counter = 0;
		double time_spl_online = 0;
		double time_aqpp_online = 0;
		double time_direct_online = 0;

		fprintf(form_file, "query_id\t group_id\t spl_est\t spl_ci\t aqpp_est\t aqpp_ci\t real_value\t time_spl\t time_aqpp\t time_direct\t error_spl\t error_aqpp\n");
		fprintf(group_form_file, "query_id\t spl_avg_err\t aqpp_avg_err\t spl_max_err\t aqpp_max_err\n");

		double before_query_time = ((double)clock() - (double)bt) / CLOCKS_PER_SEC;
		vector<double> spl_errors4avg = vector<double>();
		vector<double> spl_errors4max = vector<double>();
		vector<double> aqpp_errors4avg = vector<double>();
		vector<double> aqpp_errors4max = vector<double>();
		map<vector<int>, vector<double>> spl_errors4gp = map<vector<int>, vector<double>>();
		map<vector<int>, vector<double>> aqpp_errors4gp = map<vector<int>, vector<double>>();



		/*--------------make query, and save result----------------*/
		cout << "make query now, query num:" << user_queries.size() << endl;
		for (int query_id = 0; query_id < user_queries.size(); query_id++)
		{
			vector<double> inner_spl_errors = vector<double>();
			vector<double> inner_aqpp_errors = vector<double>();
			int group_id = -1;
			for (auto gpit : groups)
			{
				group_id++;
				auto &g = gpit.second;
				cout << "query id " << query_id << endl;
				vector<Condition> cur_q = user_queries[query_id];
				/*
				for (int i = 0; i < gpit.first.size(); i++)
				{
					Condition cd = Condition();
					cd.lb = gpit.first[i];
					cd.ub = gpit.first[i];
					cur_q.push_back(cd);
				}
				*/

				fprintf(query_file, "query_id:%d  condition: ", query_id);
				for (auto ele : cur_q)
				{
					fprintf(query_file, "(%.2f, %.2f) ", ele.lb, ele.ub);
				}
				fprintf(query_file, "\n");

				double t1 = clock();
				pair<double, double> spl_sumci = SPL_method(g.sample_rate, PAR.CI_INDEX).sampling_sum_query(g.sample, cur_q);
				double time_spl = (clock() - t1) / CLOCKS_PER_SEC;

				if (DB_equal(spl_sumci.first, 0.0)) continue;   //if not found in sample, then skip this query. 
				pair<double, double> aqpp_sumci;
				double time_aqpp = 0;
				if (exp_par.EXP_NO_UNIFORM == true)
				{
					double t1 = clock();
					aqpp_sumci = Aqpp(g.sample[0].size(), g.sample_rate, PAR.CI_INDEX).hybird_sum_query_small_sample(query_id, info_file, g.sample, g.small_sample, NF_mtl_points, NF_mtl_res, cur_q);
					time_aqpp = ((double)clock() - (double)t1) / CLOCKS_PER_SEC;
				}

				double t2 = clock();

				double real_value = query_real_value(cur_q, PAR.TABLE_NAME, sqlconnectionhandle, PAR, "sum", distinct_itos);
				double time_direct = (clock() - t2) / CLOCKS_PER_SEC;

				cout << "real_value:" << real_value << " time_direct:" << time_direct << endl;

				double error_spl = real_value > 0 ? spl_sumci.second / real_value : 0;
				double error_aqpp = real_value > 0 ? aqpp_sumci.second / real_value : 0;

				counter++;
				time_spl_online += time_spl;
				time_aqpp_online += time_aqpp;
				time_direct_online += time_direct;
				inner_spl_errors.push_back(error_spl);
				inner_aqpp_errors.push_back(error_aqpp);

				if (spl_errors4gp.find(gpit.first) != spl_errors4gp.end()) spl_errors4gp[gpit.first].push_back(error_spl); else spl_errors4gp[gpit.first] = vector<double>();

				if(aqpp_errors4gp.find(gpit.first) != aqpp_errors4gp.end()) aqpp_errors4gp[gpit.first].push_back(error_aqpp);else aqpp_errors4gp[gpit.first] = vector<double>();



				fprintf(form_file, "%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", query_id, group_id,spl_sumci.first, spl_sumci.second,
					aqpp_sumci.first, aqpp_sumci.second, real_value, time_spl, time_aqpp, time_direct, error_spl,
					error_aqpp);
			}

			double avg_spl_err_in_gb=Tool::get_avg(inner_spl_errors);
			double max_spl_err_in_gb=Tool::get_percentile(inner_spl_errors, 1.0);
			double avg_aqpp_err_in_gb = Tool::get_avg(inner_aqpp_errors);
			double max_aqpp_err_in_gb = Tool::get_percentile(inner_aqpp_errors, 1.0);

			spl_errors4avg.push_back(avg_spl_err_in_gb);
			spl_errors4max.push_back(max_spl_err_in_gb);
			aqpp_errors4avg.push_back(avg_aqpp_err_in_gb);
			aqpp_errors4max.push_back(max_aqpp_err_in_gb);
			fprintf(group_form_file, "%d\t%d\t%f\t%f\t%f\t%f\n",query_id, avg_spl_err_in_gb, avg_aqpp_err_in_gb, max_spl_err_in_gb, max_aqpp_err_in_gb);
		}
		/*-------------------------------------------------------------*/

		fclose(form_file);
		fclose(query_file);
		fclose(group_form_file);


		FILE* final_gp_file;
		fopen_s(&final_gp_file, (dirpath() + "/final_gp_file.txt").c_str(),"w");
		fprintf(final_gp_file, "group_name\t median_spl_err\t median_aqpp_err\t avg_spl_err\t avg_aqpp_err\t spl_95err\t aqpp_95err\n");
		for (auto gpit : groups)
		{
			vector<int> ind = gpit.first;
			string group_name = "";
			for (int i = 0; i < ind.size(); i++)
			{
				group_name+= (distinct_itos[i][ind[i]]+"|");
			}
			double median_spl_err = Tool::get_percentile(spl_errors4gp[ind],0.5);
			double median_aqpp_err = Tool::get_percentile(aqpp_errors4gp[ind], 0.5);
			double avg_spl_err = Tool::get_avg(spl_errors4gp[ind]);
			double avg_aqpp_err= Tool::get_avg(aqpp_errors4gp[ind]);
			double spl_95err = Tool::get_percentile(spl_errors4gp[ind], 0.95);
			double aqpp_95err= Tool::get_percentile(aqpp_errors4gp[ind], 0.95);
			fprintf(final_gp_file, "%s\t%f\t%f\t%f\t%f\t%f\t%f\n", group_name.c_str(),median_spl_err, median_aqpp_err, avg_spl_err, avg_aqpp_err, spl_95err, aqpp_95err);
		}

		fclose(final_gp_file);

		double avg_spl_error4avg = Tool::get_avg(spl_errors4avg);
		double avg_aqpp_error4avg = Tool::get_avg(aqpp_errors4avg);
		double median_spl_error4avg = Tool::get_percentile(spl_errors4avg, 0.5);
		double median_aqpp_error4avg = Tool::get_percentile(aqpp_errors4avg, 0.5);
		double spl_95p_err4avg = Tool::get_percentile(spl_errors4avg, 0.95);
		double aqpp_95p_err4avg = Tool::get_percentile(aqpp_errors4avg, 0.95);


		double avg_spl_error4max = Tool::get_avg(spl_errors4max);
		double avg_aqpp_error4max = Tool::get_avg(aqpp_errors4max);
		double median_spl_error4max = Tool::get_percentile(spl_errors4max, 0.5);
		double median_aqpp_error4max = Tool::get_percentile(aqpp_errors4max, 0.5);
		double spl_95p_err4max = Tool::get_percentile(spl_errors4max, 0.95);
		double aqpp_95p_err4max = Tool::get_percentile(aqpp_errors4max, 0.95);


		double tot_time = ((double)clock() - bt) / CLOCKS_PER_SEC;

		fprintf(info_file, "\n");
		fprintf(info_file, "gen query_num: %d\n", user_queries.size());
		fprintf(info_file, "real query_num: %d\n", (int)counter);
		fprintf(info_file, "sampling offline time: %f\n", time_spl_offline);
		fprintf(info_file, "NF hybird offline time: %f\n", time_NF_hybrid_offline);
		fprintf(info_file, "\n");

		fprintf(info_file, "avg spl_online_time: %f\n", time_spl_online / counter + time_read_sample);
		fprintf(info_file, "avg NF_hybrid_online_time: %f\n", time_aqpp_online / counter + time_read_sample + time_read_small_sample);
		fprintf(info_file, "avg direct_online_time: %f\n", time_direct_online / counter);
		fprintf(info_file, "before_query_time: %f\n", before_query_time);
		fprintf(info_file, "total_run_time: %f\n", tot_time);
		fprintf(info_file, "\n");


		fprintf(info_file, "avg_spl_error4avg: %f%%\n", 100 * avg_spl_error4avg);
		fprintf(info_file, "avg_aqpp_error4avg: %f%%\n", 100 * avg_aqpp_error4avg);
		fprintf(info_file, "median_spl_error4avg: %f%%\n", 100 * median_spl_error4avg);
		fprintf(info_file, "median_aqpp_error4avg: %f%%\n", 100 * median_aqpp_error4avg);
		fprintf(info_file, "spl_95p_error4avg: %f%%\n", 100 * spl_95p_err4avg);
		fprintf(info_file, "aqpp_95p_error4avg: %f%%\n", 100 * aqpp_95p_err4avg);
		fprintf(info_file, "\n");


		fprintf(info_file, "avg_spl_error4max: %f%%\n", 100 * avg_spl_error4max);
		fprintf(info_file, "avg_aqpp_error4max: %f%%\n", 100 * avg_aqpp_error4max);
		fprintf(info_file, "median_spl_error4max: %f%%\n", 100 * median_spl_error4max);
		fprintf(info_file, "median_aqpp_error4max: %f%%\n", 100 * median_aqpp_error4max);
		fprintf(info_file, "spl_95p_error4max: %f%%\n", 100 * spl_95p_err4max);
		fprintf(info_file, "aqpp_95p_error4max: %f%%\n", 100 * aqpp_95p_err4max);
		fprintf(info_file, "\n");


		fprintf(info_file, "create sample in db time: %f\n", time_create_db_samples.first);
		fprintf(info_file, "create sub_sample in db time: %f\n", time_create_db_samples.second);
		fprintf(info_file, "read sample from db: %f\n", time_read_sample);
		fprintf(info_file, "read sub_sample from db: %f\n", time_read_small_sample);
		fprintf(info_file, "NF dist mtl data tot time: %f\n", time_dist_mtl);
		fprintf(info_file, "NF cpt mtl res time: %f\n", time_NF_mtl_res);
		fprintf(info_file, "\n");



		double time_exp = (clock() - s_exp) / CLOCKS_PER_SEC;
		fprintf(info_file, "exp_time: %f", time_exp);
		fclose(info_file);
		cout << "exp_cph end " << endl;
		return 0;

	}
}
