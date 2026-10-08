# Builds ROSE for 64-bit Windows with MinGW-w64 GCC: the library bin\rose.dll (with the import
# library lib\rose.lib), and the programs identityTranslator, dotGenerator, rose-ren, rose-using
# and rose-m2g.  In this folder (see BUILD.txt):
#
#   mingw32-make -j8
#
# The sources are under src\ (src\sources.mk lists them, and the options they are compiled
# with); the objects are written to obj\.  Then the folder is a ROSE installation like the
# binary package: run test.bat.
#
# The recipes only run the compiler, ar, nm (through obj\rose-exports.exe), and mkdir/rmdir, so
# that they work with cmd.exe as the shell; the compiler options are passed in response files
# (obj\<group>.rsp).

ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
exe = $(subst /,\,$(1))
else
exe = ./$(1)
endif

OPT := -O2
NM := nm
LIBS := -lshlwapi -lws2_32 -lbcrypt -lpsapi

include src/sources.mk

TOOLS := identityTranslator dotGenerator rose-ren rose-using rose-m2g
PROGRAMS := $(patsubst %,bin/%.exe,$(TOOLS))

all: $(PROGRAMS)
.PHONY: all clean

# The object of a source file: obj/<path below src/, with "." for "/">.o
object = obj/$(subst /,.,$(basename $(patsubst src/%,%,$(1)))).o

define rsp_rule
obj/$(1).rsp: | obj
	$$(file >$$@,$$(FLAGS_$(1)))
endef

define compile_rule
$(call object,$(1)): $(1) | obj/$(2).rsp
	$$(CXX) @obj/$(2).rsp -MMD -MP -c $(1) -o $$@
endef

$(foreach g,$(GROUPS),$(eval $(call rsp_rule,$(g)))$(foreach s,$(SRCS_$(g)),$(eval $(call compile_rule,$(s),$(g)))))

objects = $(foreach g,$(1),$(foreach s,$(SRCS_$(g)),$(call object,$(s))))
LIB_OBJS := $(call objects,$(filter-out tools boost_%,$(GROUPS)))
BOOST_OBJS := $(call objects,$(filter boost_%,$(GROUPS)))

obj/librose.a: $(LIB_OBJS)
	$(file >obj/librose.rsp,$(LIB_OBJS))
	$(AR) rcs $@ @obj/librose.rsp

obj/boost.a: $(BOOST_OBJS)
	$(file >obj/boost.rsp,$(BOOST_OBJS))
	$(AR) rcs $@ @obj/boost.rsp

# A DLL cannot export more than 65535 names, and librose defines more: rose-exports lists ROSE's
# API (the names that librose.a defines, without those of EDG, of the C and C++ run-time
# libraries, of instances of standard library and Boost templates, and of the internals of the
# IR classes) in the module definition file obj\rose.def
obj/rose-exports.exe: src/win32/build/rose-exports.C | obj
	$(CXX) -O1 -s -o $@ $<

obj/rose.def: obj/librose.a obj/rose-exports.exe
	$(call exe,obj/rose-exports.exe) $(NM) obj/librose.a $@

# rose.dll, with the C and C++ run-time libraries linked statically (no MinGW DLLs are needed)
bin/rose.dll: obj/rose.def obj/librose.a obj/boost.a | bin lib
	$(CXX) -shared -static -pthread -s -o $@ obj/rose.def -Wl,--out-implib,lib/rose.lib \
	  -Wl,--whole-archive obj/librose.a -Wl,--no-whole-archive obj/boost.a $(LIBS)
lib/rose.lib: bin/rose.dll ;

# The programs, with a 64 MB stack for the deep recursion in EDG and ROSE
bin/%.exe: obj/tools.%.o lib/rose.lib | bin
	$(CXX) -static -pthread -Wl,--stack,67108864 -s -o $@ $< lib/rose.lib $(LIBS)

obj bin lib:
	mkdir $@

clean:
ifeq ($(OS),Windows_NT)
	if exist obj rmdir /s /q obj
	if exist bin rmdir /s /q bin
	if exist lib rmdir /s /q lib
else
	rm -rf obj bin lib
endif

-include $(LIB_OBJS:.o=.d) $(BOOST_OBJS:.o=.d) $(patsubst %,obj/tools.%.d,$(TOOLS))
