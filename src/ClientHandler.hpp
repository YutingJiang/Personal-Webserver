#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <netdb.h>
#include <sys/types.h>
#include <iostream>
#include <string>

#include "logger.hpp"
#include "PathHandler.hpp"

using namespace std;

class  ClientHandler {
    private:
        const string _doc_root;
        const vector<string> _doc_root_tokens;
        const unordered_map<string,string> &_mime;

        int validate_file(string path);
        string build_headers(string filepath, int errcode);
        int get_file_size(const char* filepath);


    public:
    ClientHandler(
    const string doc_root,
    const vector<string> doc_root_tokens,
    const unordered_map<string,string> &mime);

    ~ClientHandler();
    
    bool launch(int client_sock, const char* buff);
};

#endif