#ifndef PATHHANDLER_H
#define PATHHANDLER_H

#include <string>
#include <vector>

using namespace std;

//set the program path
void setProgramPath(char* program);
//return program path
string getProgramPath();
string getRoot();
vector<string> getRootTokens();

string parseURL(string path);

//get the real path of the path
string getRealPath(char* path);

//print error message and abort
void FatalError(const string ErrMsg);

//split string

vector <string> &split(const string &str, 
const string delimiters, 
vector<string>  &elements,
bool skip_empty = true);

//check if the direction or file exists

void checkDir(const string &dir);
void checkFile(const string &filepath);

#endif