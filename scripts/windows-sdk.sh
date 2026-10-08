#!/bin/sh
# Adds the SDK to the Windows package (run by "make package"):
#   include/rose/   the headers that programs using ROSE include: those that the tools include
#                   (rose.h, RoseRefactor.h and what they include, taken from the dependency
#                   files of the tools), and the parts of Boost that they include
#   include/rose/rose.rsp
#                   the compiler options, with the include directories relative to include/rose
#                   (for gcc -iprefix <package>/include/rose/ @<package>/include/rose/rose.rsp)
#   rose.mk         the compiler and linker options, for Makefiles
#   examples/tools/ the tools' sources, with a Makefile that builds them with the SDK
#
# usage: scripts/windows-sdk.sh <variables file written by the Makefile>
set -eu
. "$1"
inc=$PKG_DIR/include/rose
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT INT TERM
mkdir -p "$inc"

# Where a header of the build goes in include/rose
dest_of() {
  case $1 in
    "$GEN") echo "gen" ;;
    "$GEN"/*) echo "gen/${1#"$GEN"/}" ;;
    rose/*) echo "${1#rose/}" ;;
    /*) echo "" ;;
    *) echo "$1" ;;
  esac
}

# ROSE's headers
for o in $TOOL_OBJS; do cat "${o%.o}.d"; done | tr ' \\' '\n\n' | sed -n 's/:$//; /./p' | grep -v '\.o$' |
  sort -u > "$tmp/files"
while read -r f; do
  case $f in
    "$TOOL_SRC"/*) continue ;;
  esac
  d=$(dest_of "$f")
  if [ -z "$d" ]; then
    echo "windows-sdk.sh: $f is outside the source tree and the generated files" >&2
    exit 1
  fi
  mkdir -p "$inc/$(dirname "$d")"
  cp "$f" "$inc/$d"
done < "$tmp/files"

# Boost's headers: those the tools include (gcc -M lists the system headers too)
for o in $TOOL_OBJS; do
  src=$TOOL_SRC/$(basename "${o%.o}").C
  $CXX $FLAGS -M "$src" > "$tmp/deps"
  tr ' \\' '\n\n' < "$tmp/deps" | sed -n "s|^$BOOST_INC/||p" >> "$tmp/boost"
done
sort -u "$tmp/boost" | while read -r f; do
  mkdir -p "$inc/$(dirname "$f")"
  cp "$BOOST_INC/$f" "$inc/$f"
done

# The compiler options: include directories relative to include/rose (that the SDK has)
# (one option per line; a response file has no comments)
{
  set -- $FLAGS
  while [ $# -gt 0 ]; do
    a=$1
    shift
    case $a in
      -I*)
        d=$(dest_of "${a#-I}")
        [ -n "$d" ] && [ -d "$inc/$d" ] && echo "-iwithprefixbefore $d"
        ;;
      -isystem)
        shift  # Boost: in include/rose itself
        ;;
      -include)
        echo "-include $(dest_of "$1")"
        shift
        ;;
      *)
        case " $OPT " in
          *" $a "*) ;;
          *) echo "$a" ;;
        esac
        ;;
    esac
  done
} > "$inc/rose.rsp"
grep -q '^-iwithprefixbefore \.$' "$inc/rose.rsp" || echo "-iwithprefixbefore ." >> "$inc/rose.rsp"

# rose.mk and the examples
cp win32/sdk/rose.mk "$PKG_DIR/rose.mk"
mkdir -p "$PKG_DIR/examples/tools"
for o in $TOOL_OBJS; do cp "$TOOL_SRC/$(basename "${o%.o}").C" "$PKG_DIR/examples/tools/"; done
cp win32/sdk/Makefile.examples "$PKG_DIR/examples/tools/Makefile"
