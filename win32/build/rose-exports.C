// Writes the export list (a module definition file) of rose.dll, the ROSE library of the Windows
// build, from the symbols of librose.a (read with nm).  A Windows DLL cannot export more than
// 65535 names, and librose defines more than 100000, so only ROSE's API is exported.  Not
// exported:
//   - the EDG front end (namespace edg), and C functions and variables (the C and C++ run-time
//     libraries are linked into the DLL and into each program separately)
//   - instances of templates of the standard library and of Boost (programs instantiate their
//     own), and of ROSE's sg:: dispatch templates
//   - the AST file I/O (the *StorageClass classes) and the memory pool internals of the IR
//     classes, which only the library uses
//   - the names of type_info objects (the type_info objects are exported)
//
// usage: rose-exports <nm> <librose.a> <rose.def>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <map>
#include <set>
#include <string>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace {

bool startsWith(const std::string& s, const char* p) { return s.compare(0, std::strlen(p), p) == 0; }

// Whether a mangled name is in namespace std (_ZSt..., _ZNSt..., _ZNKSt..., the abbreviations Sa,
// Ss, Si, So, Sd, and the vtables and type_info objects of such classes)
bool isStd(const std::string& m) {
  std::size_t i = 2;
  if (startsWith(m, "_ZT") && m.size() > 3 && std::strchr("VISTH", m[3]) != nullptr) {
    i = 4;
    if (i < m.size() && m[i] == 'N') ++i;
  } else if (i < m.size() && m[i] == 'N') {
    ++i;
    while (i < m.size() && (m[i] == 'r' || m[i] == 'V' || m[i] == 'K')) ++i;
  }
  if (i + 1 >= m.size() || m[i] != 'S') return false;
  return std::strchr("taisod", m[i + 1]) != nullptr;
}

bool inNamespace(const std::string& m, const char* encoded) {
  for (const char* p : {"_ZN", "_ZNK", "_ZTVN", "_ZTIN", "_ZTSN", "_ZTTN"}) {
    if (startsWith(m, (std::string(p) + encoded).c_str())) return true;
  }
  return false;
}

const std::set<std::string> internalMembers = {
    "getPointerFromGlobalIndex", "resetValidFreepointers", "extendMemoryPoolForFileIO",
    "getNumberOfValidNodesAndSetGlobalIndexInFreepointer", "getNumberOfLastValidPointer",
    "initializeStorageClassArray", "deleteMemoryPool", "clearMemoryPool",
    "checkDataMemberPointersIfInMemoryPool", "processDataMemberReferenceToPointers",
    "getNodeByNodeIdInternal", "getNodeIdStringInternal", "returnDataMemberPointers"};

// The member name of "Sg...::name" at the end of a demangled name (without its parameters)
std::string irMember(const std::string& name) {
  std::size_t colons = name.rfind("::");
  if (colons == std::string::npos) return "";
  std::size_t b = colons;
  while (b > 0 && (std::isalnum((unsigned char)name[b - 1]) || name[b - 1] == '_')) --b;
  if (name.compare(b, 2, "Sg") != 0) return "";
  std::string member = name.substr(colons + 2);
  if (!member.empty() && member[0] == '~') member = member.substr(1);
  for (char c : member) {
    if (!(std::isalnum((unsigned char)c) || c == '_')) return "";
  }
  return member;
}

bool exported(const std::string& m, const std::string& d) {
  if (!startsWith(m, "_Z")) return false;  // C
  if (isStd(m) || inNamespace(m, "9__gnu_cxx") || startsWith(m, "_ZN10__cxxabiv1")) return false;
  if (inNamespace(m, "3edg")) return false;  // EDG
  if (startsWith(d, "typeinfo name for ")) return false;
  if (d.find("StorageClass") != std::string::npos || d.find("ReferenceToPointerHandler") != std::string::npos) {
    return false;
  }
  std::string name = d.substr(0, d.find('('));  // without the parameters
  bool tmpl = name.find('<') != std::string::npos;
  if (tmpl && name.find("boost::") != std::string::npos) return false;
  if (tmpl && (startsWith(name, "sg::") || name.find(" sg::") != std::string::npos)) return false;
  if (internalMembers.count(irMember(name))) return false;
  return true;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 4) {
    std::fprintf(stderr, "usage: rose-exports <nm> <librose.a> <rose.def>\n");
    return 2;
  }
#ifdef _WIN32
  // cmd.exe removes the outer quotes of the command
  std::string command = std::string("\"\"") + argv[1] + "\" -g --defined-only \"" + argv[2] + "\"\"";
#else
  std::string command = std::string("'") + argv[1] + "' -g --defined-only '" + argv[2] + "'";
#endif
  FILE* p = popen(command.c_str(), "r");
  if (p == nullptr) {
    std::perror(argv[1]);
    return 1;
  }
  // name -> data (not code)
  std::map<std::string, bool> symbols;
  char line[65536];
  while (std::fgets(line, sizeof line, p) != nullptr) {
    // "address type name" (or "type name")
    char* tok[3];
    int n = 0;
    for (char* t = std::strtok(line, " \t\r\n"); t != nullptr && n < 3; t = std::strtok(nullptr, " \t\r\n")) {
      tok[n++] = t;
    }
    if (n < 2) continue;
    const char* type = tok[n - 2];
    if (std::strlen(type) != 1 || std::strchr("TDBR", type[0]) == nullptr) continue;
    symbols[tok[n - 1]] = type[0] != 'T';
  }
  if (pclose(p) != 0 || symbols.empty()) {
    std::fprintf(stderr, "rose-exports: %s failed\n", argv[1]);
    return 1;
  }
  std::string out = "LIBRARY rose.dll\nEXPORTS\n";
  std::size_t count = 0;
  for (const auto& s : symbols) {
    int status = 0;
    char* dem = abi::__cxa_demangle(s.first.c_str(), nullptr, nullptr, &status);
    std::string d = status == 0 && dem != nullptr ? dem : s.first;
    std::free(dem);
    if (!exported(s.first, d)) continue;
    out += "  " + s.first + (s.second ? " DATA\n" : "\n");
    ++count;
  }
  if (count > 65535) {
    std::fprintf(stderr, "rose-exports: %zu names to export, more than the 65535 a DLL can export\n", count);
    return 1;
  }
  FILE* f = std::fopen(argv[3], "wb");
  if (f == nullptr || std::fwrite(out.data(), 1, out.size(), f) != out.size() || std::fclose(f) != 0) {
    std::perror(argv[3]);
    return 1;
  }
  return 0;
}
