g++ -c linemod.cpp
g++ -c flat.cpp
g++ linemod.o flat.o -o ../bin/flat.deb
rm linemod.o flat.o