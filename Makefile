all: cli gui
cli:
	g++ -g -Wall -std=c++11 -L/usr/local/boost_1_63_0/stage/lib -I /usr/local/boost_1_63_0 mass_edit.cpp cli_mass_edit.cpp -o cli_mass_edit -lboost_system -lboost_filesystem
gui:
	g++ -g -Wall -std=c++11 -L/usr/local/lib -I /usr/local/include mass_edit.cpp gui_mass_edit.cpp -o gui_mass_edit -lwt -lwthttp -lboost_system -lboost_filesystem
clean:
	rm mass_edit
