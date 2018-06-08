//#include "stdafx.h"
#include "tool.h"
#include<time.h>
namespace aqppp
{
	 std::vector<std::string> Tool::split(const std::string &s, char delim) {
			std::vector<std::string> elems;
			split(s, delim, std::back_inserter(elems));
			return elems;
		}


	 void Tool::MkDirRecursively(std::string dirpath)
	 {
		 std::vector<std::string> pathseps = Tool::split(dirpath, '/');
		 std::string s = "";
		 for (int i = 0;i < pathseps.size();i++)
		 {
			 s += pathseps[i] + '/';
			 _mkdir(s.c_str());
		 }
	 }



	 double Tool::ComputeCovariance(const std::vector<double>& X, const std::vector<double>& Y)
	 {
		 double sx = X.size();
		 double sy = Y.size();
		 double ex = get_avg(X);
		 double ey = get_avg(Y);
		 double cov = 0;
		 for (int i = 0;i < X.size();i++)
		 {
			 for (int j = 0;j < Y.size();j++)
			 {
				 cov += (X[i] - ex)*(Y[j] - ey);
			 }
		 }
		 cov /= sx;
		 cov /= sy;
		 return cov;
	 }



	 double Tool::ComputeCorrelation(std::vector<double>& X, std::vector<double>& Y)
	 {
		 double a = ComputeCovariance(X, Y);
		 double sx = sqrt(get_var(X));
		 double sy = sqrt(get_var(Y));
		 if (DoubleEqual(sx, 0) || DoubleEqual(sy, 0)) return -10000000000;
		 return (a / sx) / sy;
	 }
	 double Tool::get_sum(const std::vector<double> &data)
	 {
		 double t = 0;
		 for (double ele : data)
		 {
			 t += ele;
		 }
		 return t;
	 }

	 void Tool::multpily(const std::vector<double> &vec,double t, std::vector<double> &o_res)
	 {
		 o_res = std::vector<double>(vec.size());
		 for (int i=0;i<vec.size();i++)
		 {
			 o_res[i] = vec[i] * t;
		 }
		 return;
	 }

	 void Tool::add(std::vector<double> &o_vec1, const std::vector<double> &vec2)
	 {
		 for (int i=0;i<o_vec1.size();i++)
		 {
			 o_vec1[i] += vec2[i];
		 }
		 return;
	 }

	 void Tool::add(std::vector<double> &o_vec, double t)
	 {
		 for (double &v : o_vec)
		 {
			 v += t;
		 }
	 }
	 double Tool::get_avg(const std::vector<double> &data)
	 {
		 double s = get_sum(data);
		 double len = data.size();
		 return s / len;
	 }


	 double Tool::get_var(const std::vector<double> &data)
	 {
		 double s = 0;
		 double ex = get_avg(data);
		 for (int i = 0;i < data.size();i++)
		 {
			 s += ((data[i] - ex)*(data[i] - ex));
		 }
		 double len = data.size();
		 double var = s/len;
		 return var;
	 }

	 double Tool::get_percentile(const std::vector<double> &data, double percent)
	 {
		 if (data.size() == 0) return 0;
		 std::vector<double> data_sort = std::vector<double>(data);
		 std::sort(data_sort.begin(), data_sort.end());
		 double len = data_sort.size();
		 double pos = (len - 1)*percent;
		 int kint = int(pos);
		 double kper = pos - kint;
		 if (kint == data_sort.size() - 1) return data_sort[kint];
		 return data_sort[kint] + kper*(data_sort[kint + 1] - data_sort[kint]);
	 }

	 int Tool::MapMultidimTo1Dim(const std::vector<int> &pos, const std::vector<int>& DIM_SIZES)
	 {
		 assert(pos.size() == DIM_SIZES.size());
		 int one_dim_id = 0;
		 int dim = pos.size();
		 int cur_size = 1;
		 for (int i = 0; i < dim; i++)
		 {
			 if (pos[i] >= DIM_SIZES[i]) return -1;
			 cur_size *= DIM_SIZES[i];
		 }

		 for (int i = 0; i < dim; i++)
		 {
			 cur_size /= DIM_SIZES[i];
			 one_dim_id += (pos[i] * cur_size);
		 }
		 return one_dim_id;
	 }

	 int Tool::Map1DimToMultidim(int id, const std::vector<int> &DIM_SIZES, std::vector<int> &o_pos)
	 {
		 int td = id;
		 int dim = DIM_SIZES.size();
		 o_pos = std::vector<int>(dim);
		 int cur_size = 1;
		 for (int i = 0; i < dim; i++) cur_size *= DIM_SIZES[i];
		 for (int i = 0; i < dim; i++)
		 {
			 cur_size /= DIM_SIZES[i];
			 o_pos[i] = td / cur_size;
			 td -= o_pos[i] * cur_size;
			 if (o_pos[i] >= DIM_SIZES[i]) return -1;
		 }
		 return 0;
	 }


		/*
		CA_sample doesn't include fake point.
		The duplicate conditions in each col has only kept one condition, and the aggreate exists of others has been added to that one.
		trans original sample into CAsample.
		note the CAsample doesn't have the accumulation column because it has the struct of condition attribute combined with accumulation arribute.
		each col of CAsample is the combination of the accumulatation attribute and condition attribute of the original sample. Each column of it is sorted by condition, then aggreate data.
		*/
	 void Tool::TransSample(const std::vector<std::vector<double>> &sample, std::vector<std::vector<CA>> &o_CAsample)
		{
		 std::vector<std::vector<CA> >temp_casample = std::vector<std::vector<CA>>(sample.size() - 1);
			for (int ci = 1; ci < sample.size(); ci++)
			{
				temp_casample[ci - 1] = std::vector<CA>(sample[ci].size());
				for (int ri = 0; ri < sample[ci].size(); ri++)
				{
					CA temp = CA();
					temp.sum = sample[0][ri];
					temp.sqrsum = sample[0][ri] * sample[0][ri];
					temp.condition_value = sample[ci][ri];
					temp_casample[ci - 1][ri] = temp;
				}
			}

			o_CAsample = std::vector<std::vector<CA>>(sample.size() - 1);
			for (int ci = 0; ci < temp_casample.size(); ci++)
			{
				sort(temp_casample[ci].begin(), temp_casample[ci].end(), CA_compare);
				std::vector<CA> cur_col = std::vector<CA>();
				CA ca = temp_casample[ci][0];
				ca.id = 0;
				ca.count = 1;
				cur_col.push_back(ca);
				for (int ri = 1; ri < temp_casample[ci].size(); ri++)
				{
					if (DoubleGreater(temp_casample[ci][ri].condition_value, cur_col[cur_col.size() - 1].condition_value))
					{
						CA ca = temp_casample[ci][ri];
						ca.id = cur_col.size();
						ca.count = 1;
						cur_col.push_back(ca);
					}
					else {
						cur_col[cur_col.size() - 1].sum += temp_casample[ci][ri].sum;
						cur_col[cur_col.size() - 1].sqrsum += temp_casample[ci][ri].sqrsum;
						cur_col[cur_col.size() - 1].count += 1;
					}
				}
				o_CAsample[ci] = cur_col;

			}

			return;
		}

	 void Tool::ReadQueriesFromFile(std::string query_file_full_name, int query_dim, std::vector<std::vector<Condition>> &o_user_queries)
	 {
		 o_user_queries = std::vector<std::vector<Condition>>();
		 std::ifstream infile(query_file_full_name);
		 double tp = -1;
		 int counter = 0;
		 while (infile >> tp)
		 {
			 if (counter % (2 * query_dim) == 0) o_user_queries.push_back(std::vector<Condition>(query_dim));
			 if (counter % 2 == 0) o_user_queries.back()[(counter % (2 * query_dim)) / 2].lb = tp;
			 else o_user_queries.back()[(counter % (2 * query_dim)) / 2].ub = tp;
			 counter++;
		 }
		 infile.close();
		 return;
	 }

	 void Tool::SaveQueryFile(std::string query_file_full_name, std::vector<std::vector<Condition>> &user_queries)
	 {
		 FILE *query_file = new FILE();
		 fopen_s(&query_file, query_file_full_name.c_str(), "w");
		 for (auto v : user_queries)
		 {
			 for (Condition cond:v)
			 {
				 fprintf(query_file, "%f\t%f\t", cond.lb, cond.ub);
			 }
			 fprintf(query_file, "\n");
		 }
		 fclose(query_file);
	 }

	 std::pair<int, double> Tool::EstimateSelectively(const std::vector<std::vector<double>> &sample, const std::vector<Condition> &demands)
		{
			assert(!sample.empty());
			const int ROW_NUM = sample[0].size();
			assert(ROW_NUM > 1);
			int counter = 0;
			for (int rowi = 0; rowi < ROW_NUM; rowi++)
			{
				bool meet = true;
				for (int i = 0; i < demands.size(); i++)
				{
					if (DoubleLess(sample[i + 1][rowi],demands[i].lb) || DoubleGreater(sample[i + 1][rowi],demands[i].ub))
					{
						meet = false;
						break;
					}
				}
				if (meet) counter++;
			}

			return{ counter,(double)counter / (double)ROW_NUM };
		}



	 //note dim of query is not the same as sample, cause sanple contains group by dimension.
	 bool Tool::notAllGroupHas(const std::vector<Condition> &demands, const std::unordered_map<std::vector<int>, Group, VectorHash> &groups)
	 {
		 for (auto gpit : groups)
		 {
			 std::vector<std::vector<double>> sample = gpit.second.sample;
			 const int ROW_NUM = sample[0].size();
			 assert(ROW_NUM > 1);
			 int counter = 0;
			 for (int rowi = 0; rowi < ROW_NUM; rowi++)
			 {
				 bool meet = true;
				 for (int i = 0; i < demands.size(); i++)
				 {
					 if (sample[i + 1][rowi]<demands[i].lb || sample[i + 1][rowi]>demands[i].ub)
					 {
						 meet = false;
						 break;
					 }
				 }
				 if (meet) counter++;
			 }
			 if (counter == 0) return true;
		 }
		 return false;
	 }


	 bool notGroup(const std::vector<Condition> &demands, const std::unordered_map<std::vector<int>, Group, VectorHash> &groups)
	 {
		 std::vector<int> indx = std::vector<int>();
		 int GP_LEN = groups.begin()->first.size();
		 for (int i = demands.size()-GP_LEN; i < demands.size(); i++)
		 {
			 if (!DoubleEqual(demands[i].lb, demands[i].ub)) return true;
			 indx.push_back(round(demands[i].lb));
		 }

		 return (groups.find(indx) == groups.end());
	 }


	 void Tool::GenUserQuires(const std::vector<std::vector<double>> &sample, const std::vector<std::vector<CA>> &casample, const int seed, const int query_num, std::pair<double, double> QUERY_SELECTIVELY_RANGE, std::vector<std::vector<Condition> > &o_user_queries, const std::unordered_map<std::vector<int>, Group, VectorHash> &groups)
		{
			std::mt19937 rand_gen(seed);
			const double MIN_QUERY_SELECTIVELY = QUERY_SELECTIVELY_RANGE.first;
			const double MAX_QUERY_SELECTIVELY = QUERY_SELECTIVELY_RANGE.second;
			const int DIM = casample.size();
			const int ROW_NUM = sample[0].size();
			o_user_queries = std::vector<std::vector<Condition>>();
			std::unordered_set<int> select_set = std::unordered_set<int>();
			int query_id = 0;
			for (int iter_num = 0; iter_num < 1000; iter_num++)
			{
				bool fail_group_check = false;
				for (int i = 0;i < query_num;i++)
				{
					if (query_id >= query_num) break;
					std::vector<Condition> cur_query = std::vector<Condition>(DIM);
					int counter = 0;
					std::pair<int, double> est_sel = { 0, 0.0 };
					do
					{
						counter++;
						for (int cond = 0;cond < DIM;cond++)
						{
							int cur_len = casample[cond].size();
							int t1 = rand_gen() % cur_len;
							int t2 = rand_gen() % cur_len;
							if (t1 > t2) std::swap(t1, t2);
							cur_query[cond].lb = casample[cond][t1].condition_value;
							cur_query[cond].ub = casample[cond][t2].condition_value;

							/*
							cur_query[cond].lb = min(sample[cond][rid1], sample[cond][rid2]);
							cur_query[cond].ub = max(sample[cond][rid1], sample[cond][rid2]);
							for (int ri = rid1; ri <= rid2; ri++)
							{
								if (sample[cond][ri] < cur_query[cond].lb) cur_query[cond].lb = sample[cond][ri];
								if (sample[cond][ri] > cur_query[cond].ub) cur_query[cond].ub = sample[cond][ri];
							}
							int t2 = rand_gen() % cur_len;	
							while (DoubleLess(casample[cond][t2].condition_value, cur_query[cond].lb) && (t2<cur_len-1))
							{
								t2 = t2+1+rand_gen() %(cur_len-1-t2);
							}
							cur_query[cond].ub = casample[cond][t2].condition_value;
							*/

						}
						est_sel = EstimateSelectively(sample, cur_query);
						if (groups.size() > 0) fail_group_check = notGroup(cur_query, groups);
					} while (counter < 1000 && (fail_group_check || select_set.count(est_sel.first) > 0 || DoubleGreater(est_sel.second, MAX_QUERY_SELECTIVELY) || DoubleLess(est_sel.second, MIN_QUERY_SELECTIVELY) || est_sel.first <= 0));
					if (counter < 1000)
					{
						std::cout << "iter_num:" << iter_num << " qid:" << query_id << " selectively:" << est_sel.second << std::endl;
						select_set.insert(est_sel.first);
						o_user_queries.push_back(cur_query);
						query_id++;
					}
					else
					{
						std::cout << "iter_num:" << iter_num << std::endl;
						break;
					}
				}
			}

		}
}