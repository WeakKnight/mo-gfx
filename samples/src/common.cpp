#include "common.h"

#include "string_utils.h"
#ifdef _WIN32
#include <direct.h>
#else
#include<unistd.h>  
#endif

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

void InitEnvironment(char** args)
{
	auto path = args[0];


#ifdef _WIN32
	auto tokens = StringUtils::Split(path, "\\");

	std::string environmentPath = "";

	for (int i = 0; i < tokens.size() - 2; i++)
	{
		environmentPath += (tokens[i] + "\\");
	}
	environmentPath += "samples\\assets\\";

	_chdir(environmentPath.c_str());
#else
	auto tokens = StringUtils::Split(path, "/");
	for (int i = 0; i < tokens.size() - 2; i++)
	{
		environmentPath += (tokens[i] + "/");
	}
	environmentPath += "samples/assets/";

	chdir(environmentPath.c_str());
#endif

	spdlog::info("Environment Path Is {}", environmentPath);
}

namespace Math
{
	float Min(float a, float b)
	{
		if (a <= b)
		{
			return a;
		}
		else
		{
			return b;
		}
	}

	float Max(float a, float b)
	{
		if (a >= b)
		{
			return a;
		}
		else
		{
			return b;
		}
	}
}