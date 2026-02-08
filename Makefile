# Default compiler
CXX ?= clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.

SRCS = main.cpp \
       engine.cpp \
       game_map.cpp \
       renderer.cpp \
       input_handler.cpp \
       tile.cpp \
       Entity/entity.cpp \
       Entity/entity_manager.cpp \
       Entity/entity_factory.cpp \
       Entity/anatomy_components.cpp \
       Entity/anatomy_system.cpp \
       Entity/physiology_system.cpp \
       Entity/damage_resolution_system.cpp \
       Entity/spatial_system.cpp \
       Entity/system_manager.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
