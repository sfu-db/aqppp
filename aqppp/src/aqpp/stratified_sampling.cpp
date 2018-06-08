//#include "stdafx.h"
#include "stratified_sampling.h"

namespace aqppp {

	StratifiedSampling::StratifiedSampling(double ci_index)
	{
		this->PAR.CI_INDEX = ci_index;
	}
	/*
	given full sample and the demand of user query, cpt the estimate sum result of all data and corresponding confidence interval.
	*/
	std::pair<double, double> StratifiedSampling::SamplingForSumQuery(const std::vector<std::vector<double>> &sample, const std::vector<double> &prob,const std::vector<Condition> &demands)
	{
		assert(!sample.empty());
		const int ROW_NUM = sample[0].size();
		assert(ROW_NUM > 1);
		double sum = 0.0, sum2 = 0.0;
		for (int rowi = 0; rowi < ROW_NUM; rowi++)
		{
			bool flag = true;
			for (int i = 0; i < demands.size(); i++)
			{
				if (DoubleLess(sample[i + 1][rowi], demands[i].lb) || DoubleGreater(sample[i + 1][rowi], demands[i].ub))
				{
					flag = false;
					break;
				}
			}
			if (!flag) continue;
			double data = sample[0][rowi]/prob[rowi];
			sum += data;
			sum2 += data*data;
		}
		double variance = 0;
		variance = sum2 / ROW_NUM - (sum / ROW_NUM)*(sum / ROW_NUM);
		double ci = this->PAR.CI_INDEX*sqrt(variance / ROW_NUM);
		return std::pair<double, double>(sum/ROW_NUM, ci);
	}

}

