CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I includes
SOURCES = $(wildcard src/*.cpp)
TARGET = simulator

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
	rm -f output/*.txt output/*.dot

.PHONY: clean
