/*
	Copyright (c) 2020, chris03-dev
	
	The following source code is licensed under the UPL-v1.0.
	Please read LICENSE.md for details.
*/

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include "linemod.h"

//Abstract syntax tree
/*
	
	Note: I'm currently using an unconventional method for parsing
	equations; I'm using 3 tokens instead of a full binary tree, or
	prefix equation for this purpose.
	
	The method in question is in the lexereq function.
	
	If others have to use the AST, they might have something my code 
	doesn't have right now... but I can't see yet why it should...
	
*/

compilerdata::compilerdata() {}

//Check if std::string is a number
bool isnumeric(const std::string &s) {
	const static std::set<char>
		alphanum = {
			'a', 'b', 'c', 'd', 'e', 'f',
			'A', 'B', 'C', 'D', 'E', 'F'
		};
	
	for (unsigned int i = 0; i < s.size(); ++i) {
		//Check if binary or hexadecimal
		if (s.size() > 2) {
			if ((s[0] == '0' and (s[1] == 'x' or s[1] == 'b'))) {
				if (i < 2) continue;
				if (not (alphanum.count(s[i]) or isdigit(s[i])))
					return 0;
			}
			else if (s[0] == '-') {
				if (not i) continue;
				if (not isdigit(s[i])) 
					return 0;
			}
			else if (not isdigit(s[i])) 
				return 0;
		}
		
		else if (s.size() > 1) {
			if (s[0] == '-') {
				if (not i) continue;
				if (not isdigit(s[i])) 
					return 0;
			}
			else if (not isdigit(s[i])) 
				return 0;
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

//Check if found outside quotes
unsigned int findnoqs(const std::string &s, const char &c, unsigned int n) {
	//Get position of substring
	for (unsigned int i = 0; i < s.size(); ++i) {
		if (not quotechar) {
			if (s[i] == c) {
				if (n) --n;
				else return i;
			}
			if (s[i] == '\'' or s[i] == '"') quotechar = s[i];
		}
		else if (quotechar == s[i]) quotechar = 0;
	}
	return s.size();
}
//Check if found outside quotes
unsigned int findnoqs(const std::string &s, const std::string &f, unsigned int n) {
	//Get position of substring
	for (unsigned int i = 0; i < s.size(); ++i) {
		if (not quotechar) {
			if (s.substr(i, s.size() - i).find(f) == 0) {
				if (n) --n;
				else return i;
			}
			if (s[i] == '\'' or s[i] == '"') quotechar = s[i];
		}
		else if (quotechar == s[i]) quotechar = 0;
	}
	
	return s.size();
}

//Skip whitespace for new token
std::string nextword(const std::string &s, unsigned int n) {
	while (
		n < s.size() and
		s[n] != '\0' and 
		(s[n] == ' ' or 
		s[n] == '\n' or 
		s[n] == '\r' or 
		s[n] == '\t')
	)
		n++;

	return s.substr(n, s.size() - n);
}
//Get token or word in a string
std::string lexer(const std::string &s) {
	unsigned int n = 0;
	while (
		n < s.size() and
		s[n] != '\0' and
		s[n] != ' ' and
		s[n] != '\r' and
		s[n] != '\n' and
		s[n] != '\t' and
		s[n] != ',' and
		s[n] != ':' and
		s[n] != ';' and
		s[n] != '[' and
		s[n] != ']'
	)
		++n;
	
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
		tokenptr = 0,	     	//Pointer for set of tokens
		lexcount = 0,      	 	//Get lexercount of overall equation
		varlexc = 0;       	 	//Get lexercount of parenthesized equation
	
	int addrc[2] = {0, 0};	 	//Get address or value of lvalue and rvalue
	char addrprc[2] = {0, 0};	//Get precision of dereferencing value
	
	std::string token[3]; 	 	//All 3 tokens are used in 1st operation,
	                      		//and the last 2 are used in subsequent operations
	
	//Check for endpoints
	if (findnoq(s, ';') < s.size()) s = s.substr(0, findnoq(s, ';'));
	
	const static std::set<std::string>
		operators = {
			"+", "-", "*", "/",
			"&", "!", "|", "%"
		};
	bool 
		isglobal[2] = {0, 0},		//Global flag
		isnum[2] = {1, 1},   		//Number flag
		isequ[2] = {0, 0},   		//Equation flag
		isfun[2] = {0, 0},   		//Function flag
		isarg[2] = {0, 0},   		//Argument flag
		equend = 0;          		//End of parenthesized equation (only in recursion)
	
	std::cout << "START" << std::endl;
	
	for (
		std::string lex = lexer(s); 
	    lex.size() or s.size(); 
		++lexcount,
	    s = nextword(s, lex.size()),
	    lex = lexer(s)
	) {
		token[tokenptr] = lex;
		std::cout << "TOKENA[" << tokenptr << "]:	'" << token[tokenptr] << "' : '" << s << "'	:	" << lexcount << std::endl;
		
		//Check if token is not an operator
		if (tokenptr != 1) {
			std::cout << s[token[tokenptr].size()] << (char) 10;
			//Get address or value iteration count
			if ((token[tokenptr][0] == '@' or token[tokenptr][0] == '$') and not (s[token[tokenptr].size()] == '[')) {
				for (; token[tokenptr][0] == '@'; ++addrc[tokenptr/2])
					token[tokenptr] = token[tokenptr].substr(1, token[tokenptr].size() - 1);
				
				for (; token[tokenptr][0] == '$'; --addrc[tokenptr/2]) {
					if (token[tokenptr][2] == '.') switch (token[tokenptr][1]) {
						case 'b':  case 'w': 
						case 'd':  case 'q': 
							addrprc[tokenptr/2] = token[tokenptr][1];
							token[tokenptr] = token[tokenptr].substr(2, token[tokenptr].size() - 2);
							break;
						default: break;
					}
					token[tokenptr] = token[tokenptr].substr(1, token[tokenptr].size() - 1);
				}
			}
			
			//Check if token leads to parenthesized equation
			if (token[tokenptr].find('(') < token[(tokenptr/2) * 2].size()) {
				isequ[tokenptr/2] = 1;
				
				//Save current lvalue
				//if (tokenptr/2 == 1) out << "push rax" << std::endl;
				
				//Check if parenthesis is separate from token
				if (token[tokenptr] == "(") {
					s = nextword(s, 1);
					varlexc = 1 + lexereq(out, cdata, s, sscope);
				}
				else varlexc = lexereq(out, cdata, s.substr(1, s.size() - 1), sscope);
				
				std::cout << "LEXC:	" << s << "	:	" << varlexc << " + " << lexcount << std::endl;
				
				lexcount += varlexc;
				lex = "";
				
				//Remove all tokens inside parenthesized equation
				for (; varlexc > 0; --varlexc) {
					std::string l = lexer(nextword(s, 0));
					s = nextword(s, l.size());
				}
				
				std::cout << "STRFT:	" << s << std::endl;
			}
			
			//Check if token is end of the parenthesized equation
			if (token[2].find(')') < token[tokenptr].size()) {
				token[2] = token[2].substr(0, token[2].find(')'));
				equend = 1;
			}
			
			//Prefix conditions
			//Check if token leads to function
			if ((token[tokenptr][0] == '@' or token[tokenptr][0] == '$') and (s[token[tokenptr].size()] == '[')) {
				isfun[tokenptr/2] = 1;
				bool dllcall;	//Check if calling DLL or static function
				
				std::string 
					fnname, 	//Get function name to call
					reg_xx; 	//Get register for storing parameters
				
				std::string param;
				unsigned int 
					largc = -1,  	//Get parameter count
					sqbuf = 0,  	//Check if inside nester square brackets
					sqcount = 0;	//Get number of square brackets
				
				dllcall = (token[tokenptr][0] == '$');
				fnname = s.substr(1, s.find('[') - 1);
				s = nextword(s, fnname.size() + 2);
				
				//Set function name with braces if calling dll function
				if (dllcall) fnname = '[' + fnname + ']';
				
				//Save current lvalue
				//if (tokenptr == 2) out << "push rax" << std::endl;
				
				//Check if there are parameters
				if (s[0] == ']')
					s = s.substr(1, s.size() - 1);
				
				//Get parameters
				else for (
					unsigned int i = 0;
					i < s.size();
					i++
				) {
					//if (i == findnoqs(s, s[i], (s[i] == ']') ? sqcount : 0))
					switch (s[i]) {
						case ',': 
							if (not sqbuf) {
								++largc; 
								param = s.substr(0, findnoqs(s, ',', sqcount)); 
								s = s.substr(param.size() + 1, s.size() - param.size() - 1);
							} 
							break;
						case ']': 
							if (not sqbuf) {
								++largc; 
								param = s.substr(0, findnoqs(s, ']', sqcount)); 
								s = s.substr(param.size() + 1, s.size() - param.size() - 1);
							} 
							else --sqbuf; 
							break;
						case '[': 
							++sqbuf; 
							++sqcount; 
							break;
					}
					
					//std::cout << "COUNTER:	" << i << "	:	" << s[i] << "	||	";
					//std::cout << "FINDERS:	" << sqbuf << "	:	" << findnoqs(s, ']', sqcount) << "\n";
					//std::cout << "RESULTS:	'" << s << "'\n";
					
					if (param.size() or i == s.size() - 1) {
						std::cout << "PARAM:	'" << param << "' -> '" << s << "'\n";
						
						out << std::endl;
						lexereq(out, cdata, nextword(param, 0), sscope);
						
						//Set register based on parameter
						if (largc < 5) {
							switch (largc) {
								#if _WIN32
								case 0: reg_xx = "rcx"; break;
								case 1: reg_xx = "rdx"; break;
								case 2: reg_xx = "r8";  break;
								case 3: reg_xx = "r9";  break;
								#endif
								#if __linux__
								case 0: reg_xx = "rdi"; break;
								case 1: reg_xx = "rsi"; break;
								case 2: reg_xx = "rdx"; break;
								case 3: reg_xx = "rcx"; break;
								case 4: reg_xx = "r8";  break;
								case 5: reg_xx = "r9";  break;
								#endif
							}
							
							out << "mov " << reg_xx << ", rax" << std::endl;
						}
						else out << "push rax" << std::endl;
						
						//Check if end of function
						if (not s.size()) {
							std::cout << "EJECT 1\n";
							break;
						}
						i = 0;
					}
					
					//Reset parameter
					param = "";
				}
				
				//Call the function
				out << "call " << fnname << std::endl;
				lex = "";
			}
			
			//Check if token is a function argument
			if (token[tokenptr][0] == '%') {
				isarg[tokenptr/2] = 1;
			}
			
			//Check if token is variable or number
			if (not (isequ[tokenptr/2] or isfun[tokenptr/2] or isarg[tokenptr/2] or isnumeric(token[tokenptr]))) {
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
			if (token[1].find(")") < token[1].size())
				return lexcount;
			
			std::cerr << "Error: '" << token[1] << "' is not an operator." << std::endl;
			return 0;
		}
		
		std::cout << "TOKENB[" << tokenptr << "]:	'" << token[tokenptr] << "' : '" << s << "'	:	" << lexcount << std::endl;
		
		//Determine if operation is ready to parse
		if (tokenptr < 2) ++tokenptr;
		else {
			std::string
				lvaluei,	//Initial lvalue
				rvaluei,	//Initial rvalue
				lvalue, 	//Output lvalue
				rvalue, 	//Output rvalue
				reg_ax, 	//ax register
				reg_bx; 	//bx register
			
			std::cout << "LEXCD:	" << lexcount << std::endl;
			
			//First token
			if (token[0].size() and not (isequ[0] or isfun[0] or isarg[0])) {
				if (not isnum[0]) {
					if (isglobal[0]) lvaluei = "G_" + token[0];
					else lvaluei = sscope + token[0];
					
					if (cdata.d8m.count(lvaluei)) 	reg_ax = "al";
					if (cdata.d16m.count(lvaluei))	reg_ax = "ax";
					if (cdata.d32m.count(lvaluei))	reg_ax = "eax";
					if (cdata.d64m.count(lvaluei))	reg_ax = "rax";
					
					if (isglobal[0]) lvalue = lvaluei;
					else lvalue = token[0];
					lvalue = "[" + lvalue + "]";
				}
				else {
					lvalue = token[0];
					
					unsigned int vcomp;
					
					if (lvalue[0] == '0' and (lvalue[1] == 'x' or lvalue[1] == 'b'))
						vcomp = std::stoul(lvalue, nullptr, 16);
					else vcomp = std::stoi(lvalue.c_str());
					
					if (vcomp > 0xffffffff)	reg_ax = "rax";
					else                   	reg_ax = "eax";
				}
			}
			
			std::cout << "LEXCD1:	" << isarg[1] << std::endl;
			
			//Second token
			if (not (isequ[1] or isfun[1] or isarg[1])) {
				if (not isnum[1]) {
					if (isglobal[1]) rvaluei = "G_" + token[2];
					else rvaluei = sscope + token[2];
					
					if (cdata.d8m.count(rvaluei)) 	reg_bx = "bl";
					if (cdata.d16m.count(rvaluei))	reg_bx = "bx";
					if (cdata.d32m.count(rvaluei))	reg_bx = "ebx";
					if (cdata.d64m.count(rvaluei))	reg_bx = "rbx";
					
					if (isglobal[1]) rvalue = rvaluei;
					else rvalue = token[2];
					rvalue = "[" + rvalue + "]";
				}
				else {
					rvalue = token[2];
					
					unsigned int vcomp;
					if (rvalue[0] == '0' and (rvalue[1] == 'x' or rvalue[1] == 'b'))
						vcomp = std::stoul(rvalue, nullptr, 16);
					else vcomp = std::stoi(rvalue.c_str());
					
					if (vcomp > 0xFFFFFFFF) 	reg_bx = "rbx";
					else if (vcomp > 0xFFFF)	reg_bx = "ebx";
					else if (vcomp > 0xFF)  	reg_bx = "bx";
					else                    	reg_bx = "bl";
				}
			}
			/*
			std::cout << "LEXCD2:	" << lexcount << std::endl;
			
			std::cout << "LVALUE (" << addrc[0] << "):	'" << lvalue << "'" << std::endl;
			std::cout << "RVALUE (" << addrc[1] << "):	'" << rvalue << "'" << std::endl;
			*/
			//PARSE THE OPERATION!
			if (isequ[1] or isfun[1]) {
				out << "mov rbx, rax" << std::endl;
				if (not isfun[1] or (isfun[0] and isfun[1])) 
					out << "pop rax" << std::endl;
			}
			
			std::cout << "CHECK1" << std::endl;
			
			if (token[0].size()) {
				if (not (isequ[0] or isfun[0])) {
					//Get argument in lvalue
					if (isarg[0]) {
						unsigned int argc = std::stoi(token[0].substr(4, token[tokenptr].size() - 4)) - 1;
						
						switch (argc) {
							#ifdef _WIN32
							case 0: out << "mov rax, rcx" << std::endl; break;
							case 1: out << "mov rax, rdx" << std::endl; break;
							case 2: out << "mov rax, r8" << std::endl; break;
							case 3: out << "mov rax, r9" << std::endl; break;
							#endif
							#ifdef __linux__
							case 0: out << "mov rax, rdi" << std::endl; break;
							case 1: out << "mov rax, rsi" << std::endl; break;
							case 2: out << "mov rax, rdx" << std::endl; break;
							case 3: out << "mov rax, rcx" << std::endl; break;
							case 4: out << "mov rax, r8" << std::endl; break;
							case 5: out << "mov rax, r9" << std::endl; break;
							#endif
							default:
								out << "pop rax" << std::endl;
								break;
						}
					}
					
					//Set directly to value
					else if (not addrc[0]) {
						if (isnum[0]) {
							out << "mov " << reg_ax << ", " << lvalue << std::endl;
						}
						else {
							if (not (reg_ax == "rax" or reg_ax == "eax")) 
								out << "xor eax, eax" << std::endl;
							out << "mov " << reg_ax << ", " << lvalue << std::endl;
						}
					}
				}
				
				//Point to reference or value
				if (addrc[0]) {
					bool fst = 1;
					
					//Set to address
					while (addrc[0] > 0) {
						if (fst and not (isarg[0] or isequ[0] or isfun[0])) {
							out << "lea rax, " << lvalue << std::endl;
							fst = 0;
						}
						else out << "lea rax, [rax]" << std::endl;
						
						--addrc[0];
					}
					
					//Set to value
					while (addrc[0] < 0) {
						if (fst and not (isarg[0] or isequ[0] or isfun[0])) {
							out << "mov rsi, " << lvalue << std::endl;
							fst = 0;
						}
						
						if (addrc[0] < -1) out << "mov rsi, [rsi]" << std::endl;
						else switch (addrprc[0]) {
							case 'b': out << "xor eax, eax" << std::endl; out << "mov al, byte [rsi]" << std::endl; break;
							case 'w': out << "xor eax, eax" << std::endl; out << "mov ax, word [rsi]" << std::endl; break;
							case 'd': out << "mov eax, dword [rsi]" << std::endl; break;
							case 'q': out << "mov rax, [rsi]" << std::endl; break;
							default: std::cout << "BRUH\n"; break;
						}
						
						++addrc[0];
					}
				}
			}
			
			std::cout << "CHECK2" << addrc[1] << std::endl;
			
			//Point to reference or value
			if (addrc[1]) {
				bool fst = 1;
				
				//Set to address
				while (addrc[1] > 0) {
					if (fst and not (isarg[1] or isequ[1] or isfun[1])) {
						if (not isarg[1]) out << "lea rbx, " << rvalue << std::endl;
						fst = 0;
					}
					else out << "lea rbx, [rbx]" << std::endl;
					--addrc[1];
				}
				
				//Set to value
				while (addrc[1] < 0) {
					if (fst and not (isarg[1] or isequ[1] or isfun[1])) {
						out << "mov rsi, " << rvalue << std::endl;
						fst = 0;
					}
					if (addrc[1] < -1) out << "mov rsi, [rsi]" << std::endl;
					else switch (addrprc[1]) {
						case 'b': out << "mov bl, byte [rsi]" << std::endl; break;
						case 'w': out << "mov bx, word [rsi]" << std::endl; break;
						case 'd': out << "mov ebx, dword [rsi]" << std::endl; break;
						case 'q': out << "mov rbx, [rsi]" << std::endl; break;
						default: std::cout << "BRUH\n"; break;
					}
					
					++addrc[1];
				}
				
				isequ[1] = 1;		//Terrible fix, but okay
			}
			
			if (isequ[1] or isfun[1]) {
				if (token[1] == "+") out << "add rax, rbx" << std::endl;
				if (token[1] == "-") out << "sub rax, rbx" << std::endl;
				if (token[1] == "*") out << "mul rbx" << std::endl;
				if (token[1] == "/") out << "div rbx" << std::endl;
			}
			else {
				//Check arguments in rvalue
				if (isarg[1]) {
					unsigned int argc = std::stoi(token[2].substr(4, token[tokenptr].size() - 4)) - 1;
					
					switch (argc) {
						#ifdef _WIN32
						case 0: out << "mov rbx, rcx" << std::endl; break;
						case 1: out << "mov rbx, rdx" << std::endl; break;
						case 2: out << "mov rbx, r8" << std::endl; break;
						case 3: out << "mov rbx, r9" << std::endl; break;
						#endif
						#ifdef __linux__
						case 0: out << "mov rbx, rdi" << std::endl; break;
						case 1: out << "mov rbx, rsi" << std::endl; break;
						case 2: out << "mov rbx, rdx" << std::endl; break;
						case 3: out << "mov rbx, rcx" << std::endl; break;
						case 4: out << "mov rbx, r8" << std::endl; break;
						case 5: out << "mov rbx, r9" << std::endl; break;
						#endif
						default:
							out << "pop rbx" << std::endl;
							break;
					}
				}
				
				//Check operators
				{
					if (token[1] == "+")
						out << "add rax, " << rvalue << std::endl;
					if (token[1] == "-")
						out << "sub rax, " << rvalue << std::endl;
					if (token[1] == "*") {
						if (not isarg[1]) out << "mov " << reg_bx << ", " << rvalue << std::endl;
						out << "mul rbx" << std::endl;
					}
					if (token[1] == "/") {
						if (not isarg[1]) out << "mov " << reg_bx << ", " << rvalue << std::endl;
						out << "div rbx" << std::endl;
					}
					
					if (token[1] == "&")
						out << "and rax, " << rvalue << std::endl;
					if (token[1] == "|")
						out << "or rax, " << rvalue << std::endl;
					if (token[1] == "!")
						out << "not rax, " << rvalue << std::endl;
					if (token[1] == "%")
						out << "xor rax, " << rvalue << std::endl;
				}
			}
			
			if (equend) {
				std::cout << "EQEND SUCCESS" << std::endl;
				return lexcount + 1;
			}
			
			std::cout << "RELOOP" << std::endl;
			
			//Restore default values, never use token[0] again
			token[0] = "";
			isnum[1] = 1;
			isglobal[1] = 0;
			isequ[1] = 0;
			isfun[1] = 0;
			isarg[1] = 0;
			varlexc = 0;
			
			tokenptr = 1;
		}
	}
	
	//Only if equation involves a single value
	if (token[0].size()) {
		std::string lvaluei, lvalue, reg_ax;
		
		//Check if variable or number
		if (not (isequ[0] or isfun[0] or isarg[0])) {
			if (not isnum[0]) {
				//Check if token is global or stack variable
				if (isglobal[0]) lvaluei = "G_" + token[0];
				else lvaluei = sscope + token[0];
				
				if (cdata.d8m.count(lvaluei)) 	reg_ax = "al";
				if (cdata.d16m.count(lvaluei))	reg_ax = "ax";
				if (cdata.d32m.count(lvaluei))	reg_ax = "eax";
				if (cdata.d64m.count(lvaluei))	reg_ax = "rax";
				
				if (isglobal[0]) lvalue = lvaluei;
				else lvalue = token[0];
				lvalue = "[" + lvalue + "]";
			}
			else {
				lvalue = token[0];
				
				unsigned int vcomp;
				
				if (lvalue[0] == '0' and (lvalue[1] == 'x' or lvalue[1] == 'b'))
					vcomp = std::stoul(lvalue, nullptr, 16);
				else vcomp = std::stoi(lvalue.c_str());
				
				if (vcomp > 0xffffffff)	reg_ax = "rax";
				else                   	reg_ax = "eax";
			}
		}
		
		if (not (isequ[0] or isfun[0])) {
			//Get argument in lvalue
			if (isarg[0]) {
				unsigned int argc = std::stoi(token[0].substr(4, token[tokenptr].size() - 4)) - 1;
				
				switch (argc) {
					#ifdef _WIN32
					case 0: out << "mov rax, rcx" << std::endl; break;
					case 1: out << "mov rax, rdx" << std::endl; break;
					case 2: out << "mov rax, r8" << std::endl; break;
					case 3: out << "mov rax, r9" << std::endl; break;
					#endif
					#ifdef __linux__
					case 0: out << "mov rax, rdi" << std::endl; break;
					case 1: out << "mov rax, rsi" << std::endl; break;
					case 2: out << "mov rax, rdx" << std::endl; break;
					case 3: out << "mov rax, rcx" << std::endl; break;
					case 4: out << "mov rax, r8" << std::endl; break;
					case 5: out << "mov rax, r9" << std::endl; break;
					#endif
					default:
						out << "pop rax" << std::endl;
						break;
				}
			}
			
			//Set directly to value
			else if (not addrc[0]) {
				if (isnum[0]) {
					out << "mov " << reg_ax << ", " << lvalue << std::endl;
				}
				else {
					if (not (reg_ax == "rax" or reg_ax == "eax")) 
						out << "xor eax, eax" << std::endl;
					out << "mov " << reg_ax << ", " << lvalue << std::endl;
				}
			}
		}
		
		//Point to reference or value
		if (addrc[0]) {
			bool fst = 1;
			
			//Set to address
			while (addrc[0] > 0) {
				if (fst and not (isarg[0] or isequ[0] or isfun[0])) {
					fst = 0;
					out << "lea rax, " << lvalue << std::endl;
				}
				else out << "lea rax, [rax]" << std::endl;
				
				--addrc[0];
			}
			
			//Set to value
			while (addrc[0] < 0) {
				if (fst and not (isarg[0] or isequ[0] or isfun[0])) {
					out << "mov rsi, " << lvalue << std::endl;
					fst = 0;
				}
				
				if (addrc[0] < -1) out << "mov rsi, [rsi]" << std::endl;
				else switch (addrprc[0]) {
					case 'b': out << "xor eax, eax" << std::endl; out << "mov al, byte [rsi]" << std::endl; break;
					case 'w': out << "xor eax, eax" << std::endl; out << "mov ax, word [rsi]" << std::endl; break;
					case 'd': out << "mov eax, dword [rsi]" << std::endl; break;
					case 'q': out << "mov rax, [rsi]" << std::endl; break;
					default: std::cout << "BRUH\n"; break;
				}
				
				++addrc[0];
			}
		}
	}
	
	std::cout << "SUCCESS!" << std::endl;
	return lexcount;
}

/*
	This function does NOT involve using an AST 
	or parsing in prefix.
	
	The basic concept is as goes:
		- In the first run:
			- Get first 3 tokens
				- Token A and C are the numbers/variables
				- Token B is the operator
			- Parse the 3 tokens
				- Token B, the operator, decides how the tokens
				  are parsed, in assembly language
				- In case Token A or C has '(', the function will 
				  perform recursion on the parenthesized equation
					- In case token B has ')', the function will 
					  return the number of tokens to be removed by
					  its previous instance.
				
				- In case Token A or C has '@', the function will 
				  attempt to get the reference of the token
				- In case Token A or C has '$', the function will 
				  check if such token is a function:
					- If token is a function:
						- It will perform recursion on the function parameters
					- Otherwise:
						- It will assume the value is an address, and will attempt
						  to get the value held by the address
				  
				- After the first parsing, Token A will be left blank
				  because it becomes repetitive for parsing
		- In subsequent runs:
			- Get next 2 tokens
				- Only Token B and C are updated
			- Parse the 2 tokens
				- Same behavior as first run, except Token A is 
				  no longer involved (and in case you ask why, you 
				  probably didn't read the whole concept and just 
				  skipped to this part for some reason)
		
*/
