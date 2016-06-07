OBJECTS=$(patsubst src/%.cpp,build/%.o,$(wildcard src/*cpp))
CXX?=g++
CXXFLAGS=-O2 -Wall -std=c++14 -ftemplate-depth=1024 -Iheaders -ggdb -Iprogram-options/headers

ifdef BOOST_PATH
BOOST_FLAGS=-L${BOOST_PATH}
else
BOOST_FLAGS=
endif

LDFLAGS=-lboost_iostreams -lboost_serialization ${BOOST_FLAGS}

.PHONY: all clean

all: build/cotton

build/cotton: ${OBJECTS}
	${CXX} ${OBJECTS} ${LDFLAGS} -o build/cotton

build/%.o: src/%.cpp $(wildcard headers/*hpp) $(wildcard program-options/headers/*hpp)
	${CXX} ${CXXFLAGS} -c -o $@ $<

clean:
	rm -f build/cotton ${OBJECTS}
