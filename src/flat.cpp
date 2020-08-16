/*
	Copyright (c) 2020, chris03-dev
	
	The following source code is licensed under the UPL-v1.0.
	Please read LICENSE.md for details.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <map>
#include "linemod.h"

using namespace std;

//Compiler data
compilerdata cdata;

int main(int argc, char **argv) {
	if (argc > 1) {
		cout << "Input detected." << endl;
		
		list<string> 
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
		cout << "Opening '" << argv[1] << "'..." << endl;
		input.open(argv[1]);
		{
			string s;
			while (getline(input, s))
				fstr.push_back(removecom(s));
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
        outdata << "import msvcrt, printf, 'printf'" << endl;
		
		outtext << "section '.text' code readable executable" << endl;
		#endif
		
		#ifdef __linux__
		outdata << "format ELF64 executable" << endl;
		outdata << "entry _start" << endl;
		
		outtext << "segment executable" << endl;
		#endif
		
		//Read every line of input file
		string
			scope = "G_",			//Scope of variables
			line, linetmp;			//Line of input file
			
		unsigned int 
			stacklv = 0,			//Bytes pushed in stack
			scopelv = 0;			//Check if number of scopes in stack
		
		bool datadecl = 0;		//Check if section '.data' should be inserted
		
		for (auto it = fstr.begin(); it != fstr.end(); advance(it, 1), flinenum++) { 
			line = nextword(*it, 0);
			linetmp = line;
			
			//Write line details on console
			cout << flinenum << "	(" << scopelv << "):	" << line << "		" << endl;	
			
			//Scope increment and declaration
			if (findnoq(linetmp, ':') < linetmp.size()) {
				scopelv++;
				
				scope = scope.substr(0, findnoq(scope, "_@N") + 3);
				cout << "Inc scope." << endl;
			}
			
			//Global variable declaration
			if (scope == "G_") {
				if (linetmp.substr(0, 2) == "d8") {
					datadecl = 1;
					string varname = scope + lexer(nextword(line, 2));
					cout << "d8 " << varname << "\n";
					
					cdata.d8m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				if (linetmp.substr(0, 3) == "d16") {
					datadecl = 1;
					string varname = scope + lexer(nextword(line, 3));
					cout << "d16 " << varname << "\n";
					
					cdata.d16m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				if (linetmp.substr(0, 3) == "d32") {
					datadecl = 1;
					string varname = scope + lexer(nextword(line, 3));
					cout << "d32 " << varname << "\n";
					
					cdata.d32m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				if (linetmp.substr(0, 3) == "d64") {
					datadecl = 1;
					string varname = scope + lexer(nextword(line, 3));
					cout << "d64 " << varname << "\n";
					
					cdata.d64m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				
				if (linetmp.substr(0, 4) == "ptr8") {
					datadecl = 1;
					string varname = scope + lexer(nextword(line, 4));
					cout << "ptr8 " << varname << "\n";
					
					cdata.ptr8m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				if (linetmp.substr(0, 5) == "ptr16") {
					datadecl = 1;
					string varname = scope + lexer(nextword(line, 5));
					cout << "ptr16 " << varname << "\n";
					
					cdata.ptr16m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				if (linetmp.substr(0, 5) == "ptr32") {
					datadecl = 1;
					string varname = scope + lexer(nextword(line, 5));
					cout << "ptr32 " << varname << "\n";
					
					cdata.ptr32m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				if (linetmp.substr(0, 5) == "ptr64") {
					datadecl = 1;
					string varname = scope + lexer(nextword(line, 5));
					cout << "ptr64 " << varname << "\n";
					
					cdata.ptr64m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
			}
			
			//Check if line is object declaration
			//Code is in dump.txt. DO NOT REMOVE.
			
			//Check if line is function declaration
			if (linetmp.substr(0, 2) == "fn") {
				//Used to find all stack variables
				outtext << lexer(nextword(linetmp, 2)) << ":" << endl;
				
				cout << "Scope: 	F_" << lexer(nextword(linetmp, 2)) << "_@N" << endl;
				scope = "F_" + lexer(nextword(linetmp, 2)) + "_@N";
				
				if (lexer(nextword(linetmp, 2)) == "_start")
					outtext << "xor rdx, rdx" << endl;
				
				//Set to default
				stacklv = 0;
				int scopelvtmp = 1;
				
				auto itfn = next(fstr.begin(), flinenum + 1);
				
				//Check end of function
				while (scopelvtmp > 0) {
					string 
						fnline = nextword(*itfn, 0),
						varname;
					
					//Stack variable declaration
					if (fnline.substr(0, 2) == "d8") {
						varname = scope + lexer(nextword(fnline, 2));
						
						stacklv++;
						cdata.d8m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
					}
					if (fnline.substr(0, 3) == "d16") {
						varname = scope + lexer(nextword(fnline, 3));
						
						stacklv += 2;
						cdata.d16m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
					}
					if (fnline.substr(0, 3) == "d32") {
						varname = scope + lexer(nextword(fnline, 3));
						
						stacklv += 4;
						cdata.d32m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
					}
					if (fnline.substr(0, 3) == "d64") {
						varname = scope + lexer(nextword(fnline, 3));
						
						stacklv += 8;
						cdata.d64m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
					}
					
					//Pointer declaration
					if (fnline.substr(0, 4) == "ptr8") {
						varname = scope + lexer(nextword(fnline, 4));
						
						stacklv += 8;
						cdata.ptr8m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
					}
					if (fnline.substr(0, 5) == "ptr16") {
						varname = scope + lexer(nextword(fnline, 5));
						
						stacklv += 8;
						cdata.ptr16m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
					}
					if (fnline.substr(0, 5) == "ptr32") {
						varname = scope + lexer(nextword(fnline, 5));
						
						stacklv += 8;
						cdata.ptr32m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
					}
					if (fnline.substr(0, 5) == "ptr64") {
						varname = scope + lexer(nextword(fnline, 5));
						
						stacklv += 8;
						cdata.ptr64m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
					}
					
					if (findnoq(fnline, ':') < fnline.size()) scopelvtmp++;
					if (findnoq(fnline, ';') < fnline.size()) scopelvtmp--;
					
					advance(itfn, 1);
				}
				
				if (stacklv) {
					outtext << "push rbp" << endl;
					outtext << "mov rbp, rsp" << endl;
					outtext << "sub rsp, " << stacklv << endl;
				}
			}
			
			//Check non-return function call
			if (linetmp[0] == '$') {
				linetmp = linetmp.substr(1, linetmp.size() - 1);
				
				string 
					fnname, 	//Get function name to call
					reg_xx; 	//Get register for storing parameters
					
				string param;
				unsigned int largc = 0;	//Get parameter count
				
				fnname = linetmp.substr(0, linetmp.find('['));
				linetmp = nextword(linetmp, fnname.size() + 1);
				
				//Get parameters
				for (param = 
				    (findnoq(linetmp, ',') < findnoq(linetmp, ']')) ?
				    	linetmp.substr(0, findnoq(linetmp, ',')) :
				    	linetmp.substr(0, findnoq(linetmp, ']'));
				     param != "";
					 ++largc,
				     linetmp = nextword(linetmp, param.size() + 1),					 
				     param = (findnoq(linetmp, ',') < findnoq(linetmp, ']')) ?
				     	linetmp.substr(0, findnoq(linetmp, ',')) :
				     	linetmp.substr(0, findnoq(linetmp, ']'))
				) {
					lexereq(outtext, cdata, param, scope);
					
					//Set register based on parameter
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
						default:
							outtext << "push rax" << endl;
							break;
					}
						
					outtext << "mov " << reg_xx << ", rax" << endl;
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
					outtext << "\nxor rax, rax" << endl;
					lexereq(outtext, cdata, linetmp, scope);
					
					//Regular operators
					if (equ == "=") 	outtext << "mov " << "[" << lvalue << "], " << reg_ax << endl;
					if (equ == "+=")	outtext << "add " << "[" << lvalue << "], " << reg_ax << endl;
					if (equ == "-=")	outtext << "sub " << "[" << lvalue << "], " << reg_ax << endl;
					if (equ == "*=") {
						outtext << "mov " << "rbx, [" << lvalue << "]" << endl;
						outtext << "mul " << reg_ax << endl;
						outtext << "mov " << "[" << lvalue << "], rbx" << endl;
					}
					if (equ == "/=") {
						outtext << "xor rdx, rdx" << endl;
						outtext << "mov " << "rbx, [" << lvalue << "]" << endl;
						outtext << "div " << reg_ax << endl;
						outtext << "mov " << "[" << lvalue << "], rbx" << endl;
					}
					
					//Bitwise operators
					if (equ == "&=")	outtext << "and " << "[" << lvalue << "], " << reg_ax << endl;
					if (equ == "|=")	outtext << "or " << "[" << lvalue << "], " << reg_ax << endl;
					if (equ == "~=")	outtext << "not " << "[" << lvalue << "], " << reg_ax << endl;
					if (equ == "%=")	outtext << "xor " << "[" << lvalue << "], " << reg_ax << endl;
					
					//Reference operators
					if (cdata.ptr8m.count(lvalue)
					or cdata.ptr16m.count(lvalue)
					or cdata.ptr32m.count(lvalue)
					or cdata.ptr64m.count(lvalue)) {
						if (equ == "@=")	outtext << "lea " << "[" << lvalue << "], [" << reg_ax << "]" << endl;
						if (equ == "$=")	outtext << "mov " << "[" << lvalue << "], [" << reg_ax << "]" << endl;
					}
					else {
						if (equ == "@=" or equ == "$=")	{
							cerr << "Error: Variable '" << lvalue << "' is not a pointer." << endl;
							return 1;
						}
					}
					
					continue;
				}
			}
			
			//Method keywords
			
			//Reserved keywords
			if (linetmp.substr(0, 3) == "ret") {
				linetmp = linetmp.substr(4, findnoq(linetmp, ';'));
				if (linetmp.size() > 0) lexereq(outtext, cdata, linetmp, scope);
				
				if (stacklv) {
					outtext << endl;
					outtext << "add rsp, " << stacklv << endl;
					outtext << "pop rbp" << endl;
					
					stacklv = 0;
				}
				
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
			
			//Function end, scope decrement
			if (findnoq(linetmp, ";") < linetmp.size()) {
				scopelv--;
				if (scopelv < 1) {
					scope = "G_";
					cout << "Dec scope." << endl;
					
					if (stacklv and not (linetmp.find("endproc") < linetmp.size())) {
						outtext << endl;
						outtext << "add rsp, " << stacklv << endl;
						outtext << "pop rbp" << endl;
						
						stacklv = 0;
					}
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
			cout << "PTR8:	" << it->first << endl;
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdb 0" << endl;
		}
		
		for (auto it = cdata.ptr16m.begin(); it != cdata.ptr16m.end(); advance(it, 1)) { 
			cout << "PTR16:	" << it->first << endl;
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdw 0" << endl;
		}
		
		for (auto it = cdata.ptr32m.begin(); it != cdata.ptr32m.end(); advance(it, 1)) { 
			cout << "PTR32:	" << it->first << endl;
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdd 0" << endl;
		}
		
		for (auto it = cdata.ptr64m.begin(); it != cdata.ptr64m.end(); advance(it, 1)) { 
			cout << "PTR64:	" << it->first << endl;
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
		
		system(((string) ("fasm " + outfstr + ".asm")).c_str());
		
	}
	else {
		cerr << "No input detected." << endl;
	}
	
	return 0;
}
