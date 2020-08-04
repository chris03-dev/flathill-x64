#include <map>

struct compilerdata {
	std::map<std::string, std::string> 
		d8m, d16m, d32m, d64m;
		
	std::map<std::string, std::string>::iterator
		d8it, d16it, d32it, d64it;
	
	compilerdata();
};

std::string nextword(const std::string &, int);
bool isnumeric(const std::string &);
std::string lexer(const std::string &);
std::string removecom(std::string);
int lexereq(std::stringstream &, compilerdata &, std::string, const std::string &);