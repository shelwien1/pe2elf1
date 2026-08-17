CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++20 -fno-exceptions -fno-rtti -Wall -Wextra
LDLIBS   ?= -lm
LDFLAGS  ?=

SRC := src/main.cpp src/dst.cpp src/dsdiff.cpp src/dsf.cpp \
       src/dstenc.cpp src/dsfread.cpp src/dffwrite.cpp
OBJ := $(SRC:.cpp=.o)
BIN := dff2dsf

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/main.o:     src/common.h src/dsdiff.h src/dsf.h src/dst.h \
                src/dsfread.h src/dstenc.h src/dffwrite.h
src/dst.o:      src/common.h src/bits.h src/dst.h
src/dsdiff.o:   src/common.h src/bits.h src/dsdiff.h src/dst.h
src/dsf.o:      src/common.h src/dsf.h src/dst.h
src/dstenc.o:   src/common.h src/bitwrite.h src/dst.h src/dstenc.h
src/dsfread.o:  src/common.h src/dsf.h src/dsfread.h src/dst.h
src/dffwrite.o: src/common.h src/dffwrite.h src/dst.h

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
