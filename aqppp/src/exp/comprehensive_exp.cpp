//#include "stdafx.h"
#include "comprehensive_exp.h"

namespace expDemo {
	std::string ComprehensiveExp::GetExpRootPath(void)
		{
			return "exp_result/exp_cph";
		}

	 const aqppp::Settings ComprehensiveExp::InitPar4DefaultSample()
	 {
		 aqppp::Settings PAR;
		 PAR.DB_NAME = "skew_s10_z2.dbo";
		 PAR.SAMPLE_NAME = "cpp_sample";
		 PAR.SAMPLE_RATE = 0.003;
		 PAR.ALL_MTL_POINTS = 20000;
		 PAR.CREATE_DB_SAMPLES = false;
		 return PAR;
	 }



	 const aqppp::Settings ComprehensiveExp::InitPar4LargeSample()
		{
			aqppp::Settings PAR;
			PAR.CREATE_DB_SAMPLES = true;
			PAR.SAMPLE_RATE = 0.04;
			PAR.SUB_SAMPLE_RATE = 0.01;
			PAR.SAMPLE_NAME = "sample_4per";
			PAR.SUB_SAMPLE_NAME = "sub_sample_4per";
			return PAR;
		}

	 void ComprehensiveExp::WriteExpPar(FILE *par_file, ExpPar exp_par)
		{

			fprintf(par_file, "\n-----------experiment pars----------\n");
			fprintf(par_file, "COVER_DIRECT_RESULT_FILE\t%s\n", exp_par.COVER_DIRECT_RESULT_FILE ? "true" : "false");
			fprintf(par_file, "USE_INDEX\t%s\n", exp_par.USE_DB_INDEX ? "true" : "false");
			fprintf(par_file, "DROP_INDEX_BEFORE_CREATE\t%s\n", exp_par.DROP_INDEX_BEFORE_CREATE ? "true" : "false");
			fprintf(par_file, "QUERY_DB_REAL_VALUE\t%s\n", exp_par.QUERY_DB_REAL_VALUE ? "true" : "false");
			fprintf(par_file, "QUERY_DB_SELECTIVELY\t%s\n", exp_par.QUERY_DB_SELECTIVELY ? "true" : "false");
			fprintf(par_file, "EXP_NO_UNIFORM\t%s\n", exp_par.isMTL ? "true" : "false");
			fprintf(par_file, "QUERY_NUM\t%d\n", exp_par.QUERY_NUM);
			fprintf(par_file, "MIN_QUERY_SELECTIVELY\t%f\n", exp_par.MIN_QUERY_SELECTIVELY);
			fprintf(par_file, "MAX_QUERY_SELECTIVELY\t%f\n", exp_par.MAX_QUERY_SELECTIVELY);
		}

	 ComprehensiveExp::ExpPar ComprehensiveExp::GetExpPar()
		{
		 ExpPar exp_par = ExpPar();
		  exp_par.isMTL = false;
		  exp_par.DROP_INDEX_BEFORE_CREATE = false;
		  exp_par.QUERY_DB_REAL_VALUE = true;
		  exp_par.QUERY_DB_SELECTIVELY = false;
		  exp_par.COVER_DIRECT_RESULT_FILE = false;  //generally this should be false.
		  exp_par.QUERY_NUM = 300;
		  exp_par.MIN_QUERY_SELECTIVELY = 0.005;
		  exp_par.MAX_QUERY_SELECTIVELY = 0.05;
		  return exp_par;
		}


	 int ComprehensiveExp::Exp(SQLHANDLE& sqlconnectionhandle)
	 {
		 RunOnce(sqlconnectionhandle, InitPar4DefaultSample());
		// RunOnce(sqlconnectionhandle, InitPar4LargeSample());
		 return 0;
	 }
	 int ComprehensiveExp::RunOnce(SQLHANDLE& sqlconnectionhandle,aqppp::Settings PAR)
		{
		 std::string root_path = GetExpRootPath() + "/samplerate" + std::to_string(PAR.SAMPLE_RATE);
			double s_exp = clock();
			ExpPar exp_par = GetExpPar();
			FILE *info_file, *form_file, *par_file, *query_file, *direct_query_file;
			aqppp::Tool::MkDirRecursively(root_path);
			fopen_s(&info_file, (root_path+"/info.txt").data(), "w");
			fopen_s(&form_file, (root_path+"/form_res.txt").data(), "w");
			fopen_s(&par_file, (root_path+"/par.txt").data(), "w");
			fopen_s(&query_file, (root_path+"/queries.txt").data(), "w");
			
			

			/*----------creat sample, gen query.------------*/
			clock_t bt = clock();
			std::pair<double, double> time_create_db_samples;
			if (PAR.CREATE_DB_SAMPLES) time_create_db_samples = aqppp::SqlInterface::CreateDbSamples(sqlconnectionhandle, PAR.RAND_SEED, PAR.DB_NAME,PAR.TABLE_NAME, { PAR.SAMPLE_RATE,PAR.SUB_SAMPLE_RATE }, { PAR.SAMPLE_NAME,PAR.SUB_SAMPLE_NAME });
			std::cout << "time create samples: " << time_create_db_samples.first << " " << time_create_db_samples.second << std::endl;
			/*----------end creat sample, gen query.---------*/

			/*---------read sample into memorary-----------*/
			double t1 = clock();
			std::vector <std::vector<double>> sample;
			std::vector <std::vector<double>> small_sample;
			std::pair<double,double> read_sample_times=ReadSamples(sqlconnectionhandle, PAR, 10, sample, small_sample);
			double time_read_sample = read_sample_times.first;
			double time_read_small_sample = read_sample_times.second;
			
			fprintf(info_file, "sample_row_num:%f\n", (double)sample[0].size());
			std::cout << "sub_sample size:" << small_sample[0].size() << std::endl;
			WritePar(par_file, PAR);
			WriteExpPar(par_file, exp_par);
			fclose(par_file);

			/*----------transfer sample into ca_sample--------*/
			double t3 = clock();
			std::vector<std::vector<aqppp::CA>> CAsample = std::vector<std::vector<aqppp::CA>>();
			aqppp::Tool::TransSample(sample, CAsample);
			double time_trans_sample = (clock() - t3) / CLOCKS_PER_SEC;
			/*------------------------------------------------*/

			std::cout << "gen query" << std::endl;
			std::vector<std::vector<aqppp::Condition>> user_queries = std::vector<std::vector<aqppp::Condition>>();

			if (!aqppp::DoubleEqual(PAR.SAMPLE_RATE,InitPar4DefaultSample().SAMPLE_RATE))
			{
				aqppp::Settings tppar = InitPar4DefaultSample();
				std::vector <std::vector<double>> tpsample;
				std::vector<std::vector<aqppp::CA>> tpcasample = std::vector<std::vector<aqppp::CA>>();
				aqppp::Tool::TransSample(sample, tpcasample);
				std::pair<double, double> read_sample_times = ReadSamples(sqlconnectionhandle, tppar, 1, tpsample, std::vector <std::vector<double>>());
				aqppp::Tool::GenUserQuires(tpsample, tpcasample, tppar.RAND_SEED, exp_par.QUERY_NUM, { exp_par.MIN_QUERY_SELECTIVELY,exp_par.MAX_QUERY_SELECTIVELY }, user_queries);
			}
			else
			{
				aqppp::Tool::GenUserQuires(sample, CAsample, PAR.RAND_SEED, exp_par.QUERY_NUM, { exp_par.MIN_QUERY_SELECTIVELY,exp_par.MAX_QUERY_SELECTIVELY }, user_queries);
			}
			std::cout << "gen query num: " << user_queries.size() << std::endl;

			std::cout << "sample size:" << sample[0].size() << std::endl;
			for (int i = 0;i < CAsample.size();i++)
			{
				std::cout << "dim:" << i << " distinct value:" << CAsample[i].size() << std::endl;
			}

			


			/*--------for no_uniform distribution: find materalize points and compute materlize result--------*/
			std::vector<std::vector<aqppp::CA>> NF_mtl_points;
			aqppp::MTL_STRU	NF_mtl_res = aqppp::MTL_STRU();
			std::vector<int> mtl_nums;
			std::cout << "start mtl..." << std::endl;
			
				double t10 = clock();
				aqppp::AssignBudgetForDimensions(PAR, false).AssignBudget(CAsample, mtl_nums);
				double time_dist_mtl = (clock() - t10) / CLOCKS_PER_SEC;

				double t2 = clock();
				NF_mtl_points = std::vector<std::vector<aqppp::CA>>();
				std::vector<double> max_errs;
				std::vector<int> iter_nums;
				aqppp::HillClimbing(PAR.SAMPLE_ROW_NUM, PAR.SAMPLE_RATE, PAR.CI_INDEX, PAR.NF_MAX_ITER, PAR.INIT_DISTINCT_EVEN).ChoosePoints(CAsample, mtl_nums, NF_mtl_points, max_errs, iter_nums);
				double time_NF_find_mtl_point = (clock() - t2) / CLOCKS_PER_SEC;
				fprintf(info_file, "sample size: %d\n", sample[0].size());
				for (int i = 0;i < CAsample.size();i++)
				{
					std::cout << "dim:" << i << " mtl_num:" << mtl_nums[i] << std::endl;
					fprintf(info_file, "dim:%d\tdistinct_value:%d\tmtl_num:%d\tmax_err:%f\tclimb_iter_num:%d\n", i, CAsample[i].size(), mtl_nums[i], max_errs[i], iter_nums[i]);
				}
				
				double time_NF_mtl_res = 0;
				if (exp_par.isMTL == true) time_NF_mtl_res=aqppp::Precompute(PAR.DB_NAME, PAR.TABLE_NAME, PAR.AGGREGATE_NAME, PAR.CONDITION_NAMES).GetPrefixSumCube(NF_mtl_points, sqlconnectionhandle, NF_mtl_res, "sum");
			/*-------------------------------------------------------------------*/


			double counter = 0;
			double time_spl_online = 0;
			double time_aqpp_online = 0;
			double time_direct_online = 0;

			fprintf(form_file, "query_id\t spl_est\t spl_ci\t aqpp_est\t aqpp_ci\t real_value\t selectively\t time_spl\t time_aqpp\t time_direct\t error_spl\t error_aqpp\n");

			double before_query_time = ((double)clock() - (double)bt) / CLOCKS_PER_SEC;
			std::vector<double> spl_errors = std::vector<double>();
			std::vector<double> aqpp_errors = std::vector<double>();

			/*--------------make query, and save result----------------*/
			std::cout << "make query now, query num:" << user_queries.size() << std::endl;
			for (int query_id = 0;query_id < user_queries.size();query_id++)
			{
				std::cout << "query id " << query_id << std::endl;
				std::vector<aqppp::Condition> cur_q = user_queries[query_id];
				fprintf(query_file, "query_id:%d  condition: ", query_id);
				for (auto ele : cur_q)
				{
					fprintf(query_file, "(%.2f, %.2f) ", ele.lb, ele.ub);
				}
				fprintf(query_file, "\n");

				double t3 = clock();
				std::pair<double, double> spl_sumci = aqppp::Sampling(PAR.SAMPLE_RATE, PAR.CI_INDEX).SamplingForSumQuery(sample, cur_q);
				//cout << "lala "<<spl_sumci.first << " " << spl_sumci.second << endl;
				double time_spl = (clock() - t3) / CLOCKS_PER_SEC;

				if (aqppp::DoubleEqual(spl_sumci.first, 0.0)) continue;   //if not found in sample, then skip this query. 
				std::pair<double, double> aqpp_sumci;
				double time_aqpp = 0;
				
				double t4 = clock();
				aqpp_sumci = aqppp::Aqpp(PAR.SAMPLE_ROW_NUM, PAR.SAMPLE_RATE, PAR.CI_INDEX).AqppSumQuery(query_id, info_file, sample, small_sample, NF_mtl_points, NF_mtl_res, cur_q);
				time_aqpp = ((double)clock() - (double)t4) / CLOCKS_PER_SEC;


				double t5 = clock();
				double real_value = QueryRealValue(cur_q, PAR.TABLE_NAME, sqlconnectionhandle, PAR,"sum");
				double time_direct = (clock() - t5) / CLOCKS_PER_SEC;

				double selectively = aqppp::Tool::EstimateSelectively(sample, cur_q).second;

				std::cout <<"real_value:"<<real_value<< " time_direct:" << time_direct << std::endl;

				double error_spl = real_value>0 ?  spl_sumci.second / real_value : 0;
				double error_aqpp = real_value > 0 ?  aqpp_sumci.second / real_value : 0;

				counter++;

				time_spl_online += time_spl;
				time_aqpp_online += time_aqpp;
				time_direct_online += time_direct;
				spl_errors.push_back(error_spl);
				aqpp_errors.push_back(error_aqpp);
				fprintf(form_file, "%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", query_id, spl_sumci.first, spl_sumci.second, 
					aqpp_sumci.first, aqpp_sumci.second, real_value, selectively, time_spl, time_aqpp, time_direct, error_spl, 
					error_aqpp);

			}
			/*-------------------------------------------------------------*/

			double avg_spl_error = aqppp::Tool::get_avg(spl_errors);
			double avg_aqpp_error = aqppp::Tool::get_avg(aqpp_errors);
			double median_spl_error = aqppp::Tool::get_percentile(spl_errors, 0.5);
			double median_aqpp_error = aqppp::Tool::get_percentile(aqpp_errors, 0.5);
			double spl_95p_err = aqppp::Tool::get_percentile(spl_errors, 0.95);
			double aqpp_95p_err = aqppp::Tool::get_percentile(aqpp_errors, 0.95);

			double time_spl_offline = time_create_db_samples.first;
			double time_NF_hybrid_offline = time_dist_mtl + time_create_db_samples.first + time_create_db_samples.second + time_trans_sample + time_NF_find_mtl_point + time_NF_mtl_res;


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

			fprintf(info_file, "avg_spl_error: %f%%\n", 100 * avg_spl_error);
			fprintf(info_file, "avg_aqpp_error: %f%%\n", 100 * avg_aqpp_error);
			fprintf(info_file, "median_spl_error: %f%%\n", 100 * median_spl_error);
			fprintf(info_file, "median_aqpp_error: %f%%\n", 100 * median_aqpp_error);
			fprintf(info_file, "spl_95p_error: %f%%\n", 100 * spl_95p_err);
			fprintf(info_file, "aqpp_95p_error: %f%%\n", 100 * aqpp_95p_err);
			fprintf(info_file, "\n");


			fprintf(info_file, "AQP offline time: %f\n", time_spl_offline);
			fprintf(info_file, "AQP++ offline time: %f\n", time_NF_hybrid_offline);

			fprintf(info_file, "create sample in db time: %f\n", time_create_db_samples.first);
			fprintf(info_file, "create small_sample in db time: %f\n", time_create_db_samples.second);
			fprintf(info_file, "read sample from db: %f\n", time_read_sample);
			fprintf(info_file, "read small_sample from db: %f\n", time_read_small_sample);
			fprintf(info_file, "NF dist mtl data tot time: %f\n", time_dist_mtl);
			fprintf(info_file, "NF cpt mtl res time: %f\n", time_NF_mtl_res);
			fprintf(info_file, "\n");



			double time_exp = (clock() - s_exp) / CLOCKS_PER_SEC;
			fprintf(info_file, "exp_time: %f", time_exp);
			fclose(info_file);
			fclose(form_file);
			fclose(query_file);
			std::cout << "exp_cph end " << std::endl;
			return 0;

		}
}
