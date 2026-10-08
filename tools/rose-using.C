// rose-using: adds using-declarations for the names of dependent base classes that a class
// template uses without qualification.
//
//   rose-using [options] source.cpp [class]
//
// In a class template, an unqualified name is not looked up in a base class that depends on a
// template parameter: "x = 1;" in a member function of "template <class T> struct D : B<T>"
// does not find B<T>::x.  Microsoft Visual C++ (without /permissive-) and the EDG front end in
// its Microsoft mode accept such code, GCC and Clang report the name as undeclared.  rose-using
// parses the source as Visual C++ does (with EDG's options --no_dep_name and
// --no_parse_templates: template bodies are parsed for each instance, and names are looked up
// in all base classes), finds the unqualified names that refer to members of dependent base
// classes, and adds using-declarations for them to the class template:
//
//   template <class T> struct D : B<T> {
//     using B<T>::x;
//     ...
//
// ("using typename B<T>::type;" for types), with the access of the member, so that the code
// means the same for GCC and Clang.  Only the class templates that are instantiated in the
// translation unit can be checked (the lookup depends on the template arguments).
//
// With a class name, only that class template is changed; otherwise every class template
// defined outside system headers.  The options are those of the compiler (-I, -D, -std=...,
// ...), and --dry-run (show the using-declarations without changing the files).
#include "rose.h"
#include "RoseRefactor.h"

#include <cctype>
#include <cstring>
#include <iostream>
#include <set>
#include <unistd.h>

using namespace RoseRefactor;

namespace {

const char* usage =
    "usage: rose-using [options] source.cpp [class]\n"
    "Adds using-declarations for the names of dependent base classes that class templates use\n"
    "without qualification.  options: the compiler's (-I, -D, -std=...), --dry-run\n";

std::string displayName(const std::string& file) {
  char buf[4096];
  std::string cwd = getcwd(buf, sizeof buf) ? buf : "";
  std::string f = file;
#ifdef _WIN32
  for (char& c : cwd) if (c == '/') c = '\\';
  for (char& c : f) if (c == '/') c = '\\';
  const char sep = '\\';
#else
  const char sep = '/';
#endif
  if (!cwd.empty() && f.size() > cwd.size() + 1 && f.compare(0, cwd.size(), cwd) == 0 && f[cwd.size()] == sep) {
    return f.substr(cwd.size() + 1);
  }
  return f;
}

bool isKeyword(const std::string& w) {
  return w == "public" || w == "protected" || w == "private" || w == "virtual" || w == "typename" ||
         w == "class" || w == "struct";
}

// A region of a file: [start, end)
struct Region {
  std::string file;
  std::size_t start, end;
  bool contains(const Position& p) const {
    if (p.file != file) return false;
    std::size_t o = sourceText(file).offset(p);
    return o != std::string::npos && o >= start && o < end;
  }
};

// The body of a function whose name is at offset off: from the name to the closing brace of
// the body (npos if there is no body)
std::size_t functionBodyEnd(const SourceText& st, std::size_t off) {
  const std::string& t = st.text();
  std::size_t open = st.find('(', off);
  if (open == std::string::npos) return open;
  std::size_t close = st.matching(open);
  if (close == std::string::npos) return close;
  std::size_t i = st.skipSpace(close + 1);
  // Qualifiers, exception specification, trailing return type: up to ":" or "{" or ";"
  while (i < t.size() && t[i] != '{' && t[i] != ':' && t[i] != ';' && t[i] != '=') {
    if (t[i] == '(' || t[i] == '[') {
      std::size_t m = st.matching(i);
      if (m == std::string::npos) return m;
      i = m;
    }
    i = st.skipSpace(i + 1);
  }
  if (i >= t.size() || t[i] == ';' || t[i] == '=') return std::string::npos;
  if (t[i] == ':') {
    // Constructor initializers: name(...) or name{...}, separated by commas
    i = st.skipSpace(i + 1);
    for (;;) {
      while (i < t.size() && t[i] != '(' && t[i] != '{') {
        if (t[i] == '<') {
          std::size_t m = st.matching(i);
          if (m != std::string::npos) i = m;
        }
        ++i;
      }
      if (i >= t.size()) return std::string::npos;
      std::size_t m = st.matching(i);
      if (m == std::string::npos) return m;
      i = st.skipSpace(m + 1);
      if (i < t.size() && t[i] == ',') {
        i = st.skipSpace(i + 1);
        continue;
      }
      break;
    }
  }
  if (i >= t.size() || t[i] != '{') return std::string::npos;
  std::size_t end = st.matching(i);
  return end == std::string::npos ? end : end + 1;
}

// Whether the name at off is written without a qualifier ("x", not "a.x", "p->x" or "N::x")
bool unqualified(const SourceText& st, std::size_t off) {
  const std::string& t = st.text();
  std::size_t p = st.skipSpaceBackward(off);
  // "->template f<int>()"
  if (p >= 8 && t.compare(p - 8, 8, "template") == 0 && (p == 8 || !std::isalnum((unsigned char)t[p - 9]))) {
    p = st.skipSpaceBackward(p - 8);
  }
  if (p == 0) return true;
  char c = t[p - 1];
  if (c == '.') return false;
  if (c == '>' && p >= 2 && t[p - 2] == '-') return false;
  if (c == ':' && p >= 2 && t[p - 2] == ':') return false;
  if (c == '~') return unqualified(st, p - 1);
  return true;
}

// The names of the template parameters of the class template whose name is at offset nameOff
std::set<std::string> templateParameters(const SourceText& st, std::size_t nameOff) {
  std::set<std::string> params;
  const std::string& t = st.text();
  std::size_t kw = t.rfind("template", nameOff);
  while (kw != std::string::npos) {
    std::size_t lt = st.skipSpace(kw + 8);
    if (lt < t.size() && t[lt] == '<' && st.identifierAt(kw) == "template") {
      std::size_t gt = st.matching(lt);
      if (gt != std::string::npos && gt < nameOff) {
        // Each parameter: the last identifier before "," / "=" / ">" at the top level
        std::string last;
        int depth = 0;
        for (std::size_t i = lt + 1; i <= gt; ++i) {
          char c = t[i];
          std::string w = st.identifierAt(i);
          if (!w.empty()) {
            if (depth == 0 && !isKeyword(w) && w != "int" && w != "unsigned" && w != "long" && w != "bool" &&
                w != "char" && w != "short" && w != "auto" && w != "size_t" && w != "template") {
              last = w;
            }
            i += w.size() - 1;
            continue;
          }
          if (c == '<' || c == '(') ++depth;
          if ((c == '>' || c == ')') && depth > 0) {
            --depth;
            continue;
          }
          if (depth == 0 && (c == ',' || c == '=' || i == gt)) {
            if (!last.empty()) params.insert(last);
            last.clear();
            if (c == '=') {
              // Skip the default argument
              while (i < gt && !(t[i] == ',' && depth == 0)) {
                if (t[i] == '<' || t[i] == '(') ++depth;
                if ((t[i] == '>' || t[i] == ')') && depth > 0) --depth;
                ++i;
              }
            }
          }
        }
        return params;
      }
    }
    if (kw == 0) break;
    kw = t.rfind("template", kw - 1);
  }
  return params;
}

bool mentions(const SourceText& st, std::size_t start, std::size_t end, const std::set<std::string>& names) {
  for (std::size_t i = start; i < end; ++i) {
    std::string w = st.identifierAt(i);
    if (w.empty()) continue;
    if (names.count(w)) return true;
    i += w.size() - 1;
  }
  return false;
}

Access restrict(Access a, Access b) {
  auto rank = [](Access x) { return x == Access::Private ? 2 : x == Access::Protected ? 1 : 0; };
  return rank(a) >= rank(b) ? a : b;
}

struct UsingDecl {
  std::string base;  // "Base<T>"
  std::string name;
  bool type = false;
  Access access = Access::Public;
  bool operator<(const UsingDecl& o) const {
    if (access != o.access) return (int)access < (int)o.access;
    if (base != o.base) return base < o.base;
    return name < o.name;
  }
};

int process(const CrossReferences& xr, const std::string& className, bool dryRun) {
  // The class templates to check
  std::vector<const Entity*> templates;
  for (const auto& kv : xr.entities()) {
    const Entity& e = kv.second;
    if (e.kind != Kind::ClassTemplate || e.declaredInSystemHeader() || !e.declaration().valid()) continue;
    if (!className.empty() && e.name != className && e.qualifiedName != className) continue;
    templates.push_back(&e);
  }
  std::sort(templates.begin(), templates.end(),
            [](const Entity* a, const Entity* b) { return a->declaration() < b->declaration(); });
  if (templates.empty()) {
    if (!className.empty()) {
      std::cerr << "rose-using: no class template " << className << " is defined outside system headers\n";
      return 1;
    }
    std::cout << "no class templates outside system headers\n";
    return 0;
  }

  Edits edits;
  int changed = 0;
  std::vector<std::string> uninstantiated;
  for (const Entity* tt : templates) {
    // The definition: the name, the class body
    Position decl;
    for (const Reference& r : tt->references) {
      if (r.isDefinition() && !r.inSystemHeader) decl = r.pos;
    }
    if (!decl.valid()) continue;  // only declared
    SourceText& st = sourceText(decl.file);
    std::size_t nameOff = st.offset(decl);
    if (nameOff == std::string::npos) continue;
    std::size_t open = st.find('{', nameOff);
    std::size_t close = open == std::string::npos ? open : st.matching(open);
    if (close == std::string::npos) continue;
    std::vector<Region> regions{Region{decl.file, open, close}};

    // The instances
    std::vector<const Entity*> instances;
    for (const Entity* e : xr.sameDeclaration(*tt)) {
      if (e->isTemplateInstance && (e->kind == Kind::Class || e->kind == Kind::Struct || e->kind == Kind::Union)) {
        instances.push_back(e);
      }
    }
    std::string where = displayName(decl.file) + ":" + std::to_string(decl.line);
    bool headerShown = false;
    auto header = [&]() {
      if (!headerShown) std::cout << where << ": " << tt->qualifiedName << ":\n";
      headerShown = true;
    };
    if (instances.empty()) {
      if (!className.empty()) {
        std::cerr << "rose-using: " << where << ": " << tt->qualifiedName
                  << " is not instantiated in this translation unit, so it cannot be checked\n";
      } else {
        uninstantiated.push_back(tt->qualifiedName);
      }
      continue;
    }
    bool isStruct = instances.front()->kind != Kind::Class;
    std::set<std::string> params = templateParameters(st, nameOff);

    // Out-of-class definitions of members
    std::set<EntityId> instanceIds;
    for (const Entity* inst : instances) instanceIds.insert(inst->id);
    for (const auto& kv : xr.entities()) {
      const Entity& m = kv.second;
      if (!instanceIds.count(m.parent)) continue;
      for (const Reference& r : m.references) {
        if (!r.isDefinition() || r.inSystemHeader || regions.front().contains(r.pos)) continue;
        SourceText& mt = sourceText(r.pos.file);
        std::size_t mo = mt.offset(r.pos);
        std::size_t end = mo == std::string::npos ? mo : functionBodyEnd(mt, mo);
        if (end != std::string::npos) regions.push_back(Region{r.pos.file, mo, end});
      }
    }

    // The names that using-declarations of the class already introduce
    std::set<std::string> declared;
    {
      const std::string& t = st.text();
      int depth = 0;
      bool statementStart = true;
      for (std::size_t i = open + 1; i < close; ++i) {
        char c = t[i];
        std::string w = st.identifierAt(i);
        if (!w.empty()) {
          if (statementStart && depth == 0 && w == "using") {
            std::size_t semi = st.find(';', i);
            if (semi != std::string::npos && semi < close) {
              std::size_t last = st.skipSpaceBackward(semi);
              std::size_t b = last;
              while (b > i && (std::isalnum((unsigned char)t[b - 1]) || t[b - 1] == '_')) --b;
              declared.insert(t.substr(b, last - b));
            }
          }
          statementStart = false;
          i += w.size() - 1;
          continue;
        }
        if (c == '{' || c == '(' || c == '[') ++depth;
        if (c == '}' || c == ')' || c == ']') --depth;
        if (depth == 0 && (c == ';' || c == '}' || c == ':' || c == '{')) statementStart = true;
        else if (!std::isspace((unsigned char)c)) statementStart = false;
      }
    }

    std::set<UsingDecl> decls;
    std::set<std::string> reported;
    std::set<std::size_t> qualifiedTemplates;  // offsets of "name<" rewritten as "Base::template name<"
    for (const Entity* inst : instances) {
      // The names declared in the class itself
      std::set<std::string> own;
      for (const auto& kv : xr.entities()) {
        if (kv.second.parent == inst->id) own.insert(kv.second.name);
      }
      for (const auto& kv : xr.entities()) {
        const Entity& m = kv.second;
        if (m.parent == 0 || m.parent == inst->id || !m.isMember() || own.count(m.name) || declared.count(m.name)) {
          continue;
        }
        if (m.kind == Kind::Constructor || m.kind == Kind::Destructor || m.isImplicit) continue;
        const BaseClass* via = xr.directBaseLeadingTo(inst->id, m.parent);
        if (via == nullptr || !via->start.valid() || !via->end.valid()) continue;
        for (const Reference& r : m.references) {
          if (r.isDeclaration() || r.inSystemHeader) continue;
          bool inside = false;
          for (const Region& g : regions) inside |= g.contains(r.pos);
          if (!inside) continue;
          SourceText& rt = sourceText(r.pos.file);
          std::size_t off = rt.offset(r.pos);
          if (off == std::string::npos || rt.identifierAt(off) != m.name || !unqualified(rt, off)) continue;
          // The base specifier: is it dependent?
          SourceText& bt = sourceText(via->start.file);
          std::size_t bs = bt.offset(via->start), be = bt.offset(via->end);
          if (bs == std::string::npos || be == std::string::npos || be < bs) continue;
          if (!mentions(bt, bs, be + 1, params)) continue;
          // The base as written, without access and "virtual"
          std::size_t s = bs;
          for (;;) {
            std::string w = bt.identifierAt(s);
            if (w != "public" && w != "protected" && w != "private" && w != "virtual") break;
            s = bt.skipSpace(s + w.size());
          }
          std::string base = bt.text().substr(s, be + 1 - s);
          if (m.access == Access::Private) {
            std::string key = m.qualifiedName;
            if (reported.insert(key).second) {
              std::cerr << "rose-using: " << displayName(r.pos.file) << ":" << r.pos.line << ": " << m.name
                        << " is a private member of " << base << "; not changed\n";
            }
            continue;
          }
          // A member template with template arguments ("convert<int>(x)"): a using-declaration
          // does not tell GCC that the name is a template, the name is qualified instead
          std::size_t after = rt.skipSpace(off + m.name.size());
          if (after < rt.text().size() && rt.text()[after] == '<' && !m.isType()) {
            if (qualifiedTemplates.insert(off).second && r.pos.file == rt.file()) {
              edits.insert(r.pos.file, off, base + "::template ");
              header();
              std::cout << "  " << displayName(r.pos.file) << ":" << r.pos.line << ": " << m.name << "<...> -> "
                        << base << "::template " << m.name << "<...>\n";
              ++changed;
            }
            continue;
          }
          UsingDecl u;
          u.base = base;
          u.name = m.name;
          u.type = m.isType();
          u.access = restrict(m.access == Access::None ? Access::Public : m.access, via->access);
          decls.insert(u);
        }
      }
    }
    if (decls.empty()) continue;
    ++changed;

    // The text: groups by access, then the default access of the class again
    std::string nl = st.newline();
    Position bodyPos = st.position(open);
    std::string indent = st.indentation(bodyPos.line);
    std::string memberIndent = indent + "  ";
    // The indentation of the first member that is on its own line (not an access label)
    std::size_t first = st.skipSpace(open + 1);
    auto isAccessLabel = [&](std::size_t at) {
      std::string w = st.identifierAt(at);
      if (w != "public" && w != "protected" && w != "private") return false;
      std::size_t c = st.skipSpace(at + w.size());
      return c < st.text().size() && st.text()[c] == ':' && (c + 1 >= st.text().size() || st.text()[c + 1] != ':');
    };
    for (std::size_t m = first; m < close;) {
      Position mp = st.position(m);
      if (!isAccessLabel(m)) {
        if (mp.line > bodyPos.line && st.indentation(mp.line).size() == (std::size_t)mp.column - 1) {
          memberIndent = st.indentation(mp.line);
        }
        break;
      }
      m = st.skipSpace(st.text().find(':', m) + 1);
    }
    bool labelFollows = first < close && isAccessLabel(first);
    Access defaultAccess = isStruct ? Access::Public : Access::Private;
    Access current = defaultAccess;
    std::string text;
    header();
    for (const UsingDecl& u : decls) {
      if (u.access != current) {
        text += nl + indent + accessName(u.access) + ":";
        current = u.access;
      }
      std::string line = std::string("using ") + (u.type ? "typename " : "") + u.base + "::" + u.name + ";";
      text += nl + memberIndent + line;
      std::cout << "  " << (u.access == defaultAccess ? "" : std::string(accessName(u.access)) + ": ") << line << "\n";
    }
    if (current != defaultAccess && !labelFollows) text += nl + indent + accessName(defaultAccess) + ":";
    if (first < close && st.position(first).line == bodyPos.line && !labelFollows) {
      // The first member is on the line of the brace: it moves to the next line
      edits.replace(decl.file, open + 1, first, text + nl + memberIndent);
    } else {
      edits.insert(decl.file, open + 1, text);
    }
  }

  if (!uninstantiated.empty()) {
    std::cout << "not instantiated in this translation unit, so not checked:";
    for (const std::string& n : uninstantiated) std::cout << " " << n;
    std::cout << "\n";
  }
  if (changed == 0) {
    std::cout << "no using-declarations are needed\n";
    return 0;
  }
  if (!dryRun) {
    std::string error;
    if (!edits.write(&error)) {
      std::cerr << "rose-using: " << error << "\n";
      return 1;
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  ROSE_INITIALIZE;
  bool dryRun = false;
  std::vector<char*> args;
  for (int i = 0; i < argc; ++i) {
    if (std::strcmp(argv[i], "--dry-run") == 0) {
      dryRun = true;
    } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
      std::cout << usage;
      return 0;
    } else {
      args.push_back(argv[i]);
    }
  }
  std::vector<std::string> feArgs, toolArgs;
  splitCommandLine((int)args.size(), args.data(), feArgs, toolArgs);
  if (feArgs.size() < 2 || toolArgs.size() > 1) {
    std::cerr << usage;
    return 2;
  }
  // Look up names as Visual C++ does (template bodies are parsed for each instance)
  setFrontEndOptions({"--no_dep_name", "--no_parse_templates"});
  recordCrossReferences();
  buildAst(false);  // only the cross-references are needed
  feArgs.push_back("-rose:skipfinalCompileStep");
  SgProject* project = frontend(feArgs);
  if (project == nullptr || project->get_frontendErrorCode() != 0) {
    std::cerr << "rose-using: the source could not be parsed\n";
    return 1;
  }
  return process(crossReferences(), toolArgs.empty() ? "" : toolArgs[0], dryRun);
}
