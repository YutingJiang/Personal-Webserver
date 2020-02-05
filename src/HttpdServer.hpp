#ifndef HTTPDSERVER_HPP
#define HTTPDSERVER_HPP



#include "inih/INIReader.h"
#include "logger.hpp"

using namespace std;

	

class HttpdServer {
public:
	HttpdServer(INIReader& t_config);
	~HttpdServer();
	void launch();
protected:
	INIReader& config;
	string port;
	string doc_root;
	vector<string> doc_root_tokens;
	unordered_map<string,string> mime;
	string mime_types;

	private:
};

#endif // HTTPDSERVER_HPP
