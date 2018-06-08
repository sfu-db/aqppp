#include"configuration.h"
#include"tool.h"
#include<vector>
#include <fstream>
#include<any>
#include<map>
namespace aqppp
{
	Configuration Configuration::ReadConfig(std::string fname)
	{
		Configuration conf = Configuration();
		//conf.conf = std::map<std::string, any>();

		std::string line;
		std::ifstream infile(fname);
		if (infile.is_open()) {
			while (std::getline(infile, line))
			{
				std::vector<std::string> split_result=Tool::split(line, '=');
				try
				{
					std::string attribute = split_result[0];
					std::string value = split_result[1];
					conf.conf.insert({ attribute,value });
				}
				catch (const std::out_of_range& oor) {
					std::cerr << "Out of Range error: " << oor.what() << '\n';
				}

			}

		}
		infile.close();
		return conf;
	}
}