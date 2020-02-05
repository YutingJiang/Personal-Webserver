#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <istream>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "PathHandler.hpp"
#include "logger.hpp"

string _program;
string _root;
vector <string> _rootTokens;


void setProgramPath(char* program) {
    char buff[PATH_MAX];
    realpath(program, buff);
    _program = string(buff);
    _rootTokens = split(_program,"/",_rootTokens);
    _rootTokens.pop_back();
    if(_rootTokens.size())_rootTokens.pop_back();
    _root="";
    for(uint i=0;i<_rootTokens.size();i++)
        _root+="/"+_rootTokens[i];
    if(_root.size()==0)_root="/";

    auto log = logger();
    log->info("Current program path is "+_program);
    log->info("Current program root is "+_root);
}

string getRealPath(char* path) {
    char real_path[PATH_MAX];
    realpath(path, real_path);
    return string(real_path);
}

string getProgramPath(){
    return string(_program);
}

string getRoot(){return _root;}

vector<string> getRootTokens(){return _rootTokens;}

string parseURL(string path) {
    string homedir(getenv("HOME"));
    char buff[PATH_MAX];
    if(path[0] == '~') path = homedir +"/"+ path.substr(1);
    if(path.find("~")!=path.npos)
				FatalError("Invalid Path: "+path);
	realpath(path.c_str(),buff);
    return string(buff);
}

//print error message and abort
void FatalError(const string ErrorMsg)
{   
    auto log=logger();
    log->error(ErrorMsg+"\nExiting!");
    exit(20);
} 

vector <string> &split(const string &str, 
const string delimiters, 
vector<string>  &elements,
bool skip_empty){
    string::size_type pos, prev = 0;
    uint dlen=delimiters.size();
    while ( ( pos = str.find(delimiters, prev) ) != string::npos ) {
        if ( pos > prev ) {
            if ( skip_empty && pos==prev ) break;
            elements.emplace_back( str, prev, pos - prev );
        }
        prev = pos+dlen;
    }
    if ( prev < str.size() ) elements.emplace_back( str, prev, str.size() - prev );
    return elements;
}

void checkDir(const string &dir){
    struct stat info;

    if( stat( dir.c_str(), &info ) != 0 )
        throw  string("Cannot Access Directory or Directory Doesn`t Exist!");
    else if( !(info.st_mode & S_IFDIR) ) 
        throw  string("Not a Directory!");
}

void checkFile(const string &filepath){
    struct stat info;

    if( stat( filepath.c_str(), &info ) != 0 )
        throw  string("Cannot Access Directory or Directory Doesn`t Exist!");
    else if( info.st_mode & S_IFDIR )  
        throw string("Not a File!");
}