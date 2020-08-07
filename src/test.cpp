// The following source code is licensed under the BSD 2-Clause Patent License.\
   Please read LICENSE.md for details.

#include <iostream>
#include <sstream>
#include <fstream>
#include "linemod.h"

int main() {
	std::stringstream stream;
	std::ofstream testfile;
	testfile.open("testout.txt");
	
	compilerdata cdata;
	
		cdata.d16m.insert(std::pair<std::string, std::string> ("G_s", "10"));
		if (not lexereq(stream, cdata, "10 + 2 - s + 0xAF / 5", "S_")) ;
	
	testfile << "END	" << stream.rdbuf() << std::endl;
	testfile.close();
	
	return 0;
}