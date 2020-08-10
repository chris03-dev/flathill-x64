/*
	Copyright (c) 2020, chris03-dev
	
	The following source code is licensed under the BSD 2-Clause Patent License.
	Please read LICENSE.md for details.
*/

#include <map>

struct compilerdata {
	std::map<std::string, std::string> 
		d8m, d16m, d32m, d64m;
		
	std::map<std::string, std::string>::iterator
		d8it, d16it, d32it, d64it;
	
	compilerdata();
};

extern char quotechar;

bool isnumeric(const std::string &);
unsigned int findnoq(const std::string &s, const char &c);
unsigned int findnoq(const std::string &s, const std::string &f);
std::string nextword(const std::string &, int);
std::string lexer(const std::string &);
std::string removecom(std::string);
int lexereq(std::stringstream &, const compilerdata &, std::string, const std::string &);