OBJECTS=$(patsubst src/%.cpp,build/%.o,$(wildcard src/*cpp))
CC=g++
CXXFLAGS=-O2 -Wall -std=c++11 -Iheaders
LDFLAGS=

.PHONY: all clean

all: build/cotton

build/cotton: ${OBJECTS}
	${CC} ${OBJECTS} -o build/cotton

build/%.o: src/%.cpp $(wildcard headers/*hpp)
	${CC} ${CXXFLAGS} -c -o $@ $<

clean:
	rm -f build/cotton ${OBJECTS}
