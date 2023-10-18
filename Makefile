CXXFLAGS = -std=c++2a -Wall -pedantic -Wformat -I./lib/imgui -I./lib/rlImGui
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

MAINSRC=$(wildcard src/main.cpp lib/imgui/*.cpp lib/rlImGui/*.cpp)
OBJ=$(addsuffix .o,$(basename $(MAINSRC)))

TESTSRC=$(wildcard src/tests.cpp)
TESTOBJ=$(addsuffix .o,$(basename $(TESTSRC)))

.PHONY: all debug clean test

all: CXXFLAGS += -O0
all: plogo

debug: CXXFLAGS += -g -O0
debug: plogo

plogo: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

test: $(TESTOBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	
%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f ./src/*.o
	rm -f ./plogo

cleandeep:
	rm -f ./src/*.o
	rm -f ./plogo
	rm -f ./lib/imgui/*.o
	rm -f ./lib/rlImGui/*.o
