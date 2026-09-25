CXX ?= g++
CXXFLAGS ?= -O2 -std=c++11 -Wall

all: predict metric

predict: predict.cpp tsv.h
	$(CXX) $(CXXFLAGS) -o $@ predict.cpp

metric: metric.cpp tsv.h
	$(CXX) $(CXXFLAGS) -o $@ metric.cpp

# unpack data/*.tsv.xz -> data/*.tsv
data:
	for f in data/*.tsv.xz; do [ -f "$${f%.xz}" ] || xz -dk "$$f"; done

clean:
	rm -f predict metric

.PHONY: all data clean
