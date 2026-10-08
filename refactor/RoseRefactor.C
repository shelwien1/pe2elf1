// RoseRefactor: the parts that do not depend on the EDG front end (see RoseRefactor.h; the
// cross-reference listing is read by edg2sage/xref.C).
#include "RoseRefactorImpl.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace RoseRefactor {

// ---------------------------------------------------------------------------------------------
// Positions, kinds
// ---------------------------------------------------------------------------------------------

bool Position::operator<(const Position& o) const {
  if (file != o.file) return file < o.file;
  if (line != o.line) return line < o.line;
  return column < o.column;
}

std::string Position::str() const {
  return file + ":" + std::to_string(line) + ":" + std::to_string(column);
}

const char* kindName(Kind k) {
  switch (k) {
    case Kind::Namespace: return "namespace";
    case Kind::Class: return "class";
    case Kind::Struct: return "struct";
    case Kind::Union: return "union";
    case Kind::Enum: return "enum";
    case Kind::Enumerator: return "enumerator";
    case Kind::Typedef: return "typedef";
    case Kind::Variable: return "variable";
    case Kind::Parameter: return "parameter";
    case Kind::Field: return "data member";
    case Kind::StaticDataMember: return "static data member";
    case Kind::Function: return "function";
    case Kind::MemberFunction: return "member function";
    case Kind::Constructor: return "constructor";
    case Kind::Destructor: return "destructor";
    case Kind::ClassTemplate: return "class template";
    case Kind::FunctionTemplate: return "function template";
    case Kind::VariableTemplate: return "variable template";
    case Kind::Concept: return "concept";
    case Kind::TemplateParameter: return "template parameter";
    case Kind::Label: return "label";
    case Kind::Macro: return "macro";
    case Kind::Other: break;
  }
  return "entity";
}

const char* accessName(Access a) {
  switch (a) {
    case Access::Public: return "public";
    case Access::Protected: return "protected";
    case Access::Private: return "private";
    case Access::None: break;
  }
  return "";
}

bool Entity::isMember() const {
  return access != Access::None || kind == Kind::Field || kind == Kind::StaticDataMember ||
         kind == Kind::MemberFunction || kind == Kind::Constructor || kind == Kind::Destructor;
}

bool Entity::isType() const {
  switch (kind) {
    case Kind::Class:
    case Kind::Struct:
    case Kind::Union:
    case Kind::Enum:
    case Kind::Typedef:
    case Kind::ClassTemplate:
    case Kind::TemplateParameter:
      return true;
    default:
      return false;
  }
}

Position Entity::declaration() const {
  const Reference* first = nullptr;
  for (const Reference& r : references) {
    if (r.isDeclaration() && !r.inSystemHeader && (first == nullptr || r.order < first->order)) first = &r;
  }
  return first != nullptr ? first->pos : Position();
}

bool Entity::isDeclared() const {
  for (const Reference& r : references) {
    if (r.isDeclaration()) return true;
  }
  return false;
}

bool Entity::declaredInSystemHeader() const {
  bool any = false;
  for (const Reference& r : references) {
    if (!r.isDeclaration()) continue;
    if (!r.inSystemHeader) return false;
    any = true;
  }
  return any;
}

// ---------------------------------------------------------------------------------------------
// Cross-references
// ---------------------------------------------------------------------------------------------

CrossReferences::CrossReferences() = default;
CrossReferences::~CrossReferences() = default;

const Entity* CrossReferences::entity(EntityId id) const {
  auto it = entities_.find(id);
  return it == entities_.end() ? nullptr : &it->second;
}

std::vector<const Entity*> CrossReferences::named(const std::string& name) const {
  std::vector<const Entity*> v;
  auto r = byName_.equal_range(name);
  for (auto it = r.first; it != r.second; ++it) v.push_back(entity(it->second));
  return v;
}

std::vector<const Entity*> CrossReferences::at(const Position& pos) const {
  std::vector<const Entity*> v;
  auto r = byPosition_.equal_range(pos);
  for (auto it = r.first; it != r.second; ++it) {
    const Entity* e = entity(it->second);
    if (std::find(v.begin(), v.end(), e) == v.end()) v.push_back(e);
  }
  return v;
}

std::vector<const Entity*> CrossReferences::sameDeclaration(const Entity& e) const {
  // Entities connected through shared declaration positions
  std::vector<const Entity*> result{&e};
  std::set<EntityId> seen{e.id};
  for (std::size_t i = 0; i < result.size(); ++i) {
    for (const Reference& r : result[i]->references) {
      if (!r.isDeclaration()) continue;
      auto range = byDeclaration_.equal_range(r.pos);
      for (auto it = range.first; it != range.second; ++it) {
        if (seen.insert(it->second).second) result.push_back(entity(it->second));
      }
    }
  }
  return result;
}

bool CrossReferences::isBaseOf(EntityId b, EntityId d) const {
  return directBaseLeadingTo(d, b) != nullptr;
}

const BaseClass* CrossReferences::directBaseLeadingTo(EntityId d, EntityId b) const {
  const Entity* de = entity(d);
  if (de == nullptr) return nullptr;
  for (const BaseClass& base : de->bases) {
    if (base.entity == b) return &base;
  }
  for (const BaseClass& base : de->bases) {
    std::set<EntityId> visited;
    std::vector<EntityId> work{base.entity};
    while (!work.empty()) {
      EntityId c = work.back();
      work.pop_back();
      if (!visited.insert(c).second) continue;
      const Entity* ce = entity(c);
      if (ce == nullptr) continue;
      for (const BaseClass& bb : ce->bases) {
        if (bb.entity == b) return &base;
        work.push_back(bb.entity);
      }
    }
  }
  return nullptr;
}

Entity& CrossReferences::add(EntityId id) {
  Entity& e = entities_[id];
  e.id = id;
  return e;
}

void CrossReferences::finish() {
  byName_.clear();
  byPosition_.clear();
  byDeclaration_.clear();
  for (auto& kv : entities_) {
    Entity& e = kv.second;
    std::stable_sort(e.references.begin(), e.references.end(),
                     [](const Reference& a, const Reference& b) { return a.pos < b.pos; });
    // The listing repeats some references
    e.references.erase(std::unique(e.references.begin(), e.references.end(),
                                   [](const Reference& a, const Reference& b) {
                                     return a.pos == b.pos && a.code == b.code;
                                   }),
                       e.references.end());
    byName_.emplace(e.name, e.id);
    for (const Reference& r : e.references) {
      byPosition_.emplace(r.pos, e.id);
      if (r.isDeclaration()) byDeclaration_.emplace(r.pos, e.id);
    }
  }
}

void CrossReferences::clear() {
  entities_.clear();
  byName_.clear();
  byPosition_.clear();
  byDeclaration_.clear();
}

namespace {
bool recordingFlag = false;
bool buildAstFlag = true;
std::vector<std::string> options;
std::vector<std::vector<std::string>> alternativeOptions;
int optionsUsed = 0;
bool msMode = false;
std::vector<std::string> msIncludeDirs;
int msVersion = 0;
CrossReferences* currentXref = nullptr;
std::vector<std::string>* filesToRemove = nullptr;

void removeFiles() {
  if (filesToRemove == nullptr) return;
  for (const std::string& f : *filesToRemove) std::remove(f.c_str());
}
}  // namespace

void recordCrossReferences(bool on) { recordingFlag = on; }

void setFrontEndOptions(const std::vector<std::string>& opts) { options = opts; }

void setAlternativeFrontEndOptions(const std::vector<std::vector<std::string>>& alternatives) {
  alternativeOptions = alternatives;
}

int frontEndOptionsUsed() { return optionsUsed; }

void setMicrosoftMode(const std::vector<std::string>& includeDirs, int version) {
  msMode = true;
  msIncludeDirs = includeDirs;
  msVersion = version;
}

bool microsoftMode() { return msMode; }

namespace {
bool isDirectory(const std::string& path) {
#ifdef _WIN32
  DWORD a = GetFileAttributesA(path.c_str());
  return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
  struct stat st;
  return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}
}  // namespace

std::vector<std::string> microsoftIncludeDirs(const std::string& dir) {
  std::vector<std::string> dirs;
  if (dir.empty()) {
    const char* include = std::getenv("INCLUDE");
    std::string list = include != nullptr ? include : "";
    std::size_t start = 0;
    while (start <= list.size()) {
      std::size_t end = list.find(';', start);
      if (end == std::string::npos) end = list.size();
      std::string d = list.substr(start, end - start);
      if (!d.empty()) dirs.push_back(d);
      start = end + 1;
    }
    return dirs;
  }
#ifdef _WIN32
  const std::string sep = "\\";
#else
  const std::string sep = "/";
#endif
  std::string base = dir;
  while (base.size() > 1 && (base.back() == '/' || base.back() == '\\')) base.pop_back();
  for (const char* sub : {"include", "atlmfc/include", "ucrt/include", "sdk/include", "sdk/include/ucrt",
                          "sdk/include/um", "sdk/include/shared", "sdk/include/winrt"}) {
    std::string d = base + sep + sub;
#ifdef _WIN32
    for (char& c : d) if (c == '/') c = '\\';
#endif
    if (isDirectory(d)) dirs.push_back(d);
  }
  return dirs;
}

void buildAst(bool on) { buildAstFlag = on; }

const CrossReferences& crossReferences() { return impl::current(); }

namespace impl {

bool recording() { return recordingFlag; }

bool buildsAst() { return buildAstFlag; }

int frontEndRuns() { return 1 + (int)alternativeOptions.size(); }

const std::vector<std::string>& frontEndOptions(int run) {
  return run > 0 && run <= (int)alternativeOptions.size() ? alternativeOptions[run - 1] : options;
}

void setFrontEndOptionsUsed(int run) { optionsUsed = run; }

const std::vector<std::string>& microsoftIncludeDirs() { return msIncludeDirs; }

int microsoftVersion(int* buildNumber) {
  if (buildNumber != nullptr) *buildNumber = 0;
  if (msVersion != 0) return msVersion;
  // The version of the run-time library of the headers: _MSC_VER is 1900 + the minor version for
  // the versions 14.x (Visual Studio 2015 to 2022), and the build number is that of the compiler
  for (const std::string& d : msIncludeDirs) {
    std::ifstream in((d + "/crtversion.h").c_str());
    if (!in) continue;
    int major = 0, minor = -1, build = 0;
    for (std::string line; std::getline(in, line);) {
      std::istringstream words(line);
      std::string define, name;
      long value = 0;
      if (!(words >> define >> name >> value) || define != "#define") continue;
      if (name == "_VC_CRT_MAJOR_VERSION") major = (int)value;
      if (name == "_VC_CRT_MINOR_VERSION") minor = (int)value;
      if (name == "_VC_CRT_BUILD_VERSION") build = (int)value;
    }
    if (major == 14 && minor >= 0) {
      if (buildNumber != nullptr) *buildNumber = build;
      return 1900 + minor;
    }
  }
  return 1920;  // Visual Studio 2019 16.0
}

CrossReferences& current() {
  if (currentXref == nullptr) currentXref = new CrossReferences();
  return *currentXref;
}

std::string temporaryFile(const char* prefix) {
#ifdef _WIN32
  char dir[MAX_PATH + 1], name[MAX_PATH + 1];
  DWORD n = GetTempPathA(sizeof dir, dir);
  if (n == 0 || n > MAX_PATH) std::strcpy(dir, ".");
  if (GetTempFileNameA(dir, "rose", 0, name) == 0) return "";
  (void)prefix;
  return name;
#else
  const char* tmp = std::getenv("TMPDIR");
  std::string pattern = std::string(tmp != nullptr && *tmp ? tmp : "/tmp") + "/" + prefix + "-XXXXXX";
  std::vector<char> buf(pattern.begin(), pattern.end());
  buf.push_back('\0');
  int fd = mkstemp(buf.data());
  if (fd < 0) return "";
  close(fd);
  return buf.data();
#endif
}

void removeAtExit(const std::string& file) {
  if (filesToRemove == nullptr) {
    filesToRemove = new std::vector<std::string>();
    std::atexit(removeFiles);
  }
  filesToRemove->push_back(file);
}

}  // namespace impl

// ---------------------------------------------------------------------------------------------
// Source text
// ---------------------------------------------------------------------------------------------

namespace {

// Byte classes of a source file (a simple C/C++ lexer)
enum : char { CODE = 'c', SPACE = ' ', COMMENT = '/', STRING = '"', CHARLIT = '\'' };

bool isIdentStart(unsigned char c) { return std::isalpha(c) || c == '_' || c >= 0x80; }
bool isIdentChar(unsigned char c) { return std::isalnum(c) || c == '_' || c >= 0x80; }

std::vector<char> classify(const std::string& t) {
  std::vector<char> cls(t.size(), CODE);
  std::size_t i = 0, n = t.size();
  while (i < n) {
    char c = t[i];
    if (c == '/' && i + 1 < n && t[i + 1] == '/') {
      while (i < n && t[i] != '\n') {
        // A backslash at the end of the line continues the comment
        if (t[i] == '\\' && i + 1 < n && (t[i + 1] == '\n' || (t[i + 1] == '\r' && i + 2 < n && t[i + 2] == '\n'))) {
          cls[i++] = COMMENT;
        }
        cls[i++] = COMMENT;
      }
    } else if (c == '/' && i + 1 < n && t[i + 1] == '*') {
      std::size_t end = t.find("*/", i + 2);
      end = end == std::string::npos ? n : end + 2;
      for (; i < end; ++i) cls[i] = COMMENT;
    } else if (c == '"' && i > 0 && t[i - 1] == 'R' && (i < 2 || !isIdentChar((unsigned char)t[i - 2]) ||
                                                         t[i - 2] == 'u' || t[i - 2] == 'U' || t[i - 2] == 'L' ||
                                                         t[i - 2] == '8')) {
      // Raw string literal R"delim( ... )delim"
      std::size_t open = t.find('(', i);
      std::string delim = open == std::string::npos ? "" : t.substr(i + 1, open - i - 1);
      std::size_t end = open == std::string::npos ? n : t.find(")" + delim + "\"", open);
      end = end == std::string::npos ? n : end + delim.size() + 2;
      for (; i < end; ++i) cls[i] = STRING;
    } else if (c == '"' || (c == '\'' && !(i > 0 && std::isxdigit((unsigned char)t[i - 1]) &&
                                           i + 1 < n && std::isxdigit((unsigned char)t[i + 1]) &&
                                           cls[i - 1] == CODE && !isIdentStart((unsigned char)t[i - 1])))) {
      // String or character literal (a ' between hexadecimal digits of a number is a digit
      // separator, which the test above approximates)
      char q = c;
      cls[i++] = q == '"' ? STRING : CHARLIT;
      while (i < n && t[i] != q && t[i] != '\n') {
        if (t[i] == '\\' && i + 1 < n) cls[i++] = q == '"' ? STRING : CHARLIT;
        cls[i++] = q == '"' ? STRING : CHARLIT;
      }
      if (i < n && t[i] == q) cls[i++] = q == '"' ? STRING : CHARLIT;
    } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') {
      cls[i++] = SPACE;
    } else if (c == '\\' && i + 1 < n && (t[i + 1] == '\n' || t[i + 1] == '\r')) {
      cls[i++] = SPACE;  // line continuation
    } else {
      ++i;
    }
  }
  return cls;
}

std::map<std::string, std::vector<char>>& classes() {
  static std::map<std::string, std::vector<char>> m;
  return m;
}

}  // namespace

SourceText::SourceText(const std::string& file) : file_(file) {
  std::ifstream in(file.c_str(), std::ios::binary);
  if (!in) return;
  std::ostringstream ss;
  ss << in.rdbuf();
  text_ = ss.str();
  ok_ = true;
  lineStart_.push_back(0);
  for (std::size_t i = 0; i < text_.size(); ++i) {
    if (text_[i] == '\n') {
      if (i > 0 && text_[i - 1] == '\r') crlf_ = true;
      if (i + 1 < text_.size()) lineStart_.push_back(i + 1);
    }
  }
  classes()[file_] = classify(text_);
}

std::size_t SourceText::offset(int line, int column) const {
  if (line < 1 || line > (int)lineStart_.size() || column < 1) return std::string::npos;
  std::size_t o = lineStart_[line - 1];
  for (int c = 1; c < column; ++c) {
    if (o >= text_.size() || text_[o] == '\n') return std::string::npos;
    ++o;
    while (o < text_.size() && ((unsigned char)text_[o] & 0xC0) == 0x80) ++o;  // UTF-8 continuation
  }
  return o;
}

Position SourceText::position(std::size_t off) const {
  auto it = std::upper_bound(lineStart_.begin(), lineStart_.end(), off);
  int line = (int)(it - lineStart_.begin());
  std::size_t o = lineStart_[line - 1];
  int column = 1;
  while (o < off && o < text_.size()) {
    ++o;
    while (o < text_.size() && ((unsigned char)text_[o] & 0xC0) == 0x80) ++o;
    ++column;
  }
  return Position(file_, line, column);
}

std::string SourceText::line(int line) const {
  if (line < 1 || line > (int)lineStart_.size()) return "";
  std::size_t b = lineStart_[line - 1];
  std::size_t e = text_.find('\n', b);
  if (e == std::string::npos) e = text_.size();
  if (e > b && text_[e - 1] == '\r') --e;
  return text_.substr(b, e - b);
}

std::string SourceText::identifierAt(std::size_t off) const {
  if (off >= text_.size() || !isIdentStart((unsigned char)text_[off])) return "";
  if (off > 0 && isIdentChar((unsigned char)text_[off - 1])) return "";
  const std::vector<char>& cls = classes()[file_];
  if (cls[off] != CODE) return "";
  std::size_t e = off;
  while (e < text_.size() && isIdentChar((unsigned char)text_[e])) ++e;
  return text_.substr(off, e - off);
}

std::vector<std::size_t> SourceText::occurrences(const std::string& id) const {
  std::vector<std::size_t> out;
  if (id.empty()) return out;
  const std::vector<char>& cls = classes()[file_];
  for (std::size_t off = text_.find(id); off != std::string::npos; off = text_.find(id, off + 1)) {
    std::size_t end = off + id.size();
    if (cls[off] != CODE || (end < text_.size() && isIdentChar((unsigned char)text_[end]))) continue;
    // Not in the middle of an identifier or of a number (such as 1e5 or 0x1f)
    std::size_t b = off;
    while (b > 0 && isIdentChar((unsigned char)text_[b - 1])) --b;
    if (b == off) out.push_back(off);
  }
  return out;
}

std::size_t SourceText::skipSpace(std::size_t off) const {
  const std::vector<char>& cls = classes()[file_];
  while (off < text_.size() && (cls[off] == SPACE || cls[off] == COMMENT)) ++off;
  return off;
}

std::size_t SourceText::skipSpaceBackward(std::size_t off) const {
  const std::vector<char>& cls = classes()[file_];
  while (off > 0 && (cls[off - 1] == SPACE || cls[off - 1] == COMMENT)) --off;
  return off;
}

std::size_t SourceText::matching(std::size_t off) const {
  if (off >= text_.size()) return std::string::npos;
  const std::vector<char>& cls = classes()[file_];
  char open = text_[off];
  const char* opens = "([{<";
  const char* closes = ")]}>";
  const char* p = std::strchr(opens, open);
  bool forward = p != nullptr;
  if (!forward) p = std::strchr(closes, open);
  if (p == nullptr || open == '\0') return std::string::npos;
  std::size_t k = p - (forward ? opens : closes);
  char close = forward ? closes[k] : opens[k];
  bool angle = open == '<' || open == '>';
  int depth = 0;       // of the bracket kind
  int other = 0;       // of the other brackets (for angle brackets)
  for (std::size_t i = off;; forward ? ++i : --i) {
    if (i >= text_.size()) return std::string::npos;
    if (cls[i] == CODE) {
      char c = text_[i];
      if (angle) {
        if (std::strchr(forward ? "([{" : ")]}", c) && c != '\0') {
          ++other;
        } else if (std::strchr(forward ? ")]}" : "([{", c) && c != '\0') {
          if (--other < 0) return std::string::npos;
        } else if (other == 0) {
          if (c == ';' || (c == '{' && forward) || (c == '}' && !forward)) return std::string::npos;
          if (c == open && !(c == '>' && i > 0 && text_[i - 1] == '-')) ++depth;
          if (c == close && !(c == '>' && i > 0 && text_[i - 1] == '-')) {
            if (--depth == 0) return i;
          }
        }
      } else {
        if (c == open) ++depth;
        if (c == close && --depth == 0) return i;
      }
    }
    if (i == 0 && !forward) return std::string::npos;
  }
}

std::size_t SourceText::find(char c, std::size_t off) const {
  const std::vector<char>& cls = classes()[file_];
  int depth = 0;
  for (std::size_t i = off; i < text_.size(); ++i) {
    if (cls[i] != CODE) continue;
    char x = text_[i];
    if (depth == 0 && x == c) return i;
    if (x == '(' || x == '[' || x == '{') ++depth;
    if (x == ')' || x == ']' || x == '}') {
      if (--depth < 0) return std::string::npos;
    }
  }
  return std::string::npos;
}

std::string SourceText::indentation(int ln) const {
  std::string l = line(ln);
  std::size_t e = l.find_first_not_of(" \t");
  return e == std::string::npos ? l : l.substr(0, e);
}

SourceText& sourceText(const std::string& file) {
  static std::map<std::string, std::unique_ptr<SourceText>> texts;
  std::unique_ptr<SourceText>& t = texts[file];
  if (!t) t.reset(new SourceText(file));
  return *t;
}

std::size_t nameOffset(const CrossReferences& xr, const Position& pos, const std::string& name,
                       std::string* macro) {
  if (macro != nullptr) macro->clear();
  SourceText& st = sourceText(pos.file);
  std::size_t off = st.offset(pos);
  if (off == std::string::npos) return off;
  if (st.text()[off] == '~') off = st.skipSpace(off + 1);  // destructor
  std::string id = st.identifierAt(off);
  if (id == name) return off;
  // A qualified name: EDG records some references at the start of the name (a base class in a
  // constructor's initializer list, "ns::Base(...)")
  const std::string& t = st.text();
  for (std::size_t q = off; t.compare(q, 2, "::") == 0 || !st.identifierAt(q).empty();) {
    std::size_t after = q;
    if (t.compare(q, 2, "::") != 0) {
      after = st.skipSpace(q + st.identifierAt(q).size());
      if (after < t.size() && t[after] == '<') {  // template arguments
        std::size_t close = st.matching(after);
        if (close == std::string::npos) break;
        after = st.skipSpace(close + 1);
      }
      if (t.compare(after, 2, "::") != 0) break;
    }
    q = st.skipSpace(after + 2);
    if (st.identifierAt(q) == name) return q;
  }
  if (id.empty()) return std::string::npos;
  // A macro invocation?
  bool isMacro = false;
  for (const Entity* e : xr.at(pos)) {
    if (e->kind == Kind::Macro && e->name == id) isMacro = true;
  }
  if (!isMacro) return std::string::npos;
  std::size_t open = st.skipSpace(off + id.size());
  if (open < st.text().size() && st.text()[open] == '(') {
    std::size_t close = st.matching(open);
    std::size_t found = std::string::npos;
    int count = 0;
    for (std::size_t i = open + 1; close != std::string::npos && i < close; ++i) {
      std::string w = st.identifierAt(i);
      if (w.empty()) continue;
      if (w == name) {
        found = i;
        ++count;
      }
      i += w.size() - 1;
    }
    if (count == 1) return found;
  }
  if (macro != nullptr) *macro = id;
  return std::string::npos;
}

std::vector<Position> unresolvedOccurrences(const CrossReferences& xr, const std::string& name) {
  // The offsets of the name where a declared entity of that name (or a destructor of a class of
  // that name) is referenced
  std::map<std::string, std::set<std::size_t>> resolved;
  std::vector<const Entity*> entities = xr.named(name), destructors = xr.named("~" + name);
  entities.insert(entities.end(), destructors.begin(), destructors.end());
  for (const Entity* e : entities) {
    if (!e->isDeclared()) continue;
    for (const Reference& r : e->references) {
      if (r.inSystemHeader) continue;
      std::size_t off = nameOffset(xr, r.pos, name);
      if (off != std::string::npos) resolved[r.pos.file].insert(off);
    }
  }
  std::set<std::string> files;
  for (const auto& kv : xr.entities()) {
    for (const Reference& r : kv.second.references) {
      if (!r.inSystemHeader) files.insert(r.pos.file);
    }
  }
  std::vector<Position> out;
  for (const std::string& f : files) {
    const SourceText& st = sourceText(f);
    const std::set<std::size_t>& known = resolved[f];
    for (std::size_t off : st.occurrences(name)) {
      if (!known.count(off)) out.push_back(st.position(off));
    }
  }
  return out;
}

// ---------------------------------------------------------------------------------------------
// Edits
// ---------------------------------------------------------------------------------------------

void Edits::replace(const std::string& file, std::size_t start, std::size_t end, const std::string& text) {
  edits_[file].push_back(Edit{start, end, text, count_++});
}

std::vector<std::string> Edits::conflicts() const {
  std::vector<std::string> v;
  for (const auto& kv : edits_) {
    std::vector<Edit> e = kv.second;
    std::sort(e.begin(), e.end(), [](const Edit& a, const Edit& b) {
      return a.start != b.start ? a.start < b.start : a.order < b.order;
    });
    for (std::size_t i = 1; i < e.size(); ++i) {
      if (e[i].start < e[i - 1].end) v.push_back(kv.first + ":" + std::to_string(e[i].start));
    }
  }
  return v;
}

std::string Edits::apply(const std::string& file, const std::string& text) const {
  auto it = edits_.find(file);
  if (it == edits_.end()) return text;
  std::vector<Edit> e = it->second;
  std::sort(e.begin(), e.end(), [](const Edit& a, const Edit& b) {
    return a.start != b.start ? a.start < b.start : a.order < b.order;
  });
  std::string out;
  std::size_t at = 0;
  for (const Edit& ed : e) {
    if (ed.start < at) continue;  // overlaps an earlier replacement
    out.append(text, at, ed.start - at);
    out += ed.text;
    at = std::max(at, ed.end);
  }
  out.append(text, at, std::string::npos);
  return out;
}

std::vector<std::string> Edits::files() const {
  std::vector<std::string> v;
  for (const auto& kv : edits_) v.push_back(kv.first);
  return v;
}

bool Edits::write(std::string* error) const {
  for (const auto& kv : edits_) {
    SourceText& st = sourceText(kv.first);
    if (!st.ok()) {
      if (error) *error = kv.first + ": cannot read the file";
      return false;
    }
    std::string text = apply(kv.first, st.text());
    std::ofstream out(kv.first.c_str(), std::ios::binary | std::ios::trunc);
    out << text;
    if (!out) {
      if (error) *error = kv.first + ": cannot write the file";
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------------------------
// Command lines
// ---------------------------------------------------------------------------------------------

void splitCommandLine(int argc, char* argv[], std::vector<std::string>& frontEndArgs,
                      std::vector<std::string>& toolArgs) {
  static const char* exts[] = {".c", ".cc", ".cpp", ".cxx", ".c++", ".C", ".h", ".hh", ".hpp", ".hxx", ".ii", ".i"};
  frontEndArgs.clear();
  toolArgs.clear();
  if (argc > 0) frontEndArgs.push_back(argv[0]);
  bool sourceSeen = false;
  // Options of the compiler that take a separate argument
  static const char* withArg[] = {"-I", "-D", "-U", "-include", "-isystem", "-o", "-x", "-imacros", "-iquote",
                                   "-rose:verbose", "-rose:o", "-rose:output"};
  bool msvc = false;
  std::string msvcDir;
  int msvcVersion = 0;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    // Visual C++ and its headers
    if (a == "--msvc" || a.compare(0, 7, "--msvc=") == 0) {
      msvc = true;
      msvcDir = a.size() > 7 ? a.substr(7) : "";
      continue;
    }
    if (a.compare(0, 15, "--msvc-version=") == 0) {
      msvcVersion = std::atoi(a.c_str() + 15);
      continue;
    }
    if (!a.empty() && a[0] == '-') {
      frontEndArgs.push_back(a);
      int n = (a == "-edg_parameter:" || a == "--edg_parameter:") ? 2 : 0;
      for (const char* w : withArg) {
        if (a == w) n = 1;
      }
      for (; n > 0 && i + 1 < argc; --n) frontEndArgs.push_back(argv[++i]);
      continue;
    }
    bool isSource = false;
    if (!sourceSeen) {
      for (const char* e : exts) {
        std::size_t n = std::strlen(e);
        if (a.size() > n && a.compare(a.size() - n, n, e) == 0) isSource = true;
      }
    }
    if (isSource) {
      frontEndArgs.push_back(a);
      sourceSeen = true;
    } else {
      toolArgs.push_back(a);
    }
  }
  if (msvc || msvcVersion != 0) {
    std::vector<std::string> dirs = microsoftIncludeDirs(msvcDir);
    if (dirs.empty()) {
      std::fprintf(stderr, "%s: warning: %s\n", argc > 0 ? argv[0] : "",
                   msvcDir.empty() ? "--msvc: the INCLUDE environment variable is not set"
                                   : ("--msvc=" + msvcDir + ": no include folder there").c_str());
    }
    setMicrosoftMode(dirs, msvcVersion);
  }
}

}  // namespace RoseRefactor
