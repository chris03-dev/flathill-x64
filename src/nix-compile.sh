cd $(dirname "$0")
g++ -c -Wall -O3 -Os -std=c++11 linemod.cpp
g++ -c -Wall -O3 -Os -std=c++11 flat.cpp
g++ linemod.o flat.o -o ../bin/flat.elf
rm linemod.o flat.o
