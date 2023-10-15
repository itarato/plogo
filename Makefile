UNAME_S := $(shell uname -s)

CXXFLAGS = -std=c++2a -Wall -pedantic -Wformat

ifeq ($(UNAME_S),Linux)
	LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif
ifeq ($(UNAME_S),Darwin)
	LIBS = -lm -lpthread -ldl -framework IOKit -framework Cocoa -framework OpenGL `pkg-config --libs --cflags raylib`
	CXXFLAGS += -I./lib
endif

BIN?=main
SRC=$(wildcard src/$(BIN).cpp)
OBJ=$(addsuffix .o,$(basename $(SRC)))

all: CXXFLAGS += -O3
all: executable

debug: CXXFLAGS += -g -O0
debug: executable

executable: $(OBJ)
	$(CXX) -o $(BIN) $^ $(CXXFLAGS) $(LIBS)

$(BIN).o:$(BIN).cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f ./src/*.o
	rm -f ./$(BIN)

run: clean all
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib ./$(BIN)

rebuild: clean
rebuild: all
