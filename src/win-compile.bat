@echo off
g++ -Wall -O3 -Os -std=c++11 -c linemod.cpp 
g++ -Wall -O3 -Os -std=c++11 -c flat.cpp 
g++ linemod.o flat.o -o ../bin/flat.exe

cmd /c del /f linemod.o flat.o