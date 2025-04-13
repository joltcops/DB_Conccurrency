CXX = g++-14
CXXFLAGS = -std=c++23

TARGET = lm
LOGS = logs.txt

SRCS = main.cpp lockmanager.cpp

all:
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

run:
	./$(TARGET) > logs.txt

clean:
	rm -f $(TARGET) $(LOGS)
