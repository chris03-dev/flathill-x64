/*
	Copyright (c) 2020, chris03-dev
	
	The following source code is licensed under the UPL-v1.0.
	Please read LICENSE.md for details.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
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

int main(int argc, char **argv) {
	if (argc > 1) {
		cout << "Input detected." << endl;
		
		vector<string> 
			fstr,					//List of file lines
			fnstr;					//Temporary list for function
		unsigned int flinenum = 0;	//Line number
		
		ifstream input;				//Input file
		stringstream
			outdata,
			outtext;
		ofstream 
			datatmp,				//Used only to create .data section file
			texttmp,				//Used only to create .text section file
			output;
		
		//Open and read input file
		//fstr.reserve(2048);
		//fnstr.reserve(512);
		
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
		outdata << "format PE64 GUI" << endl;
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
			stacklv = 0,  			//Bytes pushed in stack
			scopelv = 0;  			//Check if number of scopes in stack
		
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
				//Global
				if (scope == "G_") {
					//Global variable declaration
					string varname;		//Get name of variable, also used for removing tokens
					
					if (linetmp.substr(0, 2) == "d8") {
						datadecl = 1;
						vartype = FH_BYTE;
						varname = lexer(nextword(linetmp, 2));
						cout << "d8 " << varname << "\n";
						
						cdata.d8m.insert(pair<string, string>(scope + varname, (string) to_string(stacklv)));
					}
					else if (linetmp.substr(0, 3) == "d16") {
						datadecl = 1;
						vartype = FH_WORD;
						varname = lexer(nextword(linetmp, 3));
						cout << "d16 " << varname << "\n";
						
						cdata.d16m.insert(pair<string, string>(scope + varname, (string) to_string(stacklv)));
					}
					else if (linetmp.substr(0, 3) == "d32") {
						datadecl = 1;
						vartype = FH_DWORD;
						varname = lexer(nextword(linetmp, 3));
						cout << "d32 " << varname << "\n";
						
						cdata.d32m.insert(pair<string, string>(scope + varname, (string) to_string(stacklv)));
					}
					else if (linetmp.substr(0, 3) == "d64") {
						datadecl = 1;
						vartype = FH_QWORD;
						varname = lexer(nextword(linetmp, 3));
						cout << "d64 " << varname << "\n";
						
						cdata.d64m.insert(pair<string, string>(scope + varname, (string) to_string(stacklv)));
					}
					
					//Global pointer declaration
					if (line.size() >= 6) {
						if (linetmp.substr(0, 4) == "ptr8") {
							datadecl = 1;
							vartype = FH_PTR_BYTE;
							varname = lexer(nextword(linetmp, 4));
							
							cdata.ptr8m.insert(pair<string, string>(scope + varname, (string) to_string(stacklv)));
						}
						else if (linetmp.substr(0, 5) == "ptr16") {
							datadecl = 1;
							vartype = FH_PTR_WORD;
							varname = lexer(nextword(linetmp, 5));
							
							cdata.ptr16m.insert(pair<string, string>(scope + varname, (string) to_string(stacklv)));
						}
						else if (linetmp.substr(0, 5) == "ptr32") {
							datadecl = 1;
							vartype = FH_PTR_DWORD;
							varname = lexer(nextword(linetmp, 5));
							
							cdata.ptr32m.insert(pair<string, string>(scope + varname, (string) to_string(stacklv)));
						}
						else if (linetmp.substr(0, 5) == "ptr64") {
							datadecl = 1;
							vartype = FH_PTR_QWORD;
							varname = lexer(nextword(linetmp, 5));
							cout << "ptr64 " << varname << "\n";
							
							cdata.ptr64m.insert(pair<string, string>(scope + varname, (string) to_string(stacklv)));
						}
					}
					
					//Remove datatype
					if (vartype != 0) {
						switch (vartype) {
							case FH_BYTE:      linetmp = nextword(linetmp, 2); break;
							case FH_WORD:
							case FH_DWORD:
							case FH_QWORD:     linetmp = nextword(linetmp, 3); break;
							case FH_PTR_BYTE:  linetmp = nextword(linetmp, 4); break;
							case FH_PTR_WORD:
							case FH_PTR_DWORD:
							case FH_PTR_QWORD: linetmp = nextword(linetmp, 5); break;
						}
						//linetmp = nextword(linetmp.substr(vartype.size(), linetmp.size() - vartype.size()), 0);
						
						linetmp = nextword(linetmp.substr(varname.size(), linetmp.size() - varname.size()), 0);
						cout << "HEY:	'" << linetmp << "'\n";
						
						//Check if value
						string equ = lexer(linetmp);
						if (equ.size() and equ == "=") {
							linetmp = nextword(linetmp, equ.size());
							switch (vartype) {
								case FH_BYTE:      cdata.d8m.insert(pair<string, string>(scope + varname, "10"));    break;
								case FH_WORD:      cdata.d16m.insert(pair<string, string>(scope + varname, "10"));   break;
								case FH_DWORD:     cdata.d32m.insert(pair<string, string>(scope + varname, "10"));   break;
								case FH_QWORD:     cdata.d64m.insert(pair<string, string>(scope + varname, "10"));   break;
								case FH_PTR_BYTE:  cdata.ptr8m.insert(pair<string, string>(scope + varname, "10"));  break;
								case FH_PTR_WORD:  cdata.ptr16m.insert(pair<string, string>(scope + varname, "10")); break;
								case FH_PTR_DWORD: cdata.ptr32m.insert(pair<string, string>(scope + varname, "10")); break;
								case FH_PTR_QWORD: cdata.ptr64m.insert(pair<string, string>(scope + varname, "10")); break;
							}
						}
						else {
							switch (vartype) {
								case FH_BYTE:      cdata.d8m.insert(pair<string, string>(scope + varname, "res"));    break;
								case FH_WORD:      cdata.d16m.insert(pair<string, string>(scope + varname, "res"));   break;
								case FH_DWORD:     cdata.d32m.insert(pair<string, string>(scope + varname, "res"));   break;
								case FH_QWORD:     cdata.d64m.insert(pair<string, string>(scope + varname, "res"));   break;
								case FH_PTR_BYTE:  cdata.ptr8m.insert(pair<string, string>(scope + varname, "res"));  break;
								case FH_PTR_WORD:  cdata.ptr16m.insert(pair<string, string>(scope + varname, "res")); break;
								case FH_PTR_DWORD: cdata.ptr32m.insert(pair<string, string>(scope + varname, "res")); break;
								case FH_PTR_QWORD: cdata.ptr64m.insert(pair<string, string>(scope + varname, "res")); break;
							}
						}
					}
				}
				
				//Check if line is object declaration
				//Not yet implemented.
				
				//Check if line is function declaration
				if (linetmp.substr(0, 2) == "fn") {
					//Used to find all stack variables
					outtext << lexer(nextword(linetmp, 2)) << ":" << endl;
					
					cout << "Scope: 	F_" << lexer(nextword(linetmp, 2)) << "_@N" << endl;
					scope = "F_" + lexer(nextword(linetmp, 2)) + "_@N";
					
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
						
						//cout << fnline << endl;
						
						//Stack variable declaration
						if (fnline.substr(0, 2) == "d8") {
							varname = scope + lexer(nextword(fnline, 2));
							
							stacklv++;
							cdata.d8m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
						}
						else if (fnline.substr(0, 3) == "d16") {
							varname = scope + lexer(nextword(fnline, 3));
							
							stacklv += 2;
							cdata.d16m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
						}
						else if (fnline.substr(0, 3) == "d32") {
							varname = scope + lexer(nextword(fnline, 3));
							
							stacklv += 4;
							cdata.d32m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
						}
						else if (fnline.substr(0, 3) == "d64") {
							varname = scope + lexer(nextword(fnline, 3));
							
							stacklv += 8;
							cdata.d64m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
						}
						
						//Pointer declaration
						if (line.size() >= 6) {
							if (fnline.substr(0, 4) == "ptr8") {
								varname = scope + lexer(nextword(fnline, 4));
								
								stacklv += 8;
								cdata.ptr8m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
							}
							else if (fnline.substr(0, 5) == "ptr16") {
								varname = scope + lexer(nextword(fnline, 5));
								
								stacklv += 8;
								cdata.ptr16m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
							}
							else if (fnline.substr(0, 5) == "ptr32") {
								varname = scope + lexer(nextword(fnline, 5));
								
								stacklv += 8;
								cdata.ptr32m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
							}
							else if (fnline.substr(0, 5) == "ptr64") {
								varname = scope + lexer(nextword(fnline, 5));
								
								stacklv += 8;
								cdata.ptr64m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
							}
							
							//Decrement scope
							while (findnoq(fnline, ';') < fnline.size()) {
								scopelvtmp--;
								fnline = fnline.substr(findnoq(fnline, ';') + 1, fnline.size() - findnoq(fnline, ';') - 1);
							}
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
					cout << "LINEDCL:	'" << linetmp << "'"<<endl;
				}
				
				//Check non-return function call
				if (linetmp[0] == '$') {
					string 
						fnname, 	//Get function name to call
						reg_xx; 	//Get register for storing parameters
						
					string param;
					unsigned int 
						largc = -1,  	//Get parameter count
						sqbuf = 0,  	//Check if inside nester square brackets
						sqcount = 0;	//Get number of square brackets
					
					fnname = linetmp.substr(1, linetmp.find('[') - 1);
					linetmp = nextword(linetmp, fnname.size() + 2);
					
					//Get parameters
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
						or cdata.d64m.count("G_" + lvalue)
						or cdata.ptr8m.count("G_" + lvalue) 
						or cdata.ptr16m.count("G_" + lvalue) 
						or cdata.ptr32m.count("G_" + lvalue)
						or cdata.ptr64m.count("G_" + lvalue))
							isglobal = 1;

						else if (
						not (cdata.d8m.count(scope + lvalue)
						or cdata.d16m.count(scope + lvalue)
						or cdata.d32m.count(scope + lvalue)
						or cdata.d64m.count(scope + lvalue)
						or cdata.ptr8m.count(scope + lvalue) 
						or cdata.ptr16m.count(scope + lvalue) 
						or cdata.ptr32m.count(scope + lvalue)
						or cdata.ptr64m.count(scope + lvalue))) {
							cerr << "Error: Variable '" << lvalue << "' not found." << endl;
							return 1;
						}
						
						//Check size of variable
						if (isglobal)	lvalue = "G_" + lvalue;
						else         	lvalue = scope + lvalue;
						
						if (cdata.d8m.count(lvalue)) 	reg_ax = "al";
						if (cdata.d16m.count(lvalue))	reg_ax = "ax";
						if (cdata.d32m.count(lvalue))	reg_ax = "eax";
						if (cdata.d64m.count(lvalue) or
							cdata.ptr8m.count(lvalue) or 
							cdata.ptr16m.count(lvalue) or
							cdata.ptr32m.count(lvalue) or
							cdata.ptr64m.count(lvalue))	
							reg_ax = "rax";
						
						
						//Parse the equation
						outtext << endl;
						lexereq(outtext, cdata, linetmp, scope);
						
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
						if (cdata.ptr8m.count(lvalue)
						or cdata.ptr16m.count(lvalue)
						or cdata.ptr32m.count(lvalue)
						or cdata.ptr64m.count(lvalue)) {
							if (equ == "@=")	outtext << "lea " << "[" << lvalue << "], [" << reg_ax << "]" << endl; else
							if (equ == "$=")	outtext << "mov " << "[" << lvalue << "], [" << reg_ax << "]" << endl;
						}
						else if (equ == "@=" or equ == "$=") {
							cerr << "Error: Variable '" << lvalue << "' is not a pointer." << endl;
							return 1;
						}
						
						continue;
					}
				}
			
				//Reserved keywords
				if (lexer(linetmp) == "lang") {
					linetmp = nextword(linetmp, 4);
					
					if (lexer(linetmp) == "asm") flag_asm = 1;
				}
				else if (lexer(linetmp) == "ret") {
					if (findnoq(linetmp, ';') < linetmp.size())
						linetmp = linetmp.substr(4, findnoq(linetmp, ';'));
					if (linetmp.size())
						lexereq(outtext, cdata, linetmp, scope);
					
					outtext << "ret " << endl;
				}
				else if (linetmp.substr(0, 7) == "endproc") {
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
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdb 0" << endl;
		}
		
		for (auto it = cdata.d16m.begin(); it != cdata.d16m.end(); advance(it, 1)) { 
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdw 0" << endl;
		}
		
		for (auto it = cdata.d32m.begin(); it != cdata.d32m.end(); advance(it, 1)) { 
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdd 0" << endl;
		}
		
		for (auto it = cdata.d64m.begin(); it != cdata.d64m.end(); advance(it, 1)) { 
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdq 0" << endl;
		}
		
		for (auto it = cdata.ptr8m.begin(); it != cdata.ptr8m.end(); advance(it, 1)) { 
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdb 0" << endl;
		}
		
		for (auto it = cdata.ptr16m.begin(); it != cdata.ptr16m.end(); advance(it, 1)) { 
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdw 0" << endl;
		}
		
		for (auto it = cdata.ptr32m.begin(); it != cdata.ptr32m.end(); advance(it, 1)) { 
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdd 0" << endl;
		}
		
		for (auto it = cdata.ptr64m.begin(); it != cdata.ptr64m.end(); advance(it, 1)) { 
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdq 0" << endl;
		}
		
		string outfstr = ((string) argv[1]).substr(0, ((string) argv[1]).find('.'));
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
