CXX = g++
CXXFLAGS = -std=c++17 -Wall -DNCURSES_STATIC
LDFLAGS = -lncurses
TARGET = checkers
SRC = checkers.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ) log.txt
