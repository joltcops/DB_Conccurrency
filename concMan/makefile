CXX = g++-14
CXXFLAGS = -std=c++23

LOGS = logs.txt
CLEAN_TARGETS = test0 test1 test2 test3 test4 test5

SRC = lockmanager.cpp

all:
	$(CXX) $(CXXFLAGS) $(SRC) test00.cpp -o test0
	$(CXX) $(CXXFLAGS) $(SRC) test01.cpp -o test1
	$(CXX) $(CXXFLAGS) $(SRC) test02.cpp -o test2
	$(CXX) $(CXXFLAGS) $(SRC) test03.cpp -o test3
	$(CXX) $(CXXFLAGS) $(SRC) test04.cpp -o test4
	$(CXX) $(CXXFLAGE) $(SRC) test05.cpp -o test5

clean:
	rm -f $(CLEAN_TARGETS) $(LOGS)
