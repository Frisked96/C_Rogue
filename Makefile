# Default compiler
CXX = "C:\Program Files\LLVM\bin\clang++.exe"
CXXFLAGS = -std=c++20  -O3 -Wall -Wextra -I. -MMD -MP
FORMATTER = "C:\Program Files\LLVM\bin\clang-format.exe"

SRCS = main.cpp \
       engine.cpp \
       game_map.cpp \
       renderer/renderer.cpp \
       renderer/render_backend.cpp \
       renderer/compositor.cpp \
       renderer/world_renderer.cpp \
       renderer/entity_renderer.cpp \
       renderer/ui_renderer.cpp \
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
TEST_TARGET = test.exe
WEATHER_TEST_TARGET = weather_test.exe
all: $(TARGET) $(DIAG_TARGET) $(TEST_TARGET)

diag: $(DIAG_TARGET)

test: $(TEST_TARGET)

weather_test: $(WEATHER_TEST_TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

$(DIAG_TARGET): map_diag.o $(filter-out main.o, $(OBJS))
	$(CXX) $(CXXFLAGS) -o $(DIAG_TARGET) map_diag.o $(filter-out main.o, $(OBJS))

$(TEST_TARGET): test/live_test.o $(filter-out main.o, $(OBJS))
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) test/live_test.o $(filter-out main.o, $(OBJS))

$(WEATHER_TEST_TARGET): test/weather_test.o $(filter-out main.o, $(OBJS))
	$(CXX) $(CXXFLAGS) -o $(WEATHER_TEST_TARGET) test/weather_test.o $(filter-out main.o, $(OBJS))
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	if exist *.o del /q *.o
	if exist map_gen\*.o del /q map_gen\*.o
	if exist Entity\*.o del /q Entity\*.o
	if exist Object\*.o del /q Object\*.o
	if exist renderer\*.o del /q renderer\*.o
	if exist test\*.o del /q test\*.o
	if exist *.d del /q *.d
	if exist map_gen\*.d del /q map_gen\*.d
	if exist Entity\*.d del /q Entity\*.d
	if exist Object\*.d del /q Object\*.d
	if exist renderer\*.d del /q renderer\*.d
	if exist test\*.d del /q test\*.d
	if exist $(TARGET) del /q $(TARGET)
	if exist $(DIAG_TARGET) del /q $(DIAG_TARGET)
	if exist $(TEST_TARGET) del /q $(TEST_TARGET)
	if exist $(WEATHER_TEST_TARGET) del /q $(WEATHER_TEST_TARGET)
format:
	$(FORMATTER) -i $(SRCS) *.hpp Entity/*.hpp map_gen/*.hpp FastNoiseLite.h

.PHONY: all diag test clean format
