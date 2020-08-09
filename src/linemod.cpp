/*
	The following source code is licensed under the BSD 2-Clause Patent License.
	Please read LICENSE.md for details.
*/

#include <iostream>
#include <string>
#include <sstream>
#include <set>
#include "linemod.h"

compilerdata::compilerdata() {
	d8it = d8m.begin(),
	d16it = d16m.begin(),
	d32it = d32m.begin(),
	d64it = d64m.begin();
}

//Abstract syntax tree
/*
	
	Note: I'm currently using an unconventional method for parsing
	equations; I'm using 3 tokens instead of a full binary tree, or
	prefix equation for this purpose.
	
	The method in question is in the lexereq function.
	
	If others have to use the AST, they might have something my code 
	doesn't have right now... but I can't see yet why it should...
	
*/

//Check if std::string is a number
bool isnumeric(const std::string &s) {
	const static std::set<char>
		alphanum = {
			'a',
			'b',
			'c',
			'd',
			'e',
			'f',
			'A',
			'B',
			'C',
			'D',
			'E',
			'F'
		};
	
	for (unsigned int i = 0; i < s.size(); ++i) {
		//Check if binary or hex
		if (s.size() > 2) {
			i = 1;
			if (s[0] == '0' and (s[1] == 'x' or s[1] == 'b')) {
				if (not (alphanum.count(s[i]) or isdigit(s[i])))
					return 0;	
			}
			else return 0;
		}
		
		//Check if number
		else if (not isdigit(s[i])) 
			return 0;	
	}
	return 1;
}

char quotechar = 0;		//Used to check if inside a quote

//Check if found outside quotes
unsigned int findnoq(const std::string &s, const char &c) {
	//Get position of substring
	for (unsigned int i = 0; i < s.size(); ++i) {
		if (not quotechar) {
			if (s[i] == c) return i;
			if (s[i] == '\'' or s[i] == '"') quotechar = s[i];
		}
		else if (quotechar == s[i]) quotechar = 0;
	}
	return s.size();
}
//Check if found outside quotes
unsigned int findnoq(const std::string &s, const std::string &f) {
	
	//Get position of substring
	for (unsigned int i = 0; i < s.size(); ++i) {
		if (not quotechar) {
			if (s.substr(i, s.size() - i).find(f) == 0) {
				//std::cout << "SUCESS!	" << i << std::endl;
				return i;
			}
			if (s[i] == '\'' or s[i] == '"') quotechar = s[i];
		}
		else if (quotechar == s[i]) quotechar = 0;
	}
	
	return s.size();
}

//Skip whitespace for new token
std::string nextword(const std::string &s, int n) {
	while (
		s[n] == ' ' or 
		s[n] == '\n' or 
		s[n] == '\r' or 
		s[n] == '\t' or
		s[n] == ':' or
		s[n] == ';'
	)
		++n;
	
	return s.substr(n, s.length() - n);
}

//Get token or word in a string
std::string lexer(const std::string &s) {
	unsigned int n = 0;
	while (
		s[n] != '\0' and
		s[n] != ' ' and
		s[n] != '\r' and
		s[n] != '\n' and
		s[n] != '\t' and
		s[n] != ':' and
		s[n] != ';'
	)
		n++;
	
	if (n > s.size()) return "";
	return s.substr(0, n);
}

//Remove comments
std::string removecom(std::string s) {
	static bool incom = 0;		//For multiline comments
	
	//Check if single-line comment exists
	if (findnoq(s, '#') < s.size())
		return s.substr(0, findnoq(s, '#'));
	
	//Check if multiline comment exists
	if (findnoq(s, "_<") < s.size()) {
		incom = 1;
		return s.substr(0, findnoq(s, "_<"));
	}
	else if (incom) {
		quotechar = 0;
		
		if (findnoq(s, ">_") < s.size()) {
			incom = 0;
			return s.substr(findnoq(s, ">_") + 2, s.size() - findnoq(s, ">_") - 2);
		}
		else return "";
	}

	return s;
}

//Parse equations without need for AST (explanation below end of function)
int lexereq(
std::stringstream &out, 
const compilerdata &cdata,
std::string s, 
const std::string &sscope) {
	unsigned int 
		tokenptr = 0,			//Pointer for set of tokens
		lexcount = 0,  			//Get lexercount of equation
		varlexc[2] = {0, 0};	//Get lexercount of lvalue and rvalue
	
	std::string token[3];				//All 3 tokens are used in 1st operation,
										//and the last 2 are used in subsequent operations
	const static std::set<std::string>
		operators = {
			"+",		//Add
			"-",		//Sub
			"*",		//Mul
			"/",		//Div
			"&",		//And
			"!",		//Not
			"|",		//Or
			"%",		//Xor
		};
	bool 
		isglobal[2] = {0, 0},
		isnum[2] = {1, 1},
		isequ[2] = {0, 0},
		equend = 0;
	
	
	std::cout << "START" << std::endl;
	out << "xor rax, rax" << std::endl;
	
	for (
		std::string lex = lexer(s); 
	    lex != "\0"; 
		++lexcount,
	    s = nextword(s.substr(lex.size(), s.size() - lex.size()), 0),
	    lex = lexer(s)
	) {
		token[tokenptr] = lex;
		std::cout << "TOKENA[" << tokenptr << "]:	'" << token[tokenptr] << "' : '" << s << "'	:	" << lexcount << std::endl;
		
		//Check if token is not an operator
		if (tokenptr != 1) {
			//Check if token leads to another equation
			if (token[(tokenptr/2) * 2].find("(") < token[(tokenptr/2) * 2].size()) {
				isequ[tokenptr/2] = 1;
				
				if (tokenptr/2 == 1) out << "push rax" << std::endl;
				
				//Check if parenthesis is separate from token
				if (token[(tokenptr/2) * 2] == "(") {
					s = nextword(s, 1);
					varlexc[tokenptr/2] = 1 + lexereq(out, cdata, s, sscope);
				}
				else varlexc[tokenptr/2] = lexereq(out, cdata, s.substr(1, s.size() - 1), sscope);
				
				std::cout << "LEXC:	" << s << "	:	" << varlexc[tokenptr/2] << " + " << lexcount << std::endl;
				
				lexcount += varlexc[tokenptr/2];
				
				//Remove all tokens inside parenthesized equation
				for (; varlexc[tokenptr/2] > 0; --varlexc[tokenptr/2]) {
					std::string l = lexer(nextword(s, 0));
					if (l.find(")"))
						s = nextword(s, l.size());
					else
						s = nextword(s, l.size());
				}
				
				std::cout << "STRFT:	" << s << std::endl;
				
			}
			
			//Check if token is end of the parenthesized equation
			else if (tokenptr == 2) {
				if (token[2].find(")") < token[(tokenptr/2) * 2].size()) {
					token[2] = token[2].substr(0, token[2].find(')'));
					equend = 1;
				}
			}
			
			//Check if token is variable or number
			else if (not isnumeric(token[(tokenptr/2) * 2])) {
				isnum[tokenptr/2] = 0;
				
				//Check if token is global or stack variable
				if (cdata.d8m.count("G_" + token[tokenptr])
				or cdata.d16m.count("G_" + token[tokenptr])
				or cdata.d32m.count("G_" + token[tokenptr])
				or cdata.d64m.count("G_" + token[tokenptr]))
					isglobal[tokenptr/2] = 1;
				
				else if (
				not (cdata.d8m.count(sscope + token[tokenptr])
				or cdata.d16m.count(sscope + token[tokenptr])
				or cdata.d32m.count(sscope + token[tokenptr])
				or cdata.d64m.count(sscope + token[tokenptr]))) {
					std::cerr << "Error: Variable '" << token[tokenptr] << "' not found." << std::endl;
					return 0;
				}
			}
		}
		//Check if valid operator
		else if (not operators.count(token[1])) {
			if (token[1].find(")") < token[1].size()) {
				std::cout << "Hmm..." << std::endl;
				return lexcount - 1;
			}
			
			std::cerr << "Error: '" << token[1] << "' is not an operator." << std::endl;
			return 0;
		}
		
		std::cout << "TOKENB[" << tokenptr << "]:	'" << token[tokenptr] << "' : '" << s << "'	:	" << lexcount << std::endl;
		
		//Determine if operation is ready to parse
		if (tokenptr < 2) ++tokenptr;
		else {
			std::string
				lvalue, 
				rvalue,
				reg_ax,
				reg_bx,
				lvarsize,
				rvarsize;
			
			//First token
			if (token[0] != "" and not isequ[0]) {
				if (not isnum[0]) {
					if (isglobal[0]) lvalue = "G_" + token[0];
					else lvalue = sscope + token[0];
					
					if (cdata.d8m.count(lvalue)) 	reg_ax = "al";
					if (cdata.d16m.count(lvalue))	reg_ax = "ax";
					if (cdata.d32m.count(lvalue))	reg_ax = "eax";
					if (cdata.d64m.count(lvalue))	reg_ax = "rax";
					
					lvalue = "[" + lvalue + "]";
				}
				else {
					lvalue = token[0];
					
					unsigned int vcomp;
					
					if (lvalue[0] == '0' and (lvalue[1] == 'x' or lvalue[1] == 'b'))
						vcomp = std::stoul(lvalue, nullptr, 16);
					else vcomp = std::stoi(lvalue.c_str());
					
					if (vcomp > 0xffffffff) 	reg_ax = "rax";
					else if (vcomp > 0xffff)	reg_ax = "eax";
					else if (vcomp > 0xff)  	reg_ax = "ax";
					else                    	reg_ax = "al";
				}
			}
			
			//Second token
			if (not isequ[1]) {
				if (not isnum[1]) {
					if (isglobal[1]) rvalue = "G_" + token[2];
					else rvalue = sscope + token[2];
					
					rvalue = "[" + rvalue + "]";
				}
				else {
					rvalue = token[2];
					
					unsigned int vcomp;
					
					if (rvalue[0] == '0' and (rvalue[1] == 'x' or rvalue[1] == 'b'))
						vcomp = std::stoul(rvalue, nullptr, 16);
					else vcomp = std::stoi(rvalue.c_str());
					
					if (vcomp > 0xffffffff) 	reg_bx = "rbx";
					else if (vcomp > 0xffff)	reg_bx = "ebx";
					else if (vcomp > 0xff)  	reg_bx = "bx";
					else                    	reg_bx = "bl";
				}
			}
			
			//std::cout << "EQUATE: '" << s << "'" << std::endl;
			std::cout << "LVALUE:	'" << lvalue << "'" << std::endl;
			std::cout << "RVALUE:	'" << rvalue << "'" << std::endl;
		
			//PARSE THE OPERATION!
			if (isequ[1]) {
				out << "mov rbx, rax" << std::endl;
				out << "pop rax" << std::endl;
			}
			
			if (not isequ[0] and token[0] != "") {
				out << "mov " << reg_ax << ", " << lvalue << std::endl;
			}
			
			if (isequ[1]) {
				if (token[1] == "+") out << "add rax, rbx" << std::endl;
				if (token[1] == "-") out << "sub rax, rbx" << std::endl;
				if (token[1] == "*") out << "mul rbx" << std::endl;
				if (token[1] == "/") out << "div rbx" << std::endl;
			}
			else {
				if (token[1] == "+")
					out << "add rax, " << rvalue << std::endl;
				if (token[1] == "-")
					out << "sub rax, " << rvalue << std::endl;
				if (token[1] == "*") {
					out << "mov rbx, " << rvalue << std::endl;
					out << "mul rbx" << std::endl;
				}
				if (token[1] == "/") {
					out << "mov rbx, " << rvalue << std::endl;
					out << "div rbx" << std::endl;
				}
			}
			
			if (equend) {
				std::cout << "EQEND SUCCESS" << std::endl;
				return lexcount;
			}
			
			//Restore default values, never use token[0] again
			token[0] = "";
			isnum[1] = 1;
			isglobal[1] = 0;
			isequ[1] = 0;
			
			tokenptr = 1;
		}
	}
	
	//Only if equation involves a single value
	if (token[0] != "") {
		std::string lvalue, reg_ax;
		
		//Check if variable or number
		if (not isnum[0]) {
			//Check if token is global or stack variable
			if (isglobal[0]) lvalue = "G_" + token[0];
			else lvalue = sscope + token[0];
			
			if (cdata.d8m.count(lvalue)) 	reg_ax = "al";
			if (cdata.d16m.count(lvalue))	reg_ax = "ax";
			if (cdata.d32m.count(lvalue))	reg_ax = "eax";
			if (cdata.d64m.count(lvalue))	reg_ax = "rax";
			
			lvalue = "[" + lvalue + "]";
		}
		else {
			lvalue = token[0];
			
			unsigned int vcomp;
			
			if (lvalue[0] == '0' and (lvalue[1] == 'x' or lvalue[1] == 'b'))
				vcomp = std::stoul(lvalue, nullptr, 16);
			else vcomp = std::stoi(lvalue.c_str());
			
			if (vcomp > 0xffffffff) 	reg_ax = "rax";
			else if (vcomp > 0xffff)	reg_ax = "eax";
			else if (vcomp > 0xff)  	reg_ax = "ax";
			else                    	reg_ax = "al";
		}
		
		out << "mov " << reg_ax << ", " << lvalue << std::endl;
	}
	
	std::cout << "SUCCESS!" << std::endl;
	return 1;
}
/*
	This function does NOT involve using an AST 
	or parsing in prefix.
	
	The basic concept is as goes:
		- In the first run:
			- Get 3 tokens
				- Token A and C are the numbers/variables
				- Token B is the operator
			- Parse the 3 tokens
				- Token B, the operator, decides how the tokens
				  are parsed, in assembly language
				- In case Token A or C has '(', the equation will 
				  be recursed
					- In case token B has ')', the function will 
					  return to its previous instance. No values
					  needed to carry on the equation
				- After the first parsing, Token A will be left blank
				  because it becomes repetitive for parsing
		- In subsequent runs:
			- Get 2 tokens
				- Only Token B and C are updated
			- Parse the 2 tokens
				- Same behavior as first run, except Token A is 
				  no longer involved (and in case you ask why, you 
				  probably didn't read the whole concept and just 
				  skipped to this part for some reason)
		
*/
