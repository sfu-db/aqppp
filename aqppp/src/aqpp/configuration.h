#pragma once
#include<vector>
#include<any>
#include<map>

namespace aqppp {
	class Configuration {

	public:
		std::map<std::string, std::string> conf;
		/*
		std::string DB_NAME;
		std::string TABLE_NAME;
		std::string SAMPLE_NAME;
		string SUB_SAMPLE_NAME;
		bool CREATE_DB_SAMPLES;
		double SAMPLE_RATE;
		double SUB_SAMPLE_RATE;
		std::string AGGREGATE_NAME;
		std::vector<std::string> CONDITION_NAMES ;
		double CI_INDEX;
		int SAMPLE_ROW_NUM;
		int NF_MAX_ITER;
		int ALL_MTL_POINTS;
		int EP_PIECE_NUM;
		bool INIT_DISTINCT_EVEN;
		int RAND_SEED;
		*/

	public:
		Configuration ReadConfig(std::string fname);

	};
}
