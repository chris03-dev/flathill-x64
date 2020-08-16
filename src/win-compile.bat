@echo off
windres win32res.rc -O coff -o win32res.res
g++ -Wall -O3 -Os -Wall -std=c++11 -c linemod.cpp 
g++ -Wall -O3 -Os -Wall -std=c++11 -c flat.cpp 
g++ linemod.o flat.o win32res.res -s -o ../bin/flat.exe

cmd /c del /f linemod.o flat.o win32res.res