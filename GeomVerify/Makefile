# Makefile for Test

CXXFLAGS = -O2 $(DEBUGFLAGS) -fmessage-length=0 -std=c++0x -I./
LDFLAGS = -L../lib/

# Common
CXXFLAGS += -I../Common/include/
LDFLAGS += -lcommon

# KMajority
CXXFLAGS += -I../KMajorityLib/include
LDFLAGS += -lkmajority

# VocabLib (dependency upon DirectIndex)
CXXFLAGS += -I../VocabLib/include
LDFLAGS += -lvocab

# OpenCV
CXXFLAGS += `pkg-config opencv --cflags`
LDFLAGS += `pkg-config opencv --libs`

SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

BIN = GeomVerify

all: $(BIN)

$(BIN): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(BIN) $(LDFLAGS)

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(BIN) *~
