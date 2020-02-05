#include <iostream>
#include <thread>
#include <sysexits.h>
#include <stdlib.h>
#include <limits.h>

#include "inih/INIReader.h"

#include "logger.hpp"
#include "HttpdServer.hpp"
#include "PathHandler.hpp"

using namespace std;

int main(int argc, char** argv) {
	initLogging();
	auto log = logger();

	setProgramPath(argv[0]);
	// Handle the command-line argument
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " [config_file]" << endl;
		return EX_USAGE;
	}

	// Read in the configuration file

	string config_path = string(argv[1]); 
	config_path = parseURL(config_path);
	INIReader config(config_path);

	if (config.ParseError() < 0) {
		cerr << "Error parsing config file " << config_path << endl;
		return EX_CONFIG;
	}

	if (config.GetBoolean("httpd", "enabled", true)) {
		log->info("Web server enabled");
		HttpdServer * httpd = new HttpdServer(config);
		httpd->launch();
	} else {
		log->info("Web server disabled");
	}

	return 0;
} 
