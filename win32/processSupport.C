// Windows (MinGW-w64) replacement for rose/src/util/processSupport.C, which runs subprocesses
// with fork/exec.  Here they are started with CreateProcess, with the arguments quoted the way
// the Microsoft C run-time library splits a command line.  The assertion handling part is the
// same as in the original.
#include "rosePublicConfig.h"
#include "processSupport.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <fcntl.h>
#include <io.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

using namespace std;

namespace {

// Quotes one argument for CommandLineToArgvW and the C run-time's argv parsing
string quoteArgument(const string& arg) {
  if (!arg.empty() && arg.find_first_of(" \t\n\v\"") == string::npos) return arg;
  string out = "\"";
  size_t backslashes = 0;
  for (char c : arg) {
    if (c == '\\') {
      ++backslashes;
    } else if (c == '"') {
      out.append(2 * backslashes + 1, '\\');
      backslashes = 0;
    } else {
      backslashes = 0;
    }
    out += c;
  }
  out.append(backslashes, '\\');  // backslashes before the closing quote are doubled
  out += '"';
  return out;
}

string commandLine(const vector<string>& argv) {
  string line;
  for (const string& arg : argv) {
    if (!line.empty()) line += ' ';
    line += quoteArgument(arg);
  }
  return line;
}

// Starts argv[0] (searched for in the PATH, ".exe" appended if there is no extension).
// Returns the process handle, or NULL with an error message.
HANDLE startProcess(const vector<string>& argv, STARTUPINFOA& si) {
  string line = commandLine(argv);
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof pi);
  if (!CreateProcessA(NULL, &line[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
    fprintf(stderr, "error: cannot run \"%s\" (Windows error %lu)\n", argv[0].c_str(), (unsigned long)GetLastError());
    return NULL;
  }
  CloseHandle(pi.hThread);
  return pi.hProcess;
}

int waitForProcess(HANDLE process) {
  WaitForSingleObject(process, INFINITE);
  DWORD exitCode = 1;
  GetExitCodeProcess(process, &exitCode);
  CloseHandle(process);
  return (int)exitCode;
}

// The process started by popenReadFromVector (only one at a time)
HANDLE pipeProcess = NULL;

}  // namespace

int systemFromVector(const vector<string>& argv) {
  assert(!argv.empty());
  fflush(stdout);
  fflush(stderr);
  STARTUPINFOA si;
  ZeroMemory(&si, sizeof si);
  si.cb = sizeof si;
  HANDLE process = startProcess(argv, si);
  return process ? waitForProcess(process) : 1;
}

FILE* popenReadFromVector(const vector<string>& argv) {
  assert(!argv.empty());
  SECURITY_ATTRIBUTES sa;
  ZeroMemory(&sa, sizeof sa);
  sa.nLength = sizeof sa;
  sa.bInheritHandle = TRUE;
  HANDLE readEnd, writeEnd;
  if (!CreatePipe(&readEnd, &writeEnd, &sa, 0)) {
    fprintf(stderr, "error: CreatePipe failed (Windows error %lu)\n", (unsigned long)GetLastError());
    abort();
  }
  SetHandleInformation(readEnd, HANDLE_FLAG_INHERIT, 0);
  STARTUPINFOA si;
  ZeroMemory(&si, sizeof si);
  si.cb = sizeof si;
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = writeEnd;
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  fflush(stdout);
  fflush(stderr);
  pipeProcess = startProcess(argv, si);
  CloseHandle(writeEnd);
  if (pipeProcess == NULL) abort();
  int fd = _open_osfhandle((intptr_t)readEnd, _O_RDONLY | _O_TEXT);
  return fd < 0 ? NULL : _fdopen(fd, "r");
}

int pcloseFromVector(FILE* f) {
  fclose(f);
  int status = 0;
  if (pipeProcess != NULL) {
    status = waitForProcess(pipeProcess);
    pipeProcess = NULL;
  }
  return status;
}

namespace Rose {

ROSE_UTIL_API void
abortOnFailedAssertion(const char */*mesg*/, const char */*expr*/, const std::string &/*note*/, const char */*fileName*/,
                       unsigned /*lineNumber*/, const char */*functionName*/) {
    abort();
}

ROSE_UTIL_API void
exitOnFailedAssertion(const char */*mesg*/, const char */*expr*/, const std::string &/*note*/, const char */*fileName*/,
                      unsigned /*lineNumber*/, const char */*functionName*/) {
    exit(1);
}

ROSE_UTIL_API void
throwOnFailedAssertion(const char *mesg, const char *expr, const std::string &note, const char *fileName,
                       unsigned lineNumber, const char *functionName) {
    throw FailedAssertion(mesg, expr, note, fileName, lineNumber, functionName);
}

ROSE_UTIL_API void
failedAssertionBehavior(Sawyer::Assert::AssertFailureHandler handler) {
    if (handler) {
        Sawyer::Assert::assertFailureHandler = handler;
    } else {
#if !defined(ROSE_ASSERTION_BEHAVIOR)
#           error "ROSE_ASSERTION_BEHAVIOR should have been defined by the ROSE configuration system"
#elif ROSE_ASSERTION_BEHAVIOR == ROSE_ASSERTION_ABORT
            Sawyer::Assert::assertFailureHandler = abortOnFailedAssertion;
#elif ROSE_ASSERTION_BEHAVIOR == ROSE_ASSERTION_EXIT
            Sawyer::Assert::assertFailureHandler = exitOnFailedAssertion;
#elif ROSE_ASSERTION_BEHAVIOR == ROSE_ASSERTION_THROW
            Sawyer::Assert::assertFailureHandler = throwOnFailedAssertion;
#else
#           error "ROSE_ASSERTION_BEHAVIOR has an invalid value"
#endif
    }
}

Sawyer::Assert::AssertFailureHandler
failedAssertionBehavior() {
    return Sawyer::Assert::assertFailureHandler;
}

} // namespace
