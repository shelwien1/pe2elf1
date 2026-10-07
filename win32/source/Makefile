# Builds ROSE's identityTranslator and dotGenerator for 64-bit Windows with MinGW-w64 GCC.
# In this folder (see BUILD.txt):
#
#   mingw32-make -j8
#
# The sources are under src\ (src\sources.mk lists them, and the options they are compiled
# with); the objects are written to obj\ and the programs to bin\.  Then the folder is a ROSE
# installation like the binary package: run test.bat.
#
# The recipes only run the compiler, ar, and mkdir/rmdir, so that they work with cmd.exe as
# the shell; the compiler options are passed in response files (obj\<group>.rsp).

ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
endif

OPT := -O2
LIBS := -lshlwapi -lws2_32 -lbcrypt -lpsapi

include src/sources.mk

TOOLS := identityTranslator dotGenerator
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

LIB_OBJS := $(foreach g,$(filter-out tools,$(GROUPS)),$(foreach s,$(SRCS_$(g)),$(call object,$(s))))

obj/librose.a: $(LIB_OBJS)
	$(file >obj/librose.rsp,$(LIB_OBJS))
	$(AR) rcs $@ @obj/librose.rsp

# Self-contained executables (no MinGW DLLs needed), with a 64 MB stack for the deep recursion
# in EDG and ROSE
bin/%.exe: obj/tools.%.o obj/librose.a | bin
	$(CXX) -static -pthread -Wl,--stack,67108864 -s -o $@ $< obj/librose.a $(LIBS)

obj bin:
	mkdir $@

clean:
ifeq ($(OS),Windows_NT)
	if exist obj rmdir /s /q obj
	if exist bin rmdir /s /q bin
else
	rm -rf obj bin
endif

-include $(LIB_OBJS:.o=.d) $(patsubst %,obj/tools.%.d,$(TOOLS))
