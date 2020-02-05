#include <sysexits.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> 	// creating socket
#include <netinet/in.h>		// for sockaddr_in
#include <unistd.h>			// for close
#include <iostream>			// input output
#include <string>			// for bzero
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <netdb.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <queue>
#include <fstream>

#include "logger.hpp"
#include "PathHandler.hpp"
#include "ClientHandler.hpp"

#define BUFFSIZ 512
#define MAX_TIME_SIZE 128

ClientHandler::ClientHandler(const string doc_root,
const vector<string> doc_root_tokens,
const unordered_map<string,string> &mime):
_doc_root(doc_root),
_doc_root_tokens(doc_root_tokens),
_mime(mime)
 {}

 ClientHandler::~ClientHandler(){}

//check if the file path is valid
int ClientHandler::validate_file(string path) {
    auto log = logger();
    if (access(path.c_str(), F_OK) == -1){
		return -1;
	}else{
        if(access(path.c_str(),R_OK) == -1) {
            throw string("PERMISSION DENIED! NO ACCESS TO THE FILE");
            return -1;
        }
		vector<string> path_tokens;
        split(path,"/",path_tokens);
        if(path_tokens.size()<_doc_root_tokens.size()) {
            throw string("ERROR! ESCAPE DOC_ROOT");
            return -1; //escape doc_root
        }
        for(int i = 0; i < _doc_root_tokens.size() -1; i++) {
            if(path_tokens.at(i) != _doc_root_tokens.at(i)) {
                throw string("ERROR! ESCAPE DOC_ROOT");
                return -1;//escape doc_root
            }
        }
        try{checkFile(path);}
        catch(const string msg){
		log->error(path+": \n"+msg);
        return -1;//File doesn't exist
	    }
		return 1;
	}
}

//Build Header
string ClientHandler::build_headers(string filepath, int errcode) {
    string response;
    struct tm *ptr;
    time_t lt;
    time(&lt);
    ptr=localtime(&lt);
    char time[MAX_TIME_SIZE];
    strftime(time,MAX_TIME_SIZE,"%a, %y %b %d %X %z",ptr);
    switch(errcode) {
        case 200:
        {
            response += "HTTP/1.1 200 OK\r\n";
            response += "Server: YTJsServer 1.0\r\n";
            response += "Last-Modified: "+string(time)+"\r\n";
	        response += "Content-Length: "+ to_string(get_file_size(filepath.c_str())) +"\r\n";
            vector <string>path_tokens; 
            split(filepath,"/",path_tokens);
            string mime_name = path_tokens.back().substr(path_tokens.back().find(".")) ;
            auto types = _mime.find(mime_name);
            if (types != _mime.end()) {
               response =response +  "Content-Type: "+ types->second +"\r\n"; 
            } else {
                response += "Content-Type: application/octet-stream\r\n";
             }
	        response += "\r\n";
            break;
        }
        case 404:
        {
            response += "HTTP/1.1 404 NOT FOUND\r\n";
            response += "Server: YTJsServer 1.0\r\n";
            response += "\r\n";
            break;
        }
        case 400:
        {
            response += "HTTP/1.1 400 CLIENT ERROR\r\n";
            response += "Server: YTJsServer 1.0\r\n";
            response += "Connection: close\r\n";
            response += "\r\n";
            break;
        }
    }
    return response;
    
}

int ClientHandler::get_file_size(const char* filepath)
{
    struct stat finfo;
    if (stat(filepath, &finfo) != 0) {
        //die_system("stat() failed");
    }
    return (int) finfo.st_size;
}





bool ClientHandler::launch(int client_sock, const char* buff) {
    auto log = logger();
    string headers;
    bool client_error = false;
    bool connection_close = false;
    //Parse the request
    char* buf_copy = (char*) malloc (strlen(buff) + 1);
    strcpy(buf_copy, buff);
    string url;
    std::string request(buf_copy);
    vector<string> subrequest;
    if(request == "\r\n\r\n") {
        client_error = true;
        headers = build_headers("NA", 400);
        send(client_sock, (void*) headers.c_str(), (ssize_t) headers.size(), 0);
    }
    split(request,"\r\n\r\n",subrequest);
    //cout<<subrequest.at(0)<<endl;
    vector<string>lines;
    vector<string>lines_tokens;
    //pipeline
    for(int j = 0; j < subrequest.size();j++){
    //pipeline starts
        string full_path;
        int is_valid = 0;
        split(subrequest.at(j),"\r\n",lines);
        int n = lines.size();
        //cout<<lines.at(0)<<endl;
        for(int i = 0; i <n; i ++){
            split(lines.at(i)," ",lines_tokens);
            //cout<<lines_tokens.size()<<endl;

            // if(lines_tokens.size() < 2 || lines_tokens.size() > 3) {
            //     headers = build_headers("NA", 400);
            //     cout<<"ERROR1"<<endl;
            //     client_error = true;
            //     break;
            // }

            if(i == 1 && lines_tokens.at(0) != "Host:") {//See if Malformat(No Host)
                    headers = build_headers("NA", 400);
                    cout<<"ERROR5"<<endl;
                    client_error = true;
                    break;
            }

            if(lines_tokens.at(0) == "GET"){//Check Request Header
               if(lines_tokens.size()>=3){
                if( lines_tokens.at(2) == "HTTP/1.1") {
                    url = lines_tokens.at(1);
                    if(url.substr(0,1) == "/"){
                        lines_tokens.clear();
                        continue;
                    } else {
                        headers = build_headers("NA", 400);
                        client_error = true;
                        break;
                    }
                } else {
                    headers = build_headers("NA", 400);
                    client_error = true;
                    break;
                }
                }  else {
                    headers = build_headers("NA", 400);
                    client_error = true;
                    break;
                }   
            }

            if(lines_tokens.at(0) != "GET" &&                //See if Malformat (No colon:)
            lines_tokens.at(0).substr(lines_tokens.at(0).length()-1) != ":"){
                headers = build_headers("NA", 400);
                cout<<"ERROR4"<<endl;
                client_error = true;
                break;
            }

            if(lines_tokens.at(0) == "Connection:"){//See if connection is closed automatically
                if(lines_tokens.at(1) == "close") connection_close = true;
            }
            lines_tokens.clear();
        }

    //parse file's path
    if(!client_error){
        string homedir(getenv("HOME"));
        char buffer[PATH_MAX];
        if(url.substr(0,1) == "~")  full_path = homedir + "/" + string(url).substr(1);
        else if(url== "" || url == "/") full_path = _doc_root + "/" + "index.html";
        //else if(url.substr(url.length()-1) == "/")full_path = _doc_root + url ;
        else full_path = _doc_root + url;
        if(full_path.find("~")!=full_path.npos) FatalError("Invalid File Path! "+ full_path );
        realpath(full_path.c_str(),buffer);
        full_path = string(buffer);
        if(url.substr(url.length()-1) == "/" && url.length() > 1) full_path += "/";
        cout << full_path << endl;
        try{is_valid = validate_file(full_path);}
        catch(const string msg){
            log->error(msg);
        }
        if(is_valid == 1) {
            headers = build_headers(full_path, 200);
        } else if (is_valid == -1) {
            headers = build_headers(full_path, 404);
        }
        
    }

        //build headers

        if(client_error == false){
            if(is_valid == 1) {
                headers = build_headers(full_path, 200);
            } else if (is_valid == -1) {
                headers = build_headers(full_path, 404);
            }
        }
        send(client_sock, (void*) headers.c_str(), (ssize_t) headers.size(), 0);


        if(is_valid == 1 && client_error == false){
		    struct stat finfo;
		    int fd = open(full_path.c_str(), O_RDONLY);
		    fstat(fd, &finfo);
		    off_t off = 0;
            ulong f_size =  get_file_size(full_path.c_str());
            int h = 0;
            while((f_size>0)&& (h = sendfile(client_sock, fd,  &off,  BUFFSIZ)) > 0){
                f_size -= h;
            }
	    }
    lines.clear();

    if(connection_close == true || client_error == true) break;//If Connection: close or 400 ERROR, close the socket
    //pipeline ends
    }
     
    //  cout<<"Connection Closed"<<endl;
    //  close(client_sock);
     return (connection_close || client_error);
 }