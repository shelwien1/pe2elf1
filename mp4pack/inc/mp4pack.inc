// inc/mp4pack.inc -- mp4pack's engine: all of it but the command line
// program's main (mp4pack.cpp). mp4pack.cpp, mp4gui.cpp and the tests include
// this. Each part below is a piece of one source, no header of its own: each
// takes what the ones before it have, in this order.
//
//   base.inc        the platform (windows.h, or winapi_posix.h), the types
//   methods.inc     the groups and methods (packers); a stream's, by its name
//   state.inc       the settings (the options) and a run's state
//   strings.inc     strings: UTF-8 and wide, numbers, lines, columns, SysErr
//   messages.inc    the messages (console, or msg_hook); a run's error, summary
//   crc32.inc       CRC32
//   files.inc       files and paths, reading and writing, directories
//   builds.inc      the CPU and the tool builds for it (GetTool)
//   work.inc        the work directory
//   tools.inc       a tool's process, as a job: its pipes and log
//   stages.inc      running the jobs: progress, Pause, stages, one job
//   streams.inc     the streams and their packers' jobs; the index
//   options.inc     -t, -c, -l; the stages, the part sizes, the levels
//   archive.inc     the archive: its members written, scanned, read
//   compress.inc    c
//   decompress.inc  d (a member)
//   lists.inc       @list; the names of the outputs in a directory
//   run.inc         the run: Init, each file, a list's files, Setup, RunList
//   cmdline.inc     the command line (mp4pack's, and mp4gui's)

#include "base.inc"
#include "methods.inc"
#include "state.inc"
#include "strings.inc"
#include "messages.inc"
#include "crc32.inc"
#include "files.inc"
#include "builds.inc"
#include "work.inc"
#include "tools.inc"
#include "stages.inc"
#include "streams.inc"
#include "options.inc"
#include "archive.inc"
#include "compress.inc"
#include "decompress.inc"
#include "lists.inc"
#include "run.inc"
#include "cmdline.inc"
