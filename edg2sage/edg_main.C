// edg_main(): ROSE's entry point into the C/C++ front end, and the other
// symbols librose expects from its EDG library.
#include "edg2sage.h"
#include "fixupTypeReferences.h"
#include "rose_paths.h"
#include "RoseRefactorImpl.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

using namespace edg2sage;

namespace edg {
// The EDG front end, built as a library (see EDG_MAIN in edgconfig/edg_config.h).
extern int edg2sage_cfe_main(int argc, char* argv[]);
}

namespace edg2sage {
Sawyer::Message::Facility mlog;
Translator* translator = nullptr;
static SgSourceFile* currentSourceFile = nullptr;
static int translationErrors = 0;
static bool backEndCalled = false;
}

// --------------------------------------------------------------------------------------
// Symbols referenced by librose (historically provided by ROSE's EDG library)
// --------------------------------------------------------------------------------------
namespace EDG_ROSE_Translation {
// Maps file names to the SgIncludeFile nodes describing the #include tree.
std::map<std::string, SgIncludeFile*> edg_include_file_map;
// Suppresses the detection of transformations in SageBuilder (used by outlining).
bool suppress_detection_of_transformations = false;

void initDiagnostics() {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    Rose::Diagnostics::initAndRegister(&edg2sage::mlog, "EDG_ROSE_Translation");
    edg2sage::mlog.comment("translating the EDG IL to the Sage III AST");
    // Constructs that cannot be translated are reported as warnings.
    edg2sage::mlog[Sawyer::Message::WARN].enable();
    if (std::getenv("EDG2SAGE_DEBUG") != nullptr) {
      edg2sage::mlog[Sawyer::Message::INFO].enable();
      edg2sage::mlog[Sawyer::Message::TRACE].enable();  // constructs skipped in system headers
#pragma push_macro("DEBUG")
#undef DEBUG  // EDG configuration macro
      edg2sage::mlog[Sawyer::Message::DEBUG].enable();
#pragma pop_macro("DEBUG")
    }
  }
}

void clear_global_caches() {}
}  // namespace EDG_ROSE_Translation

// Post-processing hook used by ROSE's AST post-processing; this translator does
// not create type placeholders that would need fixing up.
void FixupTypeReferencesOnMemoryPool::visit(SgNode*) {}

// --------------------------------------------------------------------------------------
// The EDG back end: called by the EDG front end once the IL of the translation
// unit is complete (and only if there were no errors).
// --------------------------------------------------------------------------------------
void back_end(void) {
  ROSE_ASSERT(currentSourceFile != nullptr);
  backEndCalled = true;
  Translator t(currentSourceFile);
  translator = &t;
  try {
    if (xrefBuildsAst()) t.translate();
  } catch (const Unsupported& u) {
    mlog[Sawyer::Message::ERROR] << "unsupported construct: " << u.what << "\n";
    translationErrors++;
  }
  xrefCollect();
  translator = nullptr;
}

namespace {

// Directory with EDG's run-time configuration (lib/predefined_macros.txt)
std::string edgBaseDirectory() {
  if (const char* env = std::getenv("ROSE_EDG_BASE")) return env;
#ifdef _WIN32
  // The Windows build is relocatable (see rose_paths.C)
  return ROSE_AUTOMAKE_PREFIX + "/edg-base";
#elif defined(EDG2SAGE_EDG_BASE)
  return EDG2SAGE_EDG_BASE;
#else
  return "";
#endif
}

bool startsWith(const std::string& s, const char* prefix) { return s.compare(0, std::strlen(prefix), prefix) == 0; }

// Converts the command line built by ROSE (SgFile::build_EDG_CommandLine) into
// one for the open-source EDG front end (for one of the runs of xrefRuns()).
std::vector<std::string> edgCommandLine(int argc, char* argv[], int run) {
  std::vector<std::string> out;
  out.push_back("edg2sage");
  std::string base = edgBaseDirectory();
  if (!base.empty()) {
    out.push_back("--edg_base");
    out.push_back(base);
  }
  // Parsing as Visual C++ does (RoseRefactor::setMicrosoftMode): its data model and class layout
  // (EDG's win64 target configuration), language and predefined macros (with those that cl.exe
  // defines with /EHsc and EDG does not), and its headers instead of those of GCC
  bool msvc = RoseRefactor::microsoftMode();
  if (msvc) {
    int build = 0;
    int version = RoseRefactor::impl::microsoftVersion(&build);
    out.insert(out.end(), {"--target", "win64", "--microsoft", "--microsoft_version", std::to_string(version)});
    if (build > 0) out.insert(out.end(), {"--microsoft_build_number", std::to_string(build)});
    for (const std::string& d : RoseRefactor::impl::microsoftIncludeDirs()) out.insert(out.end(), {"--sys_include", d});
    out.insert(out.end(), {"-D_MT=1", "-D_CPPUNWIND=1"});
  }
  // Options of refactoring tools (RoseRefactor.h), and the cross-reference listing
  bool cplusplus = true;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--gcc") == 0 || std::strcmp(argv[i], "--c") == 0 ||
        std::strcmp(argv[i], "-DROSE_LANGUAGE_MODE=0") == 0) {
      cplusplus = false;
    }
  }
  std::size_t toolOptions = out.size();
  xrefOptions(out, run, cplusplus);
  // Visual C++ parses the bodies of templates where they are defined only with /permissive-
  // (two-phase name lookup)
  if (msvc && std::find(out.begin() + toolOptions, out.end(), "--no_defer_parse_function_templates") != out.end()) {
    out.push_back("--no_ms_permissive");
  }
  // EDG2SAGE_EDG_OPTIONS: more EDG options, separated by spaces (for debugging)
  if (const char* extra = std::getenv("EDG2SAGE_EDG_OPTIONS")) {
    std::istringstream words(extra);
    for (std::string w; words >> w;) out.push_back(w);
  }
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    // ROSE defines __STRICT_ANSI__ as 0 for GNU modes and expects its own preinclude header to
    // undo that; for EDG's native GNU emulation the macro must simply not be defined.
    if (a == "-D__STRICT_ANSI__=0") continue;
    if (msvc) {
      // Not GCC: no GNU mode, GCC's predefined macros or system include directories
      if (a == "--gnu_version" || a == "--sys_include") {
        ++i;
        continue;
      }
      if (startsWith(a, "-D__GNUG__") || startsWith(a, "-D__GNUC__") || startsWith(a, "-D__GNUC_MINOR__") ||
          startsWith(a, "-D__GNUC_PATCHLEVEL__")) {
        continue;
      }
      if (a == "--g++") a = "--c++";
      if (a == "--gcc") a = "--c";
      // The language standard, as Visual C++ selects it (/std:c++14 is the oldest and the default;
      // /std:c++20 since 19.29)
      if (a == "--c++98" || a == "--c++03" || a == "--c++11" || a == "--c++14") a = "--ms_c++14";
      if (a == "--c++17") a = "--ms_c++17";
      if (a == "--c++20") a = RoseRefactor::impl::microsoftVersion(nullptr) >= 1929 ? "--ms_c++20" : "--ms_c++latest";
      if (a == "--c++23" || a == "--c++26") a = "--ms_c++latest";
      if (a == "--c89" || a == "--c99") continue;
      if (a == "--c11") a = "--ms_c11";
      if (a == "--c17" || a == "--c18" || a == "--c23") a = "--ms_c17";
    }
    // EDG only needs declarations of the template specializations that are used.
    if (a == "--auto_instantiation" || a == "-tused" || a == "-tlocal" || a == "-tall") continue;
    out.push_back(a);
  }
  return out;
}

}  // namespace

// --------------------------------------------------------------------------------------
// ROSE's entry point: parse one C or C++ source file and build its AST in sageFile.
// --------------------------------------------------------------------------------------
// Refactoring tools can give alternative options (RoseRefactor::setAlternativeFrontEndOptions):
// if the front end rejects the file, it runs again with the next options.  The diagnostics of a
// run that may be followed by another are written to a file, and shown if the run succeeds.
int edg_main(int argc, char* argv[], SgSourceFile& sageFile) {
  EDG_ROSE_Translation::initDiagnostics();
  int runs = xrefRuns();
  int status = 1;
  for (int run = 0; run < runs; ++run) {
    std::vector<std::string> args = edgCommandLine(argc, argv, run);
    std::string diagnostics;
    if (run + 1 < runs) {
      diagnostics = RoseRefactor::impl::temporaryFile("rose-diagnostics");
      if (!diagnostics.empty()) {
        RoseRefactor::impl::removeAtExit(diagnostics);
        args.insert(args.begin() + 1, {"--error_output", diagnostics});
      }
    }
    if (SgProject::get_verbose() > 0) {
      std::string line;
      for (const std::string& a : args) line += a + " ";
      mlog[Sawyer::Message::INFO] << "EDG command line: " << line << "\n";
    }
    std::vector<char*> cargs;
    for (std::string& a : args) cargs.push_back(&a[0]);
    cargs.push_back(nullptr);

    currentSourceFile = &sageFile;
    translationErrors = 0;
    backEndCalled = false;
    status = edg::edg2sage_cfe_main((int)args.size(), cargs.data());
    currentSourceFile = nullptr;

    if (!backEndCalled && status == 0) {
      // EDG reported success but did not call the back end
      status = 1;
    }
    // Errors of the translation into the AST are not a reason to try other options
    bool accepted = backEndCalled;
    if (translationErrors > 0) status = 1;
    if (accepted || run + 1 == runs) {
      if (!diagnostics.empty()) {
        std::ifstream in(diagnostics.c_str());
        std::ostringstream text;
        text << in.rdbuf();
        std::fputs(text.str().c_str(), stderr);
        std::fflush(stderr);
      }
      xrefRunAccepted(run);
      break;
    }
  }
  return status;
}
