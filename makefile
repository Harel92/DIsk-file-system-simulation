all: TestProject.cpp
	g++ TestProject.cpp -o TestProject
all-GDB: TestProject.cpp
	g++ -g TestProject.cpp -o TestProject