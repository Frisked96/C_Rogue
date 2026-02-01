# Default compiler
CXX ?= clang++
CXXFLAGS = -std=c++17 -Wall -Wextra

SRCS = main.cpp engine.cpp player.cpp game_map.cpp renderer.cpp input_handler.cpp entity.cpp tile.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = roguelike

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
