#pragma once
#include"common_content.h"
#include "tool.h"
#include <cstdio>
#include<iostream>
#include<vector>

namespace aqppp {
	class StratifiedSampling {
	private:
		struct Par
		{
			double CI_INDEX;
		} PAR;

	public:
		StratifiedSampling(double ci_index);
		/*
		given full sample and the demand of user query, cpt the estimate sum result of all data and corresponding confidence interval.
		*/
		std::pair<double, double> SamplingForSumQuery(const std::vector<std::vector<double>> &sample, const std::vector<double> &prob, const std::vector<Condition> &demands);

	};
}

