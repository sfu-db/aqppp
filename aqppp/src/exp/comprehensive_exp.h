//comprehensive experiment.
#pragma once
#include<vector>
#include <windows.h>
#include<time.h>
#include"../aqpp/sql_interface.h"
#include"../aqpp/common_content.h"
#include"../exp/exp_common.h"
#include"../aqpp/precompute.h"
#include"../aqpp/assign_budget_for_dimensions.h"
#include"../aqpp/hill_climbing.h"
#include"../aqpp/aqpp.h"
#include"../aqpp/tool.h"
#include"../aqpp/sampling.h"
namespace expDemo {
	class ComprehensiveExp {
	private:
		static std::string GetExpRootPath(void);

		static const aqppp::Settings InitPar4DefaultSample();
		static const aqppp::Settings InitPar4LargeSample();

		struct ExpPar
		{
			bool USE_DB_INDEX ;
			bool DROP_INDEX_BEFORE_CREATE;
			bool isMTL;
			bool EXP_UNIFORM;
			bool QUERY_DB_REAL_VALUE;
			bool QUERY_DB_SELECTIVELY;
			bool COVER_DIRECT_RESULT_FILE;  //generally this should be false.
			int QUERY_NUM;
			double MIN_QUERY_SELECTIVELY;
			double MAX_QUERY_SELECTIVELY;

		};


		static void WriteExpPar(FILE *par_file, ExpPar exp_par);

		static ExpPar GetExpPar();

	public:

		static int Exp(SQLHANDLE& sqlconnectionhandle);
		static int ComprehensiveExp::RunOnce(SQLHANDLE& sqlconnectionhandle, aqppp::Settings PAR);
	};
}
