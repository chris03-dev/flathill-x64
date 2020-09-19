/*	
	Copyright (c) 2020, chris03-dev
	
	The following source code is licensed under the UPL-v1.0.
	Please read LICENSE.md for details.
*/

#include <map>

//Holds variables
struct compilerdata {
	std::map<std::string, std::string> 
		d8m, d16m, d32m, d64m;
	
	compilerdata();
};

//To identify datatypes
enum vartype_enum {
	FH_DEFAULT = 0,
	
	//Regular types
	FH_BYTE = 1,
	FH_WORD = 2,
	FH_DWORD = 3,
	FH_QWORD = 4
};

extern char quotechar;

bool isnumeric(const std::string &);
unsigned int findnoq(const std::string &s, const char &c);
unsigned int findnoq(const std::string &s, const std::string &f);
unsigned int findnoqs(const std::string &s, const char &c, unsigned int i);
unsigned int findnoqs(const std::string &s, const std::string &f, unsigned int i);
std::string nextword(const std::string &, unsigned int);
std::string lexer(const std::string &);
std::string removecom(std::string);
int lexereq(std::stringstream &, const compilerdata &, std::string, const std::string &);