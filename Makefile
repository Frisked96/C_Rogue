# Default compiler
CXX = "C:\Program Files\LLVM\bin\clang++.exe"
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -I. -MMD -MP

SRCS = main.cpp \
       engine.cpp \
       game_map.cpp \
       renderer.cpp \
       input_handler.cpp \
       tile.cpp \
       material.cpp \
       map_gen/map_generator.cpp \
       map_gen/simulator.cpp \
       map_gen/visibility.cpp \
       Entity/entity_properties.cpp \
       Entity/entity_manager.cpp \
       Entity/spatial_grid.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)
TARGET = main.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	if exist *.o del /q *.o
	if exist map_gen\*.o del /q map_gen\*.o
	if exist Entity\*.o del /q Entity\*.o
	if exist *.d del /q *.d
	if exist map_gen\*.d del /q map_gen\*.d
	if exist Entity\*.d del /q Entity\*.d
	if exist $(TARGET) del /q $(TARGET)

.PHONY: all clean
