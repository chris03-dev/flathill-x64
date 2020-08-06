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
		
		//Local and global variables
		outdata << "section '.data' data readable writeable" << endl;
		outdata << "C_byte db 0" << endl;
		outdata << "C_word dw 0" << endl;
		outdata << "C_dword dd 0" << endl;
		outdata << "C_qword dq 0" << endl;
		
		outtext << "section '.text' code readable executable" << endl;
		#endif
		
		#ifdef __linux__
		outdata << "format ELF64 executable" << endl;
		outdata << "entry _start" << endl;
		outdata << "segment writeable" << endl;
		
		outtext << "segment executable" << endl;
		#endif
		
		//Read every line of input file
		string
			scope = "G_",			//Scope of variables
			line, linetmp;			//Line of input file
			
		unsigned int 
			stacklv = 0,			//Bytes pushed in stack
			scopelv = 0;			//Number of scopes in stack
		
		for (auto it = fstr.begin(); it != fstr.end(); advance(it, 1), flinenum++) { 
			line = nextword(*it, 0);
			linetmp = line;
			cout << flinenum << "	(" << scopelv << "):	" << line << "		" << endl;	
			
			//Scope increment and declaration
			if (linetmp.find(':') < linetmp.size()) {
				scopelv++;
				scope = scope.substr(0, scope.find("_S") + 2) + to_string(scopelv) + "_@N";
				cout << "Inc scope." << endl;
			}
			
			//Global variable declaration
			if (scope == "G_") {
				if (linetmp.substr(0, 2) == "d8") {
					string varname = scope + lexer(nextword(line, 2));
					
					cdata.d8m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				if (linetmp.substr(0, 3) == "d16") {
					string varname = scope + lexer(nextword(line, 3));
					cout << "d16 " << varname << "\n";
					
					cdata.d16m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				if (linetmp.substr(0, 3) == "d32") {
					string varname = scope + lexer(nextword(line, 3));
					cout << "d32 " << varname << "\n";
					
					cdata.d32m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
				if (linetmp.substr(0, 3) == "d64") {
					string varname = scope + lexer(nextword(line, 3));
					cout << "d64 " << varname << "\n";
					
					cdata.d64m.insert(pair<string, string>(varname, (string) to_string(stacklv)));
				}
			}
			
			//Check if line is object declaration
			//Code is in dump.txt. DO NOT REMOVE.
			
			//Check if line is function declaration
			if (linetmp.substr(0, 2) == "fn") {
				//Used to find all stack variables
				outtext << lexer(nextword(linetmp, 2)) << ":" << endl;
				
				cout << "Scopelv:	" << scopelv << endl;
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
						fnline = nextword(*itfn, 0);
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
					
					if (fnline.find(':') < fnline.size()) {
						cout << "COL\n";
						scopelvtmp++;
					}
					if (fnline.find(';') < fnline.size()) {
						cout << "SCOLN\n";
						scopelvtmp--;
					}
					
					advance(itfn, 1);
				}
				
				if (stacklv) {
					outtext << "push rbp" << endl;
					outtext << "mov rbp, rsp" << endl;
					outtext << "sub rsp, " << stacklv << endl;
				}
			}
			
			//Arithmetic parsing
			string equ = lexer(nextword(linetmp.substr(lexer(linetmp).size(), linetmp.size() - lexer(linetmp).size()), 0));
			if (equ.find('=') < equ.size()) {
				string 
					rscope,
					lvalue,				//Get left variable of operation
					reg_ax;				//ax register size, get size of lvalue
				
				rscope = scope;
				lvalue = lexer(linetmp);
				linetmp = nextword(nextword(linetmp, lvalue.size()), equ.size());
				
				bool isglobal = 0;			//Determine if lvalue is global or stack variable
				
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
				if (isglobal)	lvalue = "G_" + lvalue;
				else         	lvalue = scope + lvalue;
				
				if (cdata.d8m.count(lvalue)) 	reg_ax = "al";
				if (cdata.d16m.count(lvalue))	reg_ax = "ax";
				if (cdata.d32m.count(lvalue))	reg_ax = "eax";
				if (cdata.d64m.count(lvalue))	reg_ax = "rax";
				
				//Parse the equation
				lexereq(outtext, cdata, linetmp, scope);
				
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
				
				continue;
			}
			
			//Reserved keywords
			
			//Function keywords
			
			//Method keywords
			
			//Function end, scope decrement
			if (linetmp.find(";") < linetmp.size()) {
				scopelv--;
				if (scopelv < 1) {
					scope = "G_";
					cout << "Dec scope." << endl;
					
					if (stacklv) {
						outtext << endl;
						outtext << "add rsp, " << stacklv << endl;
						outtext << "pop rbp" << endl;
						
						stacklv = 0;
					}
					
					if (linetmp.substr(0, 3) == "ret") {
						outtext << "ret" << endl;
					}
					if (linetmp.substr(0, 7) == "endproc") {
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
		}
		
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
			else outdata << it->first << "\tdb 0" << endl;
		}
		
		for (auto it = cdata.d32m.begin(); it != cdata.d32m.end(); advance(it, 1)) { 
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdb 0" << endl;
		}
		
		for (auto it = cdata.d64m.begin(); it != cdata.d64m.end(); advance(it, 1)) { 
			//If stack variable
			if (it->first.substr(0, 2) == "F_")
				outdata << "define " << it->first << "	rbp - " << it->second << endl;
			
			//If global variable
			else outdata << it->first << "\tdb 0" << endl;
		}
		
		output.open(((string) argv[1]).substr(0, ((string) argv[1]).find('.')) + ".asm");
		
		//Put buffer to output file
		output << outdata.rdbuf() << endl;
		output << outtext.rdbuf() << endl;
		
		output.close();
		
		cout << "Compilation end." << endl;
		
		system(((string) (
		#ifdef _WIN32
			"fasm "
		#endif
		#ifdef __linux__
			"fasm.exe "
		#endif
		 + ((string) argv[1]).substr(0, ((string) argv[1]).find('.')) + ".asm")).c_str());
		
	}
	else {
		cerr << "No input detected." << endl;
	}
	return 0;
}
