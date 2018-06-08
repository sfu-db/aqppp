#pragma once
#include"common_content.h"
#include "tool.h"
#include <cstdio>
#include<iostream>
#include<vector>

namespace aqppp {
	class Sampling {
	private:
		struct par
		{
			double SAMPLE_RATE;
			double CI_INDEX;
		} PAR;

	public:
		Sampling(double sample_rate, double ci_index);
		/*
		given full sample and the demand of user query, cpt the estimate sum result of all data and corresponding confidence interval.
		*/
		std::pair<double, double> SamplingForSumQuery(const std::vector<std::vector<double>> &sample, const std::vector<Condition> &demands);

	};
}

