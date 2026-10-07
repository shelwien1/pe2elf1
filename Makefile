################################################################################
# ROSE (C/C++ support) built from source with the open-source EDG front end.
#
#   make -j$(nproc)     build build/lib/librose.so and build/bin/<tools>
#   make check          run the translator tests in tests/
#   make clean          remove the build directory
#
# Sources:
#   rose/      subset of ROSE 2.18.0     https://github.com/llnl/rose      (BSD-3)
#   edg/       EDG C/C++ front end 7.0   https://github.com/edgcpp/compiler (Apache-2.0 WITH LLVM-exception)
#   edg2sage/  new EDG IL -> ROSE Sage III AST translation (replaces ROSE's unreleased EDG/Sage connection)
#   tools/     translators built against librose
#
# Requirements: GNU make, g++ (C++14), flex, bison, zlib and the Boost libraries
# chrono date_time filesystem iostreams program_options random regex system
# thread wave serialization (e.g. "apt install libboost-all-dev flex bison").
################################################################################

ROSE_SRC := rose
EDG_SRC  := edg
CONN_SRC := edg2sage
TOOL_SRC := tools
B        := build

CXX      ?= g++
BACKEND_CC  ?= gcc
BACKEND_CXX ?= $(CXX)
FLEX     ?= flex
BISON    ?= bison
AR       ?= ar

OPT      ?= -O2
BOOST_ROOT ?=
ifneq ($(BOOST_ROOT),)
BOOST_CPPFLAGS := -isystem $(BOOST_ROOT)/include
BOOST_LDFLAGS  := -L$(BOOST_ROOT)/lib -Wl,-rpath,$(BOOST_ROOT)/lib
endif
BOOST_LIBS := -lboost_date_time -lboost_filesystem -lboost_iostreams -lboost_program_options \
              -lboost_random -lboost_regex -lboost_system -lboost_wave -lboost_chrono \
              -lboost_thread -lboost_atomic -lboost_serialization
SYS_LIBS   := -lz -ldl -lrt -pthread

include rose-sources.mk

# A literal '#' usable inside $(shell ...) with all GNU make versions
HASH := \#

ROSE_VERSION := $(shell cat $(ROSE_SRC)/ROSE_VERSION)
ROSE_VERSION_NUMBER := $(shell cat $(ROSE_SRC)/config/SCM_DATE)
EDG_VERSION  := $(shell sed -n 's/^$(HASH)define VERSION_NUMBER "\([0-9.]*\)".*/\1/p' $(EDG_SRC)/src/version.h)

# "make V=1" shows the full compiler command lines
ifeq ($(V),1)
Q :=
msg = @true
else
Q := @
msg = @printf '  %-8s %s\n' $(1) $(2)
endif

GEN  := $(B)/gen
OBJ  := $(B)/obj
SAGE_GEN := $(GEN)/src/frontend/SageIII

################################################################################
# Compiler flags
################################################################################

ROSE_DEFS := -DBOOST_BIND_GLOBAL_PLACEHOLDERS -DROSE_ASSERTION_BEHAVIOR=ROSE_ASSERTION_EXIT \
             -D_GLIBCXX_USE_CXX11_ABI=1 -Ddisc_union=union
ROSE_CPPFLAGS := $(ROSE_DEFS) -I$(GEN) -I$(SAGE_GEN) -I$(SAGE_GEN)/astFileIO -I$(GEN)/src \
                 $(addprefix -I$(ROSE_SRC)/,$(ROSE_INCDIRS)) $(BOOST_CPPFLAGS)
ROSE_CXXFLAGS := -std=c++14 $(OPT) -fPIC -pthread -w

# EDG is configured as a callable library ("source analysis" configuration: no IL lowering, no
# EDG back end; the IL is handed to edg2sage's back_end() routine), see edg2sage/edgconfig/.
EDG_CONFIG   := $(CONN_SRC)/edgconfig/edg_config.h
EDG_CPPFLAGS := -DUSE_CMAKE_DEFINES -include $(EDG_CONFIG) -I$(CONN_SRC)/edgconfig -I$(GEN)/edg -I$(EDG_SRC)/src
EDG_CXXFLAGS := -x c++ -std=c++14 $(OPT) -fPIC -fno-rtti -w

################################################################################
# Top-level targets
################################################################################

TOOLS := identityTranslator dotGenerator
LIBROSE := $(B)/lib/librose.so

all: $(addprefix $(B)/bin/,$(TOOLS)) $(B)/edg-base/lib/predefined_macros.txt
.PHONY: all check clean rosetta

clean:
	rm -rf $(B)

################################################################################
# Configuration headers (what ROSE's configure/CMake step would produce)
################################################################################

CXX_PATH := $(shell command -v $(BACKEND_CXX))
CC_PATH  := $(shell command -v $(BACKEND_CC))
CXX_VER  := $(subst ., ,$(shell $(BACKEND_CXX) -dumpfullversion -dumpversion))
CC_VER   := $(subst ., ,$(shell $(BACKEND_CC) -dumpfullversion -dumpversion))
# The backend compilers' system include directories, as a C initializer list ("dir1", "dir2", ...)
sys_incs = $(shell echo | $(1) -x$(2) -E -v - 2>&1 | sed -n '/^$(HASH)include <...> search starts here:/,/^End of search list/p' | \
             sed -n 's/^ \(.*\)/"\1",/p' | tr '\n' ' ' | sed 's/, *$$//')
BOOST_VERSION := $(shell printf '$(HASH)include <boost/version.hpp>\nBOOST_VERSION\n' | \
                   $(CXX) $(BOOST_CPPFLAGS) -E -P -x c++ - 2>/dev/null | tail -1 | \
                   awk '{printf "%d.%d.%d", $$1/100000, $$1/100%1000, $$1%100}')

CONFIG_SUBST = sed -e 's|@ROSE_VERSION@|$(ROSE_VERSION)|g' \
  -e 's|@ROSE_VERSION_NUMBER@|$(ROSE_VERSION_NUMBER)|g' \
  -e 's|@BOOST_VERSION@|$(BOOST_VERSION)|g' \
  -e 's|@EDG_MAJOR@|$(word 1,$(subst ., ,$(EDG_VERSION)))|g' \
  -e 's|@EDG_MINOR@|$(word 2,$(subst ., ,$(EDG_VERSION)))|g' \
  -e 's|@BUILD_DIR@|$(abspath $(B))|g' \
  -e 's|@ROSE_SRC_DIR@|$(abspath $(ROSE_SRC))|g' \
  -e 's|@BUILD_DATE@|$(shell date -u +%Y-%m-%d)|g' \
  -e 's|@CXX_PATH@|$(CXX_PATH)|g' -e 's|@CXX_NAME@|$(notdir $(CXX_PATH))|g' \
  -e 's|@CXX_MAJOR@|$(word 1,$(CXX_VER))|g' -e 's|@CXX_MINOR@|$(word 2,$(CXX_VER) 0)|g' \
  -e 's|@CXX_PATCH@|$(word 3,$(CXX_VER) 0 0)|g' \
  -e 's|@CC_PATH@|$(CC_PATH)|g' -e 's|@CC_NAME@|$(notdir $(CC_PATH))|g' \
  -e 's|@CC_MAJOR@|$(word 1,$(CC_VER))|g' -e 's|@CC_MINOR@|$(word 2,$(CC_VER) 0)|g' \
  -e 's|@CC_PATCH@|$(word 3,$(CC_VER) 0 0)|g' \
  -e 's|@CXX_INCLUDE_DIRS@|$(call sys_incs,$(BACKEND_CXX),c++)|g' \
  -e 's|@C_INCLUDE_DIRS@|$(call sys_incs,$(BACKEND_CC),c)|g'

# The generated files are only replaced when their content changes (avoids needless rebuilds).
define config_file
	@mkdir -p $(@D)
	$(Q)$(CONFIG_SUBST) $< > $@.tmp
	$(Q)if cmp -s $@.tmp $@; then rm $@.tmp; else mv $@.tmp $@; fi
endef
$(GEN)/rose_config.h: config/rose_config.h.in Makefile
	$(config_file)
$(GEN)/rosePublicConfig.h: config/rosePublicConfig.h.in Makefile
	$(config_file)
$(GEN)/src/util/rose_paths.C: config/rose_paths.C.in Makefile
	$(config_file)
$(GEN)/src/Rose/CommandLine/LicenseString.h: $(ROSE_SRC)/src/Rose/CommandLine/LicenseString.pre
	@mkdir -p $(@D)
	cp $< $@

CONFIG_HEADERS := $(GEN)/rose_config.h $(GEN)/rosePublicConfig.h $(GEN)/src/Rose/CommandLine/LicenseString.h

################################################################################
# ROSETTA: build the IR generator and generate the Sage III IR classes
################################################################################

ROSE_UTIL_OBJS := $(patsubst %.C,$(OBJ)/rose/%.o,$(ROSE_UTIL_SRCS)) $(OBJ)/gen/src/util/rose_paths.o
ROSETTA_OBJS   := $(patsubst %.C,$(OBJ)/rose/%.o,$(ROSETTA_SRCS))

$(B)/bin/CxxGrammarMetaProgram: $(ROSETTA_OBJS) $(ROSE_UTIL_OBJS)
	$(call msg,LINK,$@)
	@mkdir -p $(@D)
	$(Q)$(CXX) -pthread -o $@ $^ $(BOOST_LDFLAGS) $(BOOST_LIBS) $(SYS_LIBS)

# ROSETTA reads ../Grammar/* relative to its working directory and writes into the given directory.
# Some per-class headers are only generated for binary analysis; create them empty.
ROSETTA_STAMP := $(SAGE_GEN)/rosetta.stamp
$(ROSETTA_STAMP): $(B)/bin/CxxGrammarMetaProgram $(wildcard $(ROSE_SRC)/src/ROSETTA/Grammar/*) $(ROSE_SRC)/src/ROSETTA/astNodeList
	$(call msg,ROSETTA,$(SAGE_GEN))
	$(Q)rm -rf $(B)/rosetta && mkdir -p $(B)/rosetta/src $(B)/rosetta/out/astFileIO $(B)/rosetta/out/Rose/Traits
	$(Q)cp -r $(ROSE_SRC)/src/ROSETTA/Grammar $(B)/rosetta/Grammar
	$(Q)cd $(B)/rosetta/src && $(abspath $<) $(abspath $(B))/rosetta/out/ > ../rosetta.log
	$(Q)for n in $$(cat $(ROSE_SRC)/src/ROSETTA/astNodeList); do \
	  case $$n in Sg*) [ -f $(B)/rosetta/out/$$n.h ] || touch $(B)/rosetta/out/$$n.h;; esac; done
	$(Q)cd $(B)/rosetta/out && find . -type f | while read f; do \
	  cmp -s "$$f" "$(abspath $(SAGE_GEN))/$$f" || { mkdir -p "$$(dirname "$(abspath $(SAGE_GEN))/$$f")"; \
	    cp "$$f" "$(abspath $(SAGE_GEN))/$$f"; }; done
	$(Q)touch $@
rosetta: $(ROSETTA_STAMP)

ROSETTA_GEN_SRCS := $(addprefix $(SAGE_GEN)/,Cxx_Grammar.C Cxx_GrammarCheckingIfDataMembersAreInMemoryPool.C \
  Cxx_GrammarCopyMemberFunctions.C Cxx_GrammarGetChildIndex.C Cxx_GrammarMemoryPoolSupport.C \
  Cxx_GrammarNewAndDeleteOperators.C Cxx_GrammarNewConstructors.C Cxx_GrammarProcessDataMemberReferenceToPointers.C \
  Cxx_GrammarReturnClassHierarchySubTree.C Cxx_GrammarReturnDataMemberPointers.C Cxx_GrammarRTI.C \
  Cxx_GrammarNodeIdSupport.C Cxx_GrammarTraverseMemoryPool.C Cxx_GrammarTreeTraversalSuccessorContainer.C \
  Cxx_GrammarVariantEnumNames.C Cxx_GrammarVisitorSupport.C StorageClasses.C AST_FILE_IO.C)
$(ROSETTA_GEN_SRCS): $(ROSETTA_STAMP) ;

################################################################################
# Lexers and parsers (preprocessor-directive lexer, OpenMP parser)
################################################################################

$(SAGE_GEN)/lex.yy.C: $(ROSE_SRC)/src/frontend/SageIII/preproc-c.ll
	@mkdir -p $(@D)
	$(FLEX) -t $< > $@
$(SAGE_GEN)/omp-lex.yy.C: $(ROSE_SRC)/src/frontend/SageIII/omplexer.ll
	@mkdir -p $(@D)
	$(FLEX) -t $< > $@
$(SAGE_GEN)/ompparser.C: $(ROSE_SRC)/src/frontend/SageIII/ompparser.yy
	@mkdir -p $(@D)
	$(BISON) $< -o $(SAGE_GEN)/ompparser.c && mv $(SAGE_GEN)/ompparser.c $@
$(SAGE_GEN)/ompparser.h: $(SAGE_GEN)/ompparser.C ;

################################################################################
# librose
################################################################################

ROSE_LIB_OBJS := $(patsubst %.C,$(OBJ)/rose/%.o,$(filter %.C,$(ROSE_LIB_SRCS))) \
                 $(patsubst %.cpp,$(OBJ)/rose/%.o,$(filter %.cpp,$(ROSE_LIB_SRCS))) \
                 $(patsubst %.cc,$(OBJ)/rose/%.o,$(filter %.cc,$(ROSE_LIB_SRCS)))
ROSE_GEN_OBJS := $(patsubst $(GEN)/%.C,$(OBJ)/gen/%.o,$(ROSETTA_GEN_SRCS) \
                   $(SAGE_GEN)/lex.yy.C $(SAGE_GEN)/omp-lex.yy.C $(SAGE_GEN)/ompparser.C)

# Everything except the ROSETTA generator itself needs the generated IR headers.
LIB_PREREQS := $(CONFIG_HEADERS) $(ROSETTA_STAMP) $(SAGE_GEN)/ompparser.h

$(ROSE_UTIL_OBJS) $(ROSETTA_OBJS): | $(CONFIG_HEADERS)
$(ROSE_LIB_OBJS) $(ROSE_GEN_OBJS): | $(LIB_PREREQS)

$(OBJ)/rose/%.o: $(ROSE_SRC)/%.C
	$(call msg,CXX,$<)
	@mkdir -p $(@D)
	$(Q)$(CXX) $(ROSE_CXXFLAGS) $(ROSE_CPPFLAGS) -MMD -MP -c $< -o $@
$(OBJ)/rose/%.o: $(ROSE_SRC)/%.cpp
	$(call msg,CXX,$<)
	@mkdir -p $(@D)
	$(Q)$(CXX) $(ROSE_CXXFLAGS) $(ROSE_CPPFLAGS) -MMD -MP -c $< -o $@
$(OBJ)/rose/%.o: $(ROSE_SRC)/%.cc
	$(call msg,CXX,$<)
	@mkdir -p $(@D)
	$(Q)$(CXX) $(ROSE_CXXFLAGS) $(ROSE_CPPFLAGS) -MMD -MP -c $< -o $@
$(OBJ)/gen/%.o: $(GEN)/%.C
	$(call msg,CXX,$<)
	@mkdir -p $(@D)
	$(Q)$(CXX) $(ROSE_CXXFLAGS) $(ROSE_CPPFLAGS) -MMD -MP -c $< -o $@

################################################################################
# EDG front end (compiled as C++, as EDG's own build does)
################################################################################

EDG_SRCS := $(addprefix $(EDG_SRC)/src/,attribute.c c_gen_be.c cfe.c class_decl.c cmd_line.c const_ints.c \
  cp_gen_be.c debug.c decl_inits.c decl_spec.c declarator.c decls.c def_arg.c disambig.c error.c expr.c \
  exprutil.c extasm.c fe_init.c fe_wrapup.c fixed_pt.c float_pt.c floating.c folding.c func_def.c \
  host_envir.c ifc_modules.c il.c il_alloc.c il_display.c il_read.c il_to_str.c il_walk.c il_write.c \
  inline.c interpret.c layout.c lexical.c literals.c lookup.c lower_c99.c lower_eh.c lower_il.c \
  lower_init.c lower_name.c macro.c mem_manage.c modules.c ms_attrib.c overload.c pch.c pragma.c \
  preproc.c scope_stk.c src_seq.c statements.c symbol_ref.c symbol_tbl.c sys_predef.c target.c \
  templates.c trans_copy.c trans_corresp.c trans_unit.c types.c cfe_daemon.c ifc_map_functions.c \
  ifc_map_functions_acc.c ifc_map_functions_dbg.c ifc_map_functions_mut.c ifc_map_functions_val.c \
  ifc_modules_read.c ifc_modules_templ.c ifc_modules_write.c unicode_name_fsm.c)
EDG_SRCS := $(wildcard $(EDG_SRCS))
EDG_OBJS := $(patsubst $(EDG_SRC)/src/%.c,$(OBJ)/edg/%.o,$(EDG_SRCS))
EDG_ERR_HEADERS := $(GEN)/edg/err_codes.h $(GEN)/edg/err_data.h

$(B)/bin/mk_errinfo: $(EDG_SRC)/util/mk_errinfo.c
	@mkdir -p $(@D)
	$(CXX) -x c++ -std=c++14 -O1 -w -I$(EDG_SRC)/src -o $@ $<
$(GEN)/edg/err_codes.h: $(B)/bin/mk_errinfo $(EDG_SRC)/src/error_msg.txt $(EDG_SRC)/src/error_tag.txt
	@mkdir -p $(@D)
	$(Q)$(B)/bin/mk_errinfo $(EDG_SRC)/src/error_msg.txt $(EDG_SRC)/src/error_tag.txt $(EDG_ERR_HEADERS)
$(GEN)/edg/err_data.h: $(GEN)/edg/err_codes.h ;

$(EDG_OBJS): | $(EDG_ERR_HEADERS)
$(OBJ)/edg/%.o: $(EDG_SRC)/src/%.c $(EDG_CONFIG)
	$(call msg,CXX,$<)
	@mkdir -p $(@D)
	$(Q)$(CXX) $(EDG_CXXFLAGS) $(EDG_CPPFLAGS) -MMD -MP -c $< -o $@

# EDG's table of predefined macros, scraped from the backend compilers (EDG's own script)
$(B)/edg-base/lib/predefined_macros.txt: $(EDG_SRC)/util/make_predef_macro_table
	@mkdir -p $(@D)
	cd $(@D) && sh $(abspath $<) --gcc $(BACKEND_CC) --g++ $(BACKEND_CXX) > make_predef_macro_table.log 2>&1

################################################################################
# edg2sage: EDG IL -> Sage III translation
################################################################################

CONN_SRCS := $(wildcard $(CONN_SRC)/*.C)
CONN_OBJS := $(patsubst $(CONN_SRC)/%.C,$(OBJ)/edg2sage/%.o,$(CONN_SRCS))
$(CONN_OBJS): | $(LIB_PREREQS) $(EDG_ERR_HEADERS)
$(OBJ)/edg2sage/%.o: $(CONN_SRC)/%.C $(EDG_CONFIG)
	$(call msg,CXX,$<)
	@mkdir -p $(@D)
	$(Q)$(CXX) $(ROSE_CXXFLAGS) $(ROSE_CPPFLAGS) -I$(CONN_SRC) $(EDG_CPPFLAGS) \
	  -DEDG2SAGE_EDG_BASE='"$(abspath $(B))/edg-base"' -MMD -MP -c $< -o $@

################################################################################
# Link librose and the tools
################################################################################

ALL_LIB_OBJS := $(ROSE_UTIL_OBJS) $(ROSE_LIB_OBJS) $(ROSE_GEN_OBJS) $(EDG_OBJS) $(CONN_OBJS)

$(LIBROSE): $(ALL_LIB_OBJS)
	$(call msg,LINK,$@)
	@mkdir -p $(@D)
	$(Q)$(CXX) -shared -pthread -o $@ $^ $(BOOST_LDFLAGS) $(BOOST_LIBS) $(SYS_LIBS)

$(OBJ)/tools/%.o: $(TOOL_SRC)/%.C | $(LIB_PREREQS)
	$(call msg,CXX,$<)
	@mkdir -p $(@D)
	$(Q)$(CXX) $(ROSE_CXXFLAGS) $(ROSE_CPPFLAGS) -MMD -MP -c $< -o $@

$(B)/bin/%: $(OBJ)/tools/%.o $(LIBROSE)
	$(call msg,LINK,$@)
	@mkdir -p $(@D)
	$(Q)$(CXX) -pthread -o $@ $< -L$(B)/lib -lrose -Wl,-rpath,$(abspath $(B))/lib $(BOOST_LDFLAGS) $(BOOST_LIBS) $(SYS_LIBS)

################################################################################
# Tests
################################################################################

check: all
	@sh tests/run-tests.sh $(B)/bin/identityTranslator

-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)

# Convenience targets for building parts of the tree
rose-objs: $(ROSE_UTIL_OBJS) $(ROSE_LIB_OBJS) $(ROSE_GEN_OBJS)
edg-objs: $(EDG_OBJS)
.PHONY: rose-objs edg-objs

# "make print-VAR" shows the value of a make variable
print-%:
	@echo '$($*)'
