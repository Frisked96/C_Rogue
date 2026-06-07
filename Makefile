# Default compiler
CXX = "C:\Program Files\LLVM\bin\clang++.exe"
CXXFLAGS = -std=c++17 -Wall -Wextra -I. -MMD -MP

SRCS = main.cpp \
       engine.cpp \
       game_map.cpp \
       renderer.cpp \
       input_handler.cpp \
       tile.cpp \
       material.cpp \
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
DEPS = $(SRCS:.cpp=.d)
TARGET = main.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

.PHONY: all clean
