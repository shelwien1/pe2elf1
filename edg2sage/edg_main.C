// edg_main(): ROSE's entry point into the C/C++ front end, and the other
// symbols librose expects from its EDG library.
#include "edg2sage.h"
#include "fixupTypeReferences.h"
#include "rose_paths.h"

#include <cstdlib>
#include <cstring>

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
    t.translate();
  } catch (const Unsupported& u) {
    mlog[Sawyer::Message::ERROR] << "unsupported construct: " << u.what << "\n";
    translationErrors++;
  }
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
// one for the open-source EDG front end.
std::vector<std::string> edgCommandLine(int argc, char* argv[]) {
  std::vector<std::string> out;
  out.push_back("edg2sage");
  std::string base = edgBaseDirectory();
  if (!base.empty()) {
    out.push_back("--edg_base");
    out.push_back(base);
  }
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    // ROSE defines __STRICT_ANSI__ as 0 for GNU modes and expects its own preinclude header to
    // undo that; for EDG's native GNU emulation the macro must simply not be defined.
    if (a == "-D__STRICT_ANSI__=0") continue;
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
int edg_main(int argc, char* argv[], SgSourceFile& sageFile) {
  EDG_ROSE_Translation::initDiagnostics();
  std::vector<std::string> args = edgCommandLine(argc, argv);
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
  int status = edg::edg2sage_cfe_main((int)args.size(), cargs.data());
  currentSourceFile = nullptr;

  if (!backEndCalled && status == 0) {
    // EDG reported success but did not call the back end
    status = 1;
  }
  if (translationErrors > 0) status = 1;
  return status;
}
