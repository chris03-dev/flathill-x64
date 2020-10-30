echo Using Windows LLVM clang build script...
@echo off
mkdir ..\bin
windres ../src/win32res.rc -O coff -o win32res.res
clang++ -target x86_64-w64-mingw32-g++ -Wall -O3 -c ../src/linemod.cpp
clang++ -target x86_64-w64-mingw32-g++ -Wall -O3 -c ../src/flat.cpp
clang++ linemod.o flat.o win32res.res -target x86_64-w64-mingw32-g++ -s -Wl -o ../bin/flat.exe

cmd /c del /f linemod.o flat.o win32res.res