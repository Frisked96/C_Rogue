# Default compiler
CXX = "C:\Program Files\LLVM\bin\clang++.exe"
CXXFLAGS = -std=c++20  -O3 -Wall -Wextra -I. -MMD -MP
FORMATTER = "C:\Program Files\LLVM\bin\clang-format.exe"

SRCS = main.cpp \
       engine.cpp \
       game_map.cpp \
       renderer.cpp \
       input_handler.cpp \
       tile.cpp \
       material.cpp \
       map_gen/map_generator.cpp \
       map_gen/climate_hydrology.cpp \
       map_gen/simulator.cpp \
       map_gen/visibility.cpp \
       Entity/entity_properties.cpp \
       Entity/entity_manager.cpp \
       Object/event_bus.cpp \
       Object/object_prototype_db.cpp \
       Object/object_manager.cpp \
       Object/object_spawner.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)
TARGET = main.exe
DIAG_TARGET = map_diag.exe

all: $(TARGET) $(DIAG_TARGET)

diag: $(DIAG_TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

$(DIAG_TARGET): map_diag.o $(filter-out main.o, $(OBJS))
	$(CXX) $(CXXFLAGS) -o $(DIAG_TARGET) map_diag.o $(filter-out main.o, $(OBJS))

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	if exist *.o del /q *.o
	if exist map_gen\*.o del /q map_gen\*.o
	if exist Entity\*.o del /q Entity\*.o
	if exist Object\*.o del /q Object\*.o
	if exist *.d del /q *.d
	if exist map_gen\*.d del /q map_gen\*.d
	if exist Entity\*.d del /q Entity\*.d
	if exist Object\*.d del /q Object\*.d
	if exist $(TARGET) del /q $(TARGET)
	if exist $(DIAG_TARGET) del /q $(DIAG_TARGET)

format:
	$(FORMATTER) -i $(SRCS) *.hpp Entity/*.hpp map_gen/*.hpp FastNoiseLite.h

.PHONY: all diag clean format
