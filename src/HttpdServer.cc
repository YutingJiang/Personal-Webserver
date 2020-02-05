#include <sysexits.h>
#include <stdlib.h>
#include <regex>
#include <stdio.h>
#include <sys/socket.h> 	// creating socket
#include <netinet/in.h>		// for sockaddr_in
#include <unistd.h>			// for close
#include <iostream>			// input output
#include <string>			// for bzero
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <thread>
#include <sys/types.h>
#include <sys/uio.h>
#include <limits.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <netdb.h>
#include <sys/types.h>
#include <queue>
#include <fstream>

#include "PathHandler.hpp"
#include "logger.hpp"
#include "HttpdServer.hpp"
#include "ClientHandler.hpp"

	
HttpdServer::HttpdServer(INIReader& t_config)
	: config(t_config)
{
	auto log = logger();
	string pstr = config.Get("httpd", "port", "");
	if (pstr == "") {
		log->error("PORT was not in the config file");
		exit(EX_CONFIG);
	}
	regex port_range("^([1-9][0-9]{1,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
	if(!regex_match(pstr.c_str(),port_range))
		FatalError("INVALID Port Number "+ pstr + " in Config!");
	port = pstr;

	string dr = config.Get("httpd", "doc_root", "");
	if (dr == "") {
		log->error("DOC_ROOT was not in the config file");
		exit(EX_CONFIG);
	}

	dr = parseURL(dr);

	try{checkDir(dr);}catch(const string msg){
		log->error(dr+": \n"+msg);
		cout<<"EXITING..."<<endl;
		exit(1);
	}
	doc_root = dr;
	split(doc_root,"/",doc_root_tokens);
	string mt= config.Get("httpd", "mime_types", "");
	if (mt == "") {
		log->error("MIME_TYPES was not in the config file");
		exit(EX_CONFIG);
	}

	//loading MIME file
    mt= parseURL(mt);


	ifstream mime_reader(mt.c_str());
	if(!mime_reader)FatalError(mt+": Doesn`t Exist!");
	else log->info("SUCCESSFULLY loaded mime.types");
	mime_types = mt;
	string temp;
	while (getline(mime_reader,temp)){
		vector<string> mime_pair;
		split(temp," ",mime_pair);
		if(mime_pair.size()<2)
			FatalError("Invalid mime.types!\nat: "+temp);
		mime[mime_pair[0]]=mime_pair[1];
	}
	//loading ends
}

HttpdServer::~HttpdServer() {}


void handleClient(int socket_id, string doc_root, vector<string> doc_root_tokens,
unordered_map<string, string> &mime)
 {
	 	auto log = logger();
		string input = "";
		char buffer[256];
		int n = 1;
		int cnt = 0;
		ClientHandler * clienthandler = new ClientHandler(doc_root,doc_root_tokens,mime);
		while(n>0) {//string(buffer).find("\r\n\r\n") == string(buffer).npos
			bzero(buffer, 256);
			n = read(socket_id, buffer, 256);
			cnt += n;
			input += string(buffer);
			cout<<n<<endl;
			if(n<0 ) {
					if(input.find("\r\n\r\n") == input.npos) {
					log->error("Reading message timeout!\n");
					break;
				}
			}
			if(input.length()>=4 && input.substr(input.length()-4) == "\r\n\r\n") {
				cout<<"Message received: \n"<< input <<endl;
				cout<<"Message length: "<<cnt<<endl;
				bool exit = clienthandler->launch(socket_id, input.c_str());
				input = "";
				cnt = 0;
				if(exit) break;
			}
		}
		close(socket_id);
		cout<<"Connection Closed For This Client"<<endl;
		delete clienthandler;
		// if(0 > cnt) {
		// 	cout << "ERROR getting message or Time Out!" << endl;
		// 	close(socket_id);
		// } else {
		// 	cout<<"Message received: \n"<< input <<endl;
		// 	cout<<"Message length: "<<cnt<<endl;
		// 	delete clienthandler;
		// }
}

void HttpdServer::launch()
{
	auto log = logger();

	log->info("LAUNCHING web server");
	log->info("PORT: {}", port);
	log->info("DOC_ROOT: {}", doc_root);
	uint16_t nPort = atoi((char*)port.c_str()); 

	// Put code here that actually launches your webserver...

	// if socket was created successfully, 
	// socket() returns a non-negative number
	int sock = socket(AF_INET, SOCK_STREAM,0);
	if(0> sock) {
		log->error("ERROR creating socket");
		close(sock);
	}

	// create a sockaddr_in struct
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(nPort);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	int b = bind(sock,(struct sockaddr*)&server_address,sizeof(server_address));
	if(0 > b) {
		log->info("ERROR binding socket");
		close(sock);
	}
	cout << "SERVER is running......" << endl;
	//listen client's call
	listen(sock, 1);
	struct sockaddr_in client_address;
	socklen_t client_length = sizeof(client_address);
	char buffer[256];
	bzero(buffer, 256);

	while(1){
		int new_sock = accept(sock, (struct sockaddr*)&client_address,
						  &client_length);
		// if connection was created successfully, 
		// accept() returns a non-negative number
		if(0 > new_sock) {
			cout << "ERROR accepting connection" << endl;
			close(sock);
			continue;
		}
		//Setting 5 seconds Timeout for read
		struct timeval tv;
		tv.tv_sec = 5;  /* 5 Secs Timeout */
		setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO,
		(struct timeval *)&tv,sizeof(struct timeval));
		//Setting ends

		//print client's information
		in_port_t port_no = ntohs(client_address.sin_port);
		char ip_address[INET_ADDRSTRLEN];
		void *numeric_addr = NULL;
		numeric_addr = &client_address.sin_addr;
		inet_ntop(client_address.sin_family, numeric_addr, ip_address, sizeof(ip_address));
		cout<<"Connected! Client IP: "<<string(ip_address)<<" PORT:"<<port_no<<endl;
		//end


		// int n = read(new_sock, buffer, 256);
		// if(0 > n) {
		// 	cout << "ERROR getting message or Time Out!" << endl;
		// 	close(new_sock);
		// 	continue;
		// } else {
		// 	cout<<"Message received: \n"<< buffer <<endl;
		// 	cout<<"Message length: "<<n<<endl;
		// }
		// // 6. Handle file request
		// // Parse request
		// ClientHandler * clienthandler = new ClientHandler(doc_root,doc_root_tokens,mime);
		// clienthandler->launch(new_sock, buffer);
		// delete clienthandler;



		std::thread _thread([&](){handleClient(new_sock, doc_root, doc_root_tokens, mime);});
		_thread.detach();
		//handleClient(new_sock, doc_root, doc_root_tokens, mime);
		sleep(1);




	}

	close(sock);



}

