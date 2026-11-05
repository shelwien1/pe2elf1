// mp4pack.cpp -- lossless compression of whole mp4/mkv/avi files (Windows;
// Linux with winapi_posix.h, the Win32 functions used here on POSIX)
//
//   mp4pack c [options] input output     compress
//   mp4pack d [options] input output     decompress
//
// c: "avip c" splits the container into its streams and a .meta file in a work
// directory; each stream goes to its packer -- h264p (video), aacpack, mp3pack
// or oggcomp (audio), ttfcomp (fonts, all in one solid archive), tangelo (the
// rest, in one solid archive; its -9, or the level -l gives) -- and the packed
// files and an index
// (mp4pack.idx) are stored as the output, mp4pack's own archive
// (WriteArchive).
// d: the archive's files (ReadArchive), the unpackers, "avip d"; every stream
// and the result are checked against the sizes and CRC32s in the index.
// Before either, the output's directory is made if it is not there, and the
// output tried (OutputCheck); the errors end with the system's reason (SysErr).
//
// The tools are run from the directory of mp4pack.exe, each in the best build
// there for this CPU (h264p_znver4.exe, h264p_avx2.exe, h264p.exe, ...: by the
// -march target in the name, ranked by x86targets.h: GetTool), with the work
// directory as their current one and relative paths on their command lines:
// they take ANSI command lines, so only names that survive that reach them
// (ToolName). On Linux they are the Windows tools converted by pe2elf (named
// without .exe: h264p_avx2), which take '\' for '/', and UTF-8.
//
// The engine is in inc/ (inc/mp4pack.inc includes its parts, by purpose):
// this file is the command line program's main. mp4gui.cpp, a Windows GUI,
// includes the same engine: it calls Init, ParseArgs (its command line is
// mp4pack's) and RunList as wmain does (through Run), and gets the messages,
// the progress and a list's files through msg_hook, status_hook and
// file_hook instead of the console.

#include "inc/mp4pack.inc"

// c or d of in (a file, or @list) to out
static int Run( const wstring& in, const wstring& out ) {
  vector<wstring> ins;
  if( in.size() && (in[0]=='@') ) {
    string why;
    if( ReadList( in.substr( 1 ), ins, why ) ) { Msg( why ); run_err = why; return 2; }
    if( ins.empty() ) { Msg( "no files in the list " + U( in.substr( 1 ) ) ); run_err = last_msg; return 1; }
  } else ins.push_back( in );
  return RunList( ins, out, 0 );
}

static HANDLE ev_done;

static BOOL WINAPI CtrlHandler( DWORD t ) {
  f_abort = 1;
  SetEvent( ev_abort );
  // closing the console ends the process when this returns: let main clean up first
  if( (t==CTRL_CLOSE_EVENT) || (t==CTRL_LOGOFF_EVENT) || (t==CTRL_SHUTDOWN_EVENT) ) WaitForSingleObject( ev_done, 4500 );
  return TRUE;
}

static const char usage[] =
"mp4pack - lossless compression of mp4/mkv/avi files with avip, h264p, m4vpack,\n"
"aacpack, ac3pack, mp3pack, oggcomp, ttfcomp and tangelo, from mp4pack's own\n"
"directory: the best build of each for this CPU, <tool>_<target>" EXE " by -march\n"
"target names (h264p_haswell" EXE ", h264p_x86-64-v3" EXE ", h264p_znver4" EXE "), or\n"
"<tool>" EXE "\n"
"usage: mp4pack c [options] input output   compress\n"
"       mp4pack d [options] input output   decompress\n"
"input: a file, or @list: a text file with a file on each line (UTF-8, or\n"
"  UTF-16 with its BOM, or the ANSI code page; names relative to the current\n"
"  directory), done one after another (one that fails is left, the rest go on)\n"
"output: a file, or a directory (ending with \\ or /, or one that is there),\n"
"  made if needed, for an output of each file there: c x.mkv -> x.mkv.mpk,\n"
"  d x.mkv.mpk -> x.mkv. c of a list to a file: one archive of them all; d\n"
"  of it (an archive of more than one file): the files under the names they\n"
"  had, in the directory output (made if needed)\n"
"options:\n"
"  -t T  threads of the video packer (h264p, m4vpack), the audio packer\n"
"        (aacpack, ac3pack, mp3pack, oggcomp), ttfcomp and tangelo (the other\n"
"        files), in that order; joined by + they run at once, by , one after\n"
"        another: -t 3+3+1+1 (all at once), -t 3,3,1,1 (in turn), -t 3+3,1,1.\n"
"        Missing ones: audio as video, after it; ttfcomp and tangelo 1, both\n"
"        at once, after the rest (-t 4 = 4,4,1+1; -t 3+3 = 3+3,1+1). N% is\n"
"        that share of the threads there are (all, or the free ones: -f),\n"
"        rounded up; a -N at the end leaves N of them out first:\n"
"        -t 50%+50%-2 = 3+3,1+1 with 8 threads. Alone, -N works on the\n"
"        default, 100%,100% (= N,N,1+1: all the threads for the video, then\n"
"        for the audio).\n"
"        0: no packer for that group, its files go to tangelo (for tangelo\n"
"        itself: they are stored as they are)\n"
"  -c C  part sizes, in the same order: MB, or KB with k (e.g. -c 4,512k);\n"
"        0 or missing: a part for each thread, a stream's size over its\n"
"        group's threads (1MB at least), no bigger than the packer's share of\n"
"        the free memory (h264p, m4vpack and ac3pack take about 2 times a\n"
"        part, mp3pack 10). They get it as their -c. aacpack and oggcomp get\n"
"        it only from here (else their own parts: aacpack's of 1MB, oggcomp's\n"
"        of 64KB to 4MB), on as many threads as a stream has parts. With more\n"
"        than one thread, other files bigger than it are cut into pieces, an\n"
"        archive for each. The fonts make one solid archive.\n"
"  -l L  compression levels, in the same order, joined by , (or +); missing\n"
"        or empty: the tool's default. Video and audio: 1 to 9, as the builds\n"
"        have them (now h264p, m4vpack and aacpack 9 7 5 3 1, oggcomp 9 7 3 1,\n"
"        ac3pack 9 5 1; 9 is the strongest and the default); tangelo: 1 to 9\n"
"        (default 9; its model takes 17 to 1164MB). ttfcomp and mp3pack have\n"
"        none. E.g. -l 5,9,,7\n"
"  -f    count the threads free now (the CPUs' idle share over a second)\n"
"  -i T  use only tool builds whose code runs on the -march target T as well\n"
"        (haswell, x86-64-v3 ...; default: any that runs on this CPU)\n"
#ifdef _WIN32
"  -w D  make the work directory in D (default: %TEMP%)\n"
#else
"  -w D  make the work directory in D (default: $TMPDIR, or /tmp)\n"
#endif
"  -k    keep the work directory\n"
"  -v    show the tools' command lines and output\n"
"  -verify  c, then d of the archive into a temporary file beside the work\n"
"        directory, checked as d checks (each stream's and the whole CRC)\n";

int wmain( int argc, wchar_t** argv ) {
  Init();
  int mode;
  vector<wstring> io;
  string seen, why;
  if( ParseArgs( argc, argv, 0, mode, io, seen, why ) || (io.size()!=2) ) {
    if( why.size() ) Msg( why ); else Put( usage );
    return 1;
  }
  f_dec = (mode=='d');
  ev_done = CreateEventW( 0, TRUE, FALSE, 0 );
  SetConsoleCtrlHandler( CtrlHandler, TRUE );
  int r = Run( io[0], io[1] );
  SetEvent( ev_done );
  return r;
}
