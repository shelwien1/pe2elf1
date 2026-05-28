# Build pngdump, png2bmp, preflate_raw and pngtool.
#
# preflate_raw is the preflate demo (main2.cpp) — the same -s / -r interface
# as the original preflate demo, but built from main2.cpp so it has no
# zlib/packARI dependency.

CXX      := g++
CXXFLAGS := -O3 -Wall -Wno-misleading-indentation -funroll-loops
CXX11    := -std=c++11
CXX17    := -std=c++17
PREFLATE := preflate
PREFLATE_INCS := -I$(PREFLATE) -I$(PREFLATE)/support

PREFLATE_LIB_SRCS := \
  $(PREFLATE)/preflate_block_decoder.cpp \
  $(PREFLATE)/preflate_block_reencoder.cpp \
  $(PREFLATE)/preflate_block_trees.cpp \
  $(PREFLATE)/preflate_complevel_estimator.cpp \
  $(PREFLATE)/preflate_constants.cpp \
  $(PREFLATE)/preflate_decoder.cpp \
  $(PREFLATE)/preflate_hash_chain.cpp \
  $(PREFLATE)/preflate_info.cpp \
  $(PREFLATE)/preflate_parameter_estimator.cpp \
  $(PREFLATE)/preflate_parser_config.cpp \
  $(PREFLATE)/preflate_predictor_state.cpp \
  $(PREFLATE)/preflate_reencoder.cpp \
  $(PREFLATE)/preflate_seq_chain.cpp \
  $(PREFLATE)/preflate_statistical_codec.cpp \
  $(PREFLATE)/preflate_statistical_debug.cpp \
  $(PREFLATE)/preflate_statistical_model.cpp \
  $(PREFLATE)/preflate_token.cpp \
  $(PREFLATE)/preflate_token_predictor.cpp \
  $(PREFLATE)/preflate_tree_predictor.cpp \
  $(PREFLATE)/support/arithmetic_coder.cpp \
  $(PREFLATE)/support/array_helper.cpp \
  $(PREFLATE)/support/bit_helper.cpp \
  $(PREFLATE)/support/bitstream.cpp \
  $(PREFLATE)/support/const_division.cpp \
  $(PREFLATE)/support/filestream.cpp \
  $(PREFLATE)/support/huffman_decoder.cpp \
  $(PREFLATE)/support/huffman_encoder.cpp \
  $(PREFLATE)/support/huffman_helper.cpp \
  $(PREFLATE)/support/memstream.cpp \
  $(PREFLATE)/support/outputcachestream.cpp \
  $(PREFLATE)/support/task_pool.cpp

PREFLATE_LIB_OBJS := $(patsubst %.cpp,build/%.o,$(PREFLATE_LIB_SRCS))

.PHONY: all clean test
all: pngdump png2bmp preflate_raw pngtool

clean:
	rm -rf build pngdump png2bmp preflate_raw pngtool

pngdump: pngdump.cpp
	$(CXX) $(CXX17) $(CXXFLAGS) $< -o $@

png2bmp: png2bmp.cpp
	$(CXX) $(CXX17) $(CXXFLAGS) $< -o $@

build/libpreflate.a: $(PREFLATE_LIB_OBJS)
	@mkdir -p $(@D)
	ar rcs $@ $^

build/$(PREFLATE)/%.o: $(PREFLATE)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXX11) $(CXXFLAGS) $(PREFLATE_INCS) -c $< -o $@

preflate_raw: $(PREFLATE)/main2.cpp build/libpreflate.a
	$(CXX) $(CXX11) $(CXXFLAGS) $(PREFLATE_INCS) $< build/libpreflate.a -lpthread -o $@

pngtool: pngtool.cpp build/libpreflate.a
	$(CXX) $(CXX17) $(CXXFLAGS) $(PREFLATE_INCS) $< build/libpreflate.a -lpthread -o $@

test: all
	./test.sh
