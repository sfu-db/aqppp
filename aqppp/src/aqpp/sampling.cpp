//#include "stdafx.h"
#include "sampling.h"

namespace aqppp {
	
	Sampling::Sampling(double sample_rate, double ci_index)
		{
			this->PAR.SAMPLE_RATE = sample_rate;
			this->PAR.CI_INDEX = ci_index;
		}
		/*
		given full sample and the demand of user query, cpt the estimate sum result of all data and corresponding confidence interval.
		*/
	std::pair<double, double> Sampling::SamplingForSumQuery(const std::vector<std::vector<double>> &sample, const std::vector<Condition> &demands)
		{
			assert(!sample.empty());
			const double ROW_NUM = sample[0].size();
			assert(ROW_NUM > 1);
			double sum = 0.0, sum2 = 0.0;
			for (int rowi = 0; rowi < sample[0].size(); rowi++)
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
				sum += sample[0][rowi];
				sum2 += sample[0][rowi] * sample[0][rowi];
			}
			double variance = 0;
			variance = sum2 / ROW_NUM - (sum / ROW_NUM)*(sum / ROW_NUM);
			double ci = this->PAR.CI_INDEX*sqrt(variance / ROW_NUM)*ROW_NUM / this->PAR.SAMPLE_RATE;
			//cout << "this.sample rate: sum2 sum2 " << PAR.SAMPLE_RATE <<" "<<sum2<<" "<<sum<< endl;
			return std::pair<double, double>(sum / this->PAR.SAMPLE_RATE, ci);
		}

}

