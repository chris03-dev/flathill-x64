/*
	Copyright (c) 2020, chris03-dev
	
	The following source code is licensed under the UPL-v1.0.
	Please read LICENSE.md for details.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include "linemod.h"

using namespace std;

//Compiler data
compilerdata cdata;
vartype_enum vartype;

//Platform-dependency flags
#define PLATFORM_GLOBAL 	0
#define PLATFORM_WINDOWS	2
#define PLATFORM_LINUX  	3

//unsigned int platf = PLATFORM_GLOBAL;

vector<string> 
	fstr,       			//List of file lines
	fnstr;      			//Temporary list for function

ifstream input; 			//Input file
ofstream output;			//Assembly output file

stringstream
	outdata,    			//Used only for .data section in output file
	texttmp,    			//Used only for .text section in output file
	outtext;

int main(int argc, char **argv) {
	if (argc > 1) {
		cout << "Input detected." << endl;
		
		//Open and read input file
		cout << "Opening '" << argv[1] << "'..." << endl;
		input.open(argv[1]);
		{
			string s;
			while (getline(input, s))
				fstr.push_back(removecom(s));
		}
		
		for (unsigned int vsizetmp = 2; vsizetmp < 65536; vsizetmp *= 2) 
			if (fstr.size() == vsizetmp) {
				fstr.reserve(fstr.size() + 1);
				break;
			}
		
		input.close();
		
		#ifdef _WIN32
		outdata << "format PE64 console" << endl;
		outdata << "entry _start" << endl;
		
		//Library data
		outdata << "section '.idata' data readable import" << endl;
		outdata << "include \"win64a.inc\"" << endl;
        outdata << "library kernel32, 'kernel32.dll', \\" << endl;
        outdata << "        msvcrt,   'msvcrt.dll'" << endl;
        outdata << "import kernel32, ExitProcess, 'ExitProcess'" << endl;
        outdata << "import msvcrt, printf, 'printf', \\" << endl;
		outdata << "               malloc, 'malloc', \\" << endl;
		outdata << "               free,   'free'" << endl;
		
		outtext << "section '.text' code readable executable" << endl;
		#endif
		
		#ifdef __linux__
		outdata << "format ELF64 executable" << endl;
		outdata << "entry _start" << endl;
		
		outtext << "segment executable" << endl;
		#endif
		
		//Read every line of input file
		string
			scope = "G_", 			//Scope of variables
			line, linetmp;			//Line of input file
			
		unsigned int 
			flinenum = 0, 			//Line number
			jumpn = 0,   			//Block number
			stacklv = 0,  			//Bytes pushed in stack
			scopelv = 0;  			//Check if number of scopes in stack
		
		stack<unsigned int>
			scopelv_block;			//Store block data
		
		bool 
			datadecl = 0, 			//Check if section '.data' should be inserted
			flag_asm = 0; 			//Do-not-translate flag (code assumed as assembly)
		
		//Main pass loop
		for (auto it = fstr.cbegin(); it != fstr.end(); advance(it, 1), flinenum++, vartype = (vartype_enum) 0) { 
			line = nextword(*it, 0);
			linetmp = line;
			
			//Write line details on console
			cout << flinenum << "	(" << scopelv << "):	" << line << "		" << endl;	
			
			//Scope increment and declaration
			if (findnoq(linetmp, ':') < linetmp.size()) {
				scopelv++;
				
				//Check if not already inside function
				if (scope == "G_")
					scope = scope.substr(0, findnoq(scope, "_@N") + 3);
				cout << "Inc scope." << endl;
			}
			
			//Function end, scope decrement
			if (findnoq(linetmp, ";") < linetmp.size()) {
				//Check how many scope decrements
				--scopelv;
				
				//Restore line
				linetmp = line;
				
				if (not scopelv) {
					scope = "G_";
					jumpn = 0;
					cout << "Dec scope." << endl;
					
					if (stacklv) {
						outtext << endl;
						outtext << "add rsp, " << stacklv << endl;
						outtext << "pop rbp" << endl;
						
						stacklv = 0;
					}
				}
			}
			
			//Check if code should be treated as assembly code
			if (flag_asm) {
                //Check if end of flag_asm
				if (findnoq(line, ';') < line.size()) {
					line = line.substr(0, findnoq(line, ';'));
					flag_asm = 0;
				}
                
				//Check appropriate section to output
				if (scope == "G_") {
					//Output data header early
					if (datadecl) 
					#ifdef _WIN32
						outdata << "section '.data' data readable writeable" << endl;
					#endif
					#ifdef __linux__
						outdata << "segment writeable" << endl;
					#endif
					
					outdata << line << endl;
					datadecl = 0;
				}
				else 
					outtext << line << endl;
			}
			else {
				//Global variable declaration
				if (scope == "G_") {
					//Global variable declaration
					string varname;		//Get name of variable, also used for removing tokens
					
					if (linetmp.substr(0, 2) == "d8") {
						datadecl = 1;
						vartype = FH_BYTE;
						cout << "d8 " << varname << "\n";
					}
					else if (linetmp.substr(0, 3) == "d16") {
						datadecl = 1;
						vartype = FH_WORD;
						cout << "d16 " << varname << "\n";
					}
					else if (linetmp.substr(0, 3) == "d32") {
						datadecl = 1;
						vartype = FH_DWORD;
						cout << "d32 " << varname << "\n";
					}
					else if (linetmp.substr(0, 3) == "d64") {
						datadecl = 1;
						vartype = FH_QWORD;
						cout << "d64 " << varname << "\n";
					}
					
					//Remove datatype
					if (vartype != FH_DEFAULT) {
						switch (vartype) {
							case FH_BYTE:      linetmp = nextword(linetmp, 2); break;
							case FH_WORD:
							case FH_DWORD:
							case FH_QWORD:     linetmp = nextword(linetmp, 3); break;
							default: break;
						}
						
						do {
							varname = lexer(linetmp);
							int arrsize = 1;
							
							while (varname.size() and isnumeric(varname)) {
								if (varname[0] == '0' and (varname[1] == 'x' or 'n')) 
									arrsize *= stoul(varname.substr(2, varname.size() - 2), nullptr, (varname[1] == 'x') ? 16 : 2);
								
								else arrsize *= stoi(varname);
								linetmp = nextword(linetmp, varname.size());
								varname = lexer(linetmp);
							}
							
							cout << "HEY:	'" << linetmp << "'\n";
							linetmp = nextword(linetmp, varname.size());
							
							//Check if variable declaration contains value/s
							string sep = lexer(linetmp);
							
							if (sep.size() and sep[0] == '=' and not (arrsize - 1)) {
								linetmp = nextword(linetmp, 1);
								switch (vartype) {
									case FH_BYTE:      cdata.d8m.insert(pair<string, string>(scope + varname, lexer(linetmp)));    break;
									case FH_WORD:      cdata.d16m.insert(pair<string, string>(scope + varname, lexer(linetmp)));   break;
									case FH_DWORD:     cdata.d32m.insert(pair<string, string>(scope + varname, lexer(linetmp)));   break;
									case FH_QWORD:     cdata.d64m.insert(pair<string, string>(scope + varname, lexer(linetmp)));   break;
									default: break;
								}
							}
							else {
								switch (vartype) {
									case FH_BYTE:      cdata.d8m.insert(pair<string, string>(scope + varname, "res " + to_string(arrsize)));    break;
									case FH_WORD:      cdata.d16m.insert(pair<string, string>(scope + varname, "res " + to_string(arrsize)));   break;
									case FH_DWORD:     cdata.d32m.insert(pair<string, string>(scope + varname, "res " + to_string(arrsize)));   break;
									case FH_QWORD:     cdata.d64m.insert(pair<string, string>(scope + varname, "res " + to_string(arrsize)));   break;
									default: break;
								}
							}
							
							if (findnoq(linetmp, ',') < linetmp.size()) 
								linetmp = nextword(linetmp, findnoq(linetmp, ',') + 1);
							else linetmp = "";
						}
						while (linetmp.size());
						
						//Reset enum
						vartype = FH_DEFAULT;
					}
				}
				
				//Check if line is object declaration
				//Not yet implemented.
				
				//Check if line is function declaration
				if (linetmp.substr(0, 2) == "fn") {
					//Used to find all stack variables
					outtext << lexer(nextword(linetmp, 2)) << ":" << endl;
					
					cout << "Scope: 	F_" << lexer(nextword(linetmp, 2)) << "_" << endl;
					scope = "F_" + lexer(nextword(linetmp, 2)) + "_";
					
					cout << "SCOPELV BEGIN" << endl;
					
					if (lexer(nextword(linetmp, 2)) == "_start")
						outtext << "xor edx, edx" << endl;
					
					//Set to default
					stacklv = 0;
					unsigned int scopelvtmp = 1;
					
					auto itfn = next(fstr.begin(), flinenum);
					string 
						fnline = nextword(linetmp.substr(linetmp.find(':') + 1, linetmp.size() - linetmp.find(':') - 1), 0),
						varname;
					
					//Check end of function
					do if (line.size()) {
						//Increment scope
						while (findnoq(fnline, ':') < fnline.size()) {
							scopelvtmp++;
							fnline = fnline.substr(findnoq(fnline, ':') + 1, fnline.size() - findnoq(fnline, ':') - 1);
						}
						
						/*
						//Stack variable declaration
						if (fnline.substr(0, 2) == "d8") {
							varname = scope + lexer(nextword(fnline, 2));
							vartype = FH_BYTE;
							
							stacklv++;
							cdata.d8m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
						}
						else if (fnline.substr(0, 3) == "d16") {
							varname = scope + lexer(nextword(fnline, 3));
							vartype = FH_WORD;
							
							stacklv += 2;
							cdata.d16m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
						}
						else if (fnline.substr(0, 3) == "d32") {
							varname = scope + lexer(nextword(fnline, 3));
							vartype = FH_DWORD;
							
							stacklv += 4;
							cdata.d32m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
						}
						else if (fnline.substr(0, 3) == "d64") {
							varname = scope + lexer(nextword(fnline, 3));
							vartype = FH_QWORD;
							
							stacklv += 8;
							cdata.d64m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
						}
						
						//Pointer declaration
						else if (line.size() >= 6) {
							if (fnline.substr(0, 4) == "ptr8") {
								varname = scope + lexer(nextword(fnline, 4));
								vartype = FH_PTR_BYTE;
								
								stacklv += 8;
								cdata.ptr8m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
							}
							else if (fnline.substr(0, 5) == "ptr16") {
								varname = scope + lexer(nextword(fnline, 5));
								vartype = FH_PTR_WORD;
								
								stacklv += 8;
								cdata.ptr16m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
							}
							else if (fnline.substr(0, 5) == "ptr32") {
								varname = scope + lexer(nextword(fnline, 5));
								vartype = FH_PTR_DWORD;
								
								stacklv += 8;
								cdata.ptr32m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
							}
							else if (fnline.substr(0, 5) == "ptr64") {
								varname = scope + lexer(nextword(fnline, 5));
								vartype = FH_PTR_QWORD;
								
								stacklv += 8;
								cdata.ptr64m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
							}
							
							//Decrement scope
							while (findnoq(fnline, ';') < fnline.size()) {
								scopelvtmp--;
								fnline = fnline.substr(findnoq(fnline, ';') + 1, fnline.size() - findnoq(fnline, ';') - 1);
							}
						}
						*/
						
						//Stack variable declaration
						if (fnline.substr(0, 2) == "d8")       vartype = FH_BYTE;
						else if (fnline.substr(0, 3) == "d16") vartype = FH_WORD;
						else if (fnline.substr(0, 3) == "d32") vartype = FH_DWORD;
						else if (fnline.substr(0, 3) == "d64") vartype = FH_QWORD;
						
						int arrsize = 1;
						
						switch (vartype) {
							case FH_BYTE:      arrsize = 1; fnline = nextword(fnline, 2); break;
							case FH_WORD:      arrsize = 2; fnline = nextword(fnline, 3); break;
							case FH_DWORD:     arrsize = 4; fnline = nextword(fnline, 3); break;
							case FH_QWORD:     arrsize = 8; fnline = nextword(fnline, 3); break;
							default: break;
						}
						
						if (vartype != FH_DEFAULT) {
							do {
								varname = lexer(fnline);
								cout << "VARNAME:	" + varname << endl;
								
								while (vartype and varname.size() and isnumeric(varname)) {
									arrsize *= stoi(varname);
									fnline = nextword(fnline, varname.size());
									varname = lexer(fnline);
								}
								
								cout << "FINIHS" << endl;
								
								switch (vartype) {
									case FH_BYTE:      cout << "BYTE\n"; cdata.d8m.insert(pair<string, string>(scope + varname, "1"));    break;
									case FH_WORD:      cout << "WORD\n"; cdata.d16m.insert(pair<string, string>(scope + varname, "1"));   break;
									case FH_DWORD:     cout << "DWRD\n"; cdata.d32m.insert(pair<string, string>(scope + varname, "1"));   break;
									case FH_QWORD:     cout << "QWRD\n"; cdata.d64m.insert(pair<string, string>(scope + varname, "1"));   break;
									default: cout << "BRUH\n"; break;
								}
								cout << "FINIHS2" << endl;
								
								if (findnoq(fnline, ',') < fnline.size()) 
									fnline = nextword(fnline, findnoq(fnline, ',') + 1);
								else fnline = "";
								
								cout << "AB:	" + fnline << endl;
								stacklv += arrsize;
								outtext << varname << "	equ	rbp - " << stacklv << endl;
								
							} while (fnline.size());
							
							vartype = FH_DEFAULT;
						}
						
						//Decrement scope
						while (findnoq(fnline, ';') < fnline.size()) {
							scopelvtmp--;
							fnline = fnline.substr(findnoq(fnline, ';') + 1, fnline.size() - findnoq(fnline, ';') - 1);
						}
						
						advance(itfn, 1);
						fnline = nextword(*itfn, 0);
						varname = "";
					} 
					else {
						advance(itfn, 1);
						fnline = nextword(*itfn, 0);
						varname = "";
					}
					while (scopelvtmp > 0 and itfn != fstr.end());
					
					if (stacklv) {
						outtext << "push rbp" << endl;
						outtext << "mov rbp, rsp" << endl;
						outtext << "sub rsp, " << stacklv << endl;
					}
					
					linetmp = nextword(linetmp.substr(linetmp.find(':') + 1, linetmp.size() - linetmp.find(':') - 1), 0);
					cout << "SCOPELV END\nLINEDCL:	'" << linetmp << "'"<<endl;
				}
				
				//Check non-return function call
				if (linetmp[0] == '@' or linetmp[0] == '$') {
					bool dllcall;	//Check if calling DLL or static function
					string 
						fnname, 	//Get function name to call
						reg_xx; 	//Get register for storing parameters
						
					string param;
					unsigned int 
						largc = -1,  	//Get parameter count
						sqbuf = 0,  	//Check if inside nester square brackets
						sqcount = 0;	//Get number of square brackets
					
					dllcall = (linetmp[0] == '$');
					fnname = linetmp.substr(1, linetmp.find('[') - 1);
					linetmp = nextword(linetmp, fnname.size() + 2);
					
					//Set function name with braces if calling dll function
					if (dllcall) fnname = '[' + fnname + ']';
					
					//Check if there are parameters
					if (linetmp[0] == ']')
						linetmp = linetmp.substr(1, linetmp.size() - 1);
					
					//Get parameters
					else for (
						unsigned int i = 0;
						i < linetmp.size();
						++i
					) {
						//if (i == findnoqs(s, s[i], (s[i] == ']') ? sqcount : 0))
						switch (linetmp[i]) {
							case ',': 
								if (not sqbuf) {
									++largc; 
									param = linetmp.substr(0, findnoqs(linetmp, ',', sqcount)); 
									linetmp = linetmp.substr(param.size() + 1, linetmp.size() - param.size() - 1);
								}
								break;
							case ']': 
								if (not sqbuf) {
									++largc; 
									param = linetmp.substr(0, findnoqs(linetmp, ']', sqcount)); 
									linetmp = linetmp.substr(param.size() + 1, linetmp.size() - param.size() - 1);
								} 
								else --sqbuf; 
								break;
							case '[': ++sqbuf; ++sqcount; break;
						}
						
						//std::cout << "COUNTER:	" << i << "	:	" << linetmp[i] << "	||	";
						//std::cout << "FINDERS:	" << sqbuf << "	:	" << findnoqs(linetmp, ']', sqcount) << "\n";
						//std::cout << "RESULTS:	'" << linetmp << "'\n";
						
						if (param.size() or i == linetmp.size() - 1) {
							std::cout << "PARAM:	'" << param << "' -> '" << linetmp << "'\n";
							
							outtext << std::endl;
							lexereq(outtext, cdata, nextword(param, 0), scope);
							
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
								
								outtext << "mov " << reg_xx << ", rax" << std::endl;
							}
							else outtext << "push rax" << std::endl;
							
							//Check if end of function
							if (not linetmp.size()) {
								std::cout << "EJECT 1\n";
								break;
							}
							i = 0;
						}
						
						//Reset parameter
						param = "";
					}
					
					//Call the function
					outtext << "call " << fnname << endl;
				}
				
				//Arithmetic parsing
				if (scope != "G_") {
					string equ = lexer(nextword(linetmp.substr(lexer(linetmp).size(), linetmp.size() - lexer(linetmp).size()), 0));
					if (findnoq(equ, '=') < equ.size()) {
						string 
							lvaluei,			//Get left variable of operation (with scope prefix)
							lvalue,				//Get left variable of operation
							reg_ax;				//ax register size, get size of lvalue
						
						lvalue = lexer(linetmp);
						linetmp = nextword(nextword(linetmp, lvalue.size()), equ.size());
						
						bool isglobal = 0;			//Determine if lvalue is global or stack variable
						
						cout << "LEXER:	" << lvalue << endl;
						
						//Check if lvalue is global or stack variable
						if (cdata.d8m.count("G_" + lvalue)
						or cdata.d16m.count("G_" + lvalue)
						or cdata.d32m.count("G_" + lvalue)
						or cdata.d64m.count("G_" + lvalue))
							isglobal = 1;

						else if (
						not (cdata.d8m.count(scope + lvalue)
						or cdata.d16m.count(scope + lvalue)
						or cdata.d32m.count(scope + lvalue)
						or cdata.d64m.count(scope + lvalue))) {
							cerr << "Error: Variable '" << lvalue << "' not found." << endl;
							return 1;
						}
						
						//Check size of variable
						if (isglobal) {
							lvaluei = "G_" + lvalue;
							lvalue = lvaluei;
						}
						else lvaluei = scope + lvalue;
						
						if (cdata.d8m.count(lvaluei)) 	reg_ax = "al";
						if (cdata.d16m.count(lvaluei))	reg_ax = "ax";
						if (cdata.d32m.count(lvaluei))	reg_ax = "eax";
						if (cdata.d64m.count(lvaluei))	reg_ax = "rax";
						
						
						//Parse the equation
						outtext << endl;
						if (not lexereq(outtext, cdata, linetmp, scope)) return 1;
						cout << "ENDLEXEQ" << endl;
						
						//Regular operators
						if (equ == "=") 	outtext << "mov " << "[" << lvalue << "], " << reg_ax << endl; else
						if (equ == "+=")	outtext << "add " << "[" << lvalue << "], " << reg_ax << endl; else
						if (equ == "-=")	outtext << "sub " << "[" << lvalue << "], " << reg_ax << endl; else
						if (equ == "*=") {
							outtext << "mov " << "rbx, [" << lvalue << "]" << endl;
							outtext << "mul " << reg_ax << endl;
							outtext << "mov " << "[" << lvalue << "], rbx" << endl;
						} else
						if (equ == "/=") {
							outtext << "xor rdx, rdx" << endl;
							outtext << "mov " << "rbx, [" << lvalue << "]" << endl;
							outtext << "div " << reg_ax << endl;
							outtext << "mov " << "[" << lvalue << "], rbx" << endl;
						}
						
						//Bitwise operators
						if (equ == "&=")	outtext << "and " << "[" << lvalue << "], " << reg_ax << endl; else
						if (equ == "|=")	outtext << "or " << "[" << lvalue << "], " << reg_ax << endl; else
						if (equ == "~=")	outtext << "not " << "[" << lvalue << "], " << reg_ax << endl; else
						if (equ == "%=")	outtext << "xor " << "[" << lvalue << "], " << reg_ax << endl;
						
						//Reference operators
						/*
							if (equ == "@=")	outtext << "lea " << "[" << lvalue << "], [" << reg_ax << "]" << endl; else
							if (equ == "$=")	outtext << "mov " << "[" << lvalue << "], [" << reg_ax << "]" << endl;
						*/
						else if (equ == "@=" or equ == "$=") {
							cerr << "Error: Variable '" << lvalue << "' is not a pointer." << endl;
							return 1;
						}
						
						continue;
					}
				}
			
				//Reserved keywords
				if (lexer(linetmp) == "repeat") {
					outtext << ".L" << jumpn++ << ':' << endl;
				}
				
				else if (lexer(linetmp) == "lang") {
					linetmp = nextword(linetmp, 4);
					
					if (lexer(linetmp) == "asm") {
						flag_asm = 1;
						
						cout << "L:	" << linetmp << endl;
						linetmp = nextword(linetmp, linetmp.find(':') + 1);
						cout << "L:	" << linetmp << endl;
						
						//Check if end of flag_asm
						if (findnoq(linetmp, ';') < linetmp.size()) {
							linetmp = linetmp.substr(0, findnoq(linetmp, ';'));
							flag_asm = 0;
						}
						
						//Check appropriate section to output
						if (scope == "G_") {
							//Output data header early
							if (datadecl) 
							#ifdef _WIN32
								outdata << "section '.data' data readable writeable" << endl;
							#endif
							#ifdef __linux__
								outdata << "segment writeable" << endl;
							#endif
							
							outdata << linetmp << endl;
							datadecl = 0;
						}
						else outtext << linetmp << endl;
					}
				}
				else if (lexer(linetmp) == "ret") {
					if (findnoq(linetmp, ';') < linetmp.size())
						linetmp = linetmp.substr(4, findnoq(linetmp, ';'));
					if (linetmp.size())
						lexereq(outtext, cdata, linetmp, scope);
					
					outtext << "ret \n" << endl;
				}
				else if (lexer(linetmp) == "endproc") {
					if (stacklv) {
						outtext << endl;
						outtext << "add rsp, " << stacklv << endl;
						outtext << "pop rbp" << endl;
					}
					#ifdef _WIN32
					outtext << "mov rcx, 0" << endl;
					outtext << "call [ExitProcess]" << endl;
					#endif
					#ifdef __linux__
					outtext << "mov rax, 60" << endl;
					outtext << "syscall" << endl;
					#endif
				}
			}
		}
		
		
		if (datadecl) 
		#ifdef _WIN32
			outdata << "section '.data' data readable writeable" << endl;
		#endif
		#ifdef __linux__
			outdata << "segment writeable" << endl;
		#endif
		
		//Put variables to file
		for (auto it = cdata.d8m.begin(); it != cdata.d8m.end(); advance(it, 1)) { 
			//If reserved variable
			if (it->second.substr(0, 3) == "res")
				outdata << it->first << "\trb " << nextword(it->second, 3) << endl;
			else if (it->second[0] == '"') {
				outdata << it->first << "\tdb " << it->second << endl;
				cout << it->first << it->second << endl;
			}
			else outdata << it->first << "\tdb " << it->second << endl;
		}
		
		for (auto it = cdata.d16m.begin(); it != cdata.d16m.end(); advance(it, 1)) { 
			//If reserved variable
			if (it->second.substr(0, 3) == "res")
				outdata << it->first << "\trw " << nextword(it->second, 3) << endl;
			else outdata << it->first << "\tdw " << it->second << endl;
		}
		
		for (auto it = cdata.d32m.begin(); it != cdata.d32m.end(); advance(it, 1)) { 
			//If reserved variable
			if (it->second.substr(0, 3) == "res")
				outdata << it->first << "\trd " << nextword(it->second, 3) << endl;
			else outdata << it->first << "\tdd " << it->second << endl;
		}
		
		for (auto it = cdata.d64m.begin(); it != cdata.d64m.end(); advance(it, 1)) {
			//If reserved variable
			if (it->second.substr(0, 3) == "res")
				outdata << it->first << "\trq " << nextword(it->second, 3) << endl;
			else outdata << it->first << "\tdq " << it->second << endl;
		}
		
		string outfstr = argv[1];

		outfstr = outfstr.substr(0, outfstr.size() - (revstr(outfstr).find('.') + 1));
		
		cout << outfstr << '\t' << outfstr.size() - (revstr(outfstr).find('.') + 1) << endl;
		output.open(outfstr + ".asm");
		
		//Put buffer to output file
		output << outdata.rdbuf() << endl;
		output << outtext.rdbuf() << endl;
		
		output.close();
		
		cout << "Compilation end." << endl;
		
		#ifdef _WIN32
		system(((string) ("fasm " + outfstr + ".asm")).c_str());
		#endif
		#ifdef __linux__
		system(((string) ("fasm.exe " + outfstr + ".asm")).c_str());
		#endif
		
	}
	else {
		cerr << "No input detected." << endl;
	}
	
	return 0;
}
