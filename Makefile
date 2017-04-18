main:
	g++ -g -Wall -std=c++11 -L/usr/local/boost_1_63_0/stage/lib -I /usr/local/boost_1_63_0 mass_edit.cpp -o mass_edit -lboost_system -lboost_filesystem
clean:
	rm mass_edit
