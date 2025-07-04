#pragma once

#include <string>
#include <cstdint>
#include "../ziran-shared/FilesystemLib.h"

struct TConfig
{
	filesystem::path inDir = "input";
	filesystem::path outDir = "output";
	filesystem::path workDir = "work";
	filesystem::path discardDir = "discard";

	std::string bindIpString = "localhost";
	uint16_t bindPort = 6480;

	std::string dbHost = "localhost";
	uint16_t dbPort = 3306;
	std::string dbUser = "root";
	std::string dbPassword = "";
	std::string dbName = "ziran";
};
