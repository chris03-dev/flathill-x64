echo Using *nix GCC build script
cd $(dirname "$0")
mkdir -p ../bin
g++ -c -Wall -O3 -Os -Wall -std=c++11 ../src/linemod.cpp
g++ -c -Wall -O3 -Os -std=c++11 ../src/flat.cpp
g++ linemod.o flat.o -s -o ../bin/flat.elf
rm linemod.o flat.o