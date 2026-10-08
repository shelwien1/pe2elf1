// rose-m2g: turns a member function into a global function with an explicit "This" parameter.
//
//   rose-m2g [options] source.cpp Class::method
//
//   struct A {                                struct A {
//     int v;                                    int v;
//     int get(int k) const {          ->        friend int get(const A* This, int k);
//       return v + k;                         };
//     }                                       inline int get(const A* This, int k) {
//   };                                          return This->v + k;
//   ... a.get(1) ... p->get(2) ...            }
//                                             ... get(&a, 1) ... get(p, 2) ...
//
// All the (non-virtual, non-static) member functions called "method" of the class are
// converted.  In the function, "this" becomes "This", the members that were named without
// qualification are accessed through This ("This->v"), and the static members, types and
// enumerators of the class are qualified with its name.  The declaration in the class becomes a
// friend declaration (the function keeps its access to private members); a definition in the
// class moves after the class (as an inline function), a definition outside the class stays
// where it is.  Calls become calls of the global function: obj.method(x) -> method(&obj, x),
// ptr->method(x) -> method(ptr, x), and method(x) in member functions -> method(this, x).
//
// The options are those of the compiler (-I, -D, -std=...), and --dry-run (print the changed
// files instead of writing them).  Not converted: virtual and static member functions,
// operators, member function templates, members of class templates, and functions whose
// address is taken (&Class::method).
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
    "usage: rose-m2g [options] source.cpp Class::method\n"
    "Turns the member function into a global function with an explicit This parameter.\n"
    "options: the compiler's (-I, -D, -std=...), --dry-run\n";

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

std::string where(const Position& p) { return displayName(p.file) + ":" + std::to_string(p.line); }

bool isIdentChar(char c) { return std::isalnum((unsigned char)c) || c == '_' || (unsigned char)c >= 0x80; }

// A change of the text of a file
struct Change {
  std::size_t start, end;
  std::string text;
  bool consumed = false;  // included in the text of a larger change
};

class Changes {
public:
  void add(const std::string& file, std::size_t start, std::size_t end, const std::string& text) {
    changes_[file].push_back(Change{start, end, text});
  }
  // The text of [start, end) of a file with the changes inside it, which are then consumed
  std::string text(const std::string& file, std::size_t start, std::size_t end) {
    const std::string& t = sourceText(file).text();
    std::vector<Change*> inside;
    for (Change& c : changes_[file]) {
      if (!c.consumed && c.start >= start && c.end <= end && !(c.start == end && c.end == end)) inside.push_back(&c);
    }
    std::stable_sort(inside.begin(), inside.end(), [](const Change* a, const Change* b) { return a->start < b->start; });
    std::string out;
    std::size_t at = start;
    for (Change* c : inside) {
      if (c->start < at) continue;
      out.append(t, at, c->start - at);
      out += c->text;
      at = c->end;
      c->consumed = true;
    }
    out.append(t, at, end - at);
    return out;
  }
  // Consumes the changes inside [start, end) without using them
  void drop(const std::string& file, std::size_t start, std::size_t end) {
    for (Change& c : changes_[file]) {
      if (c.start >= start && c.end <= end && !(c.start == end && c.end == end)) c.consumed = true;
    }
  }
  void addTo(Edits& edits) const {
    for (const auto& kv : changes_) {
      for (const Change& c : kv.second) {
        if (!c.consumed) edits.replace(kv.first, c.start, c.end, c.text);
      }
    }
  }

private:
  std::map<std::string, std::vector<Change>> changes_;
};

// The start of the postfix expression that ends just before offset end ("a.b[i]" in
// "x = a.b[i].f()"), or end if there is none
std::size_t expressionStart(const SourceText& st, std::size_t end) {
  static const std::set<std::string> stop = {"return", "new", "delete", "case", "throw", "else", "do",
                                             "co_await", "co_return", "co_yield", "sizeof", "typeid"};
  const std::string& t = st.text();
  std::size_t i = st.skipSpaceBackward(end);
  std::size_t start = end;
  bool needOperand = true;  // an operand (name, parenthesized expression, ...) comes next
  for (;;) {
    if (i == 0) break;
    char c = t[i - 1];
    if (needOperand) {
      if (c == ')' || c == ']') {
        std::size_t m = st.matching(i - 1);
        if (m == std::string::npos) break;
        start = m;
        i = st.skipSpaceBackward(m);
        // A call or subscript: the callee comes before; a parenthesized expression ends here
        if (c == ')' && (i == 0 || !(isIdentChar(t[i - 1]) || t[i - 1] == ')' || t[i - 1] == ']' || t[i - 1] == '>'))) {
          needOperand = false;
          continue;
        }
        continue;
      }
      if (c == '>' && !(i >= 2 && t[i - 2] == '-')) {
        // Template arguments of a name ("cast<A&>(x)")
        std::size_t m = st.matching(i - 1);
        if (m == std::string::npos) break;
        i = st.skipSpaceBackward(m);
        continue;
      }
      if (isIdentChar(c)) {
        std::size_t b = i;
        while (b > 0 && isIdentChar(t[b - 1])) --b;
        std::string w = t.substr(b, i - b);
        if (stop.count(w)) break;
        start = b;
        i = st.skipSpaceBackward(b);
        needOperand = false;
        continue;
      }
      break;
    }
    // After an operand: a member access or scope operator continues the expression
    if (c == '.' && !(i >= 2 && t[i - 2] == '.')) {
      i = st.skipSpaceBackward(i - 1);
      needOperand = true;
      continue;
    }
    if (c == '>' && i >= 2 && t[i - 2] == '-') {
      i = st.skipSpaceBackward(i - 2);
      needOperand = true;
      continue;
    }
    if (c == ':' && i >= 2 && t[i - 2] == ':') {
      i = st.skipSpaceBackward(i - 2);
      needOperand = true;
      continue;
    }
    if (c == ')' || c == ']') {  // "f(a)(b)"
      needOperand = true;
      continue;
    }
    break;
  }
  return start;
}

// The start of a declaration whose name (or qualified name) starts at offset nameStart: just
// after the preceding ";", "{", "}" or access label
std::size_t declarationStart(const SourceText& st, std::size_t nameStart) {
  const std::string& t = st.text();
  std::size_t i = nameStart;
  while (i > 0) {
    std::size_t p = st.skipSpaceBackward(i);
    if (p == 0) {
      i = 0;
      break;
    }
    char c = t[p - 1];
    if (c == ';' || c == '{' || c == '}') {
      i = p;
      break;
    }
    if (c == ':' && !(p >= 2 && t[p - 2] == ':') && !(p < t.size() && t[p] == ':')) {
      i = p;  // access label
      break;
    }
    if (c == '>' || c == ')' || c == ']') {
      std::size_t m = st.matching(p - 1);
      if (m == std::string::npos) {
        i = p - 1;
        continue;
      }
      i = m;
      continue;
    }
    i = p - 1;
    while (i > 0 && isIdentChar(t[i - 1]) && isIdentChar(t[i])) --i;
  }
  return st.skipSpace(i);
}

// The parts of a function declaration
struct FunctionText {
  std::string file;
  std::size_t declStart = 0;   // the declaration (decl-specifiers)
  std::size_t qualStart = 0;   // the name with its qualifiers ("A::get")
  std::size_t nameStart = 0;   // the name
  std::size_t paramsOpen = 0;  // "("
  std::size_t paramsClose = 0; // ")"
  std::size_t specEnd = 0;     // the end of the qualifiers after the parameters
  std::size_t bodyOpen = std::string::npos, bodyClose = std::string::npos;  // "{" "}" of the body
  std::size_t end = 0;         // after the ";" or the body
  bool ok = false;
};

FunctionText parseFunction(const SourceText& st, std::size_t nameStart, const std::string& name) {
  FunctionText f;
  f.file = st.file();
  const std::string& t = st.text();
  f.nameStart = nameStart;
  // The qualifiers of the name ("ns::A::")
  std::size_t q = nameStart;
  for (;;) {
    std::size_t p = st.skipSpaceBackward(q);
    if (p >= 2 && t[p - 1] == ':' && t[p - 2] == ':') {
      std::size_t b = st.skipSpaceBackward(p - 2);
      if (b > 0 && t[b - 1] == '>') {
        std::size_t m = st.matching(b - 1);
        if (m == std::string::npos) break;
        b = st.skipSpaceBackward(m);
      }
      std::size_t s = b;
      while (s > 0 && isIdentChar(t[s - 1])) --s;
      if (s == b) break;
      q = s;
      continue;
    }
    break;
  }
  f.qualStart = q;
  f.declStart = declarationStart(st, q);
  std::size_t open = st.skipSpace(nameStart + name.size());
  if (open >= t.size() || t[open] != '(') return f;
  f.paramsOpen = open;
  f.paramsClose = st.matching(open);
  if (f.paramsClose == std::string::npos) return f;
  std::size_t i = st.skipSpace(f.paramsClose + 1);
  while (i < t.size() && t[i] != '{' && t[i] != ';' && t[i] != '=') {
    if (t[i] == '(' || t[i] == '[') {
      std::size_t m = st.matching(i);
      if (m == std::string::npos) return f;
      i = m;
    }
    i = st.skipSpace(i + 1);
  }
  if (i >= t.size()) return f;
  f.specEnd = st.skipSpaceBackward(i);
  if (t[i] == '{') {
    f.bodyOpen = i;
    f.bodyClose = st.matching(i);
    if (f.bodyClose == std::string::npos) return f;
    f.end = f.bodyClose + 1;
  } else if (t[i] == ';') {
    f.end = i + 1;
  } else {
    return f;  // "= 0", "= delete", ...
  }
  f.ok = true;
  return f;
}

// The parameters of a parameter list ("int a, const std::map<K, V>& m = {}"), split at the
// top-level commas
std::vector<std::string> splitParameters(const std::string& s) {
  std::vector<std::string> v;
  int depth = 0;
  std::string cur;
  for (char c : s) {
    if (c == '(' || c == '[' || c == '{' || c == '<') ++depth;
    if ((c == ')' || c == ']' || c == '}' || c == '>') && depth > 0) --depth;
    if (c == ',' && depth == 0) {
      v.push_back(cur);
      cur.clear();
      continue;
    }
    cur += c;
  }
  if (cur.find_first_not_of(" \t\r\n") != std::string::npos) v.push_back(cur);
  return v;
}

std::string trim(const std::string& s) {
  std::size_t b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos) return "";
  std::size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e + 1 - b);
}

std::string withoutDefault(const std::string& param) {
  int depth = 0;
  for (std::size_t i = 0; i < param.size(); ++i) {
    char c = param[i];
    if (c == '(' || c == '[' || c == '{' || c == '<') ++depth;
    if ((c == ')' || c == ']' || c == '}' || c == '>') && depth > 0) --depth;
    if (c == '=' && depth == 0 && (i + 1 >= param.size() || param[i + 1] != '=')) return trim(param.substr(0, i));
  }
  return trim(param);
}

// The qualifiers after the parameter list without "const", "volatile", "&", "&&", "override"
// and "final" (they belong to the member function, not to the global function)
std::string functionSpecifiers(const std::string& s) {
  std::string out;
  std::size_t i = 0;
  while (i < s.size()) {
    if (isIdentChar(s[i])) {
      std::size_t e = i;
      while (e < s.size() && isIdentChar(s[e])) ++e;
      std::string w = s.substr(i, e - i);
      if (w != "const" && w != "volatile" && w != "override" && w != "final") out += w;
      i = e;
      continue;
    }
    if (s[i] == '&') {
      ++i;
      continue;
    }
    out += s[i++];
  }
  return trim(out);
}

int convert(const CrossReferences& xr, const std::string& className, const std::string& method, bool dryRun) {
  // The class
  std::vector<const Entity*> classes;
  bool templateFound = false;
  for (const auto& kv : xr.entities()) {
    const Entity& e = kv.second;
    if (e.name != className && e.qualifiedName != className) continue;
    if (e.declaredInSystemHeader()) continue;
    if (e.kind == Kind::ClassTemplate || e.isInTemplate) {
      templateFound = true;
      continue;
    }
    if ((e.kind == Kind::Class || e.kind == Kind::Struct || e.kind == Kind::Union) && !e.isTemplateInstance) {
      classes.push_back(&e);
    }
  }
  if (classes.empty()) {
    std::cerr << "rose-m2g: " << (templateFound ? className + " is a class template, which is not supported"
                                                : "no class " + className + " is defined outside system headers")
              << "\n";
    return 1;
  }
  if (classes.size() > 1) {
    std::cerr << "rose-m2g: several classes are called " << className << "; use the qualified name:";
    for (const Entity* c : classes) std::cerr << " " << c->qualifiedName;
    std::cerr << "\n";
    return 1;
  }
  const Entity& cls = *classes.front();

  // Its definition, and the end of the outermost class that contains it
  Position clsDef;
  for (const Reference& r : cls.references) {
    if (r.isDefinition() && !r.inSystemHeader) clsDef = r.pos;
  }
  if (!clsDef.valid()) {
    std::cerr << "rose-m2g: the definition of " << cls.qualifiedName << " is not in this translation unit\n";
    return 1;
  }
  const Entity* outer = &cls;
  std::string relName = cls.qualifiedName;
  {
    const Entity* p = xr.entity(cls.parent);
    while (p != nullptr && (p->kind == Kind::Class || p->kind == Kind::Struct || p->kind == Kind::Union)) {
      outer = p;
      p = xr.entity(p->parent);
    }
    if (p != nullptr && p->kind == Kind::Namespace && relName.compare(0, p->qualifiedName.size() + 2, p->qualifiedName + "::") == 0) {
      relName = relName.substr(p->qualifiedName.size() + 2);
    }
  }
  Position outerDef;
  for (const Reference& r : outer->references) {
    if (r.isDefinition() && !r.inSystemHeader) outerDef = r.pos;
  }
  SourceText& ot = sourceText(outerDef.file);
  std::size_t outerOpen = ot.find('{', ot.offset(outerDef));
  std::size_t outerClose = outerOpen == std::string::npos ? outerOpen : ot.matching(outerOpen);
  if (outerClose == std::string::npos) {
    std::cerr << "rose-m2g: " << where(outerDef) << ": cannot find the body of " << outer->qualifiedName << "\n";
    return 1;
  }
  std::size_t outerEnd = ot.find(';', outerClose + 1);  // after "};"
  outerEnd = outerEnd == std::string::npos ? outerClose + 1 : outerEnd + 1;
  SourceText& ct = sourceText(clsDef.file);
  std::size_t clsOpen = ct.find('{', ct.offset(clsDef));
  std::size_t clsClose = clsOpen == std::string::npos ? clsOpen : ct.matching(clsOpen);

  // The member functions
  std::vector<const Entity*> methods;
  for (const auto& kv : xr.entities()) {
    const Entity& m = kv.second;
    if (m.parent != cls.id || m.name != method || m.isImplicit) continue;
    std::string problem;
    if (m.kind == Kind::FunctionTemplate) problem = "is a member function template";
    else if (m.kind != Kind::MemberFunction) problem = std::string("is a ") + kindName(m.kind);
    else if (m.isVirtual) problem = "is virtual";
    else if (m.isStatic) problem = "is static";
    else if (method.compare(0, 8, "operator") == 0 && (method.size() == 8 || !isIdentChar(method[8]))) {
      problem = "is an operator";
    }
    if (!problem.empty()) {
      std::cerr << "rose-m2g: " << m.qualifiedName << " " << problem << ", which cannot be converted\n";
      return 1;
    }
    methods.push_back(&m);
  }
  if (methods.empty()) {
    std::cerr << "rose-m2g: " << cls.qualifiedName << " has no member function " << method << "\n";
    return 1;
  }
  std::set<EntityId> methodIds;
  for (const Entity* m : methods) methodIds.insert(m->id);

  Changes changes;
  std::vector<std::string> problems;

  // The functions: their declaration in the class and their definition
  struct Method {
    const Entity* entity;
    FunctionText inClass, outside;  // outside.ok if defined outside the class
  };
  std::vector<Method> ms;
  for (const Entity* m : methods) {
    Method mm;
    mm.entity = m;
    for (const Reference& r : m->references) {
      if (!r.isDeclaration() || r.inSystemHeader) continue;
      SourceText& st = sourceText(r.pos.file);
      std::size_t off = nameOffset(xr, r.pos, method);
      if (off == std::string::npos) {
        problems.push_back(where(r.pos) + ": the declaration of " + method + " is not written as such (macro?)");
        continue;
      }
      FunctionText f = parseFunction(st, off, method);
      if (!f.ok) {
        problems.push_back(where(r.pos) + ": cannot parse the declaration of " + method);
        continue;
      }
      bool inside = r.pos.file == clsDef.file && off > clsOpen && off < clsClose;
      if (inside) mm.inClass = f;
      else if (f.bodyOpen != std::string::npos) mm.outside = f;
    }
    if (!mm.inClass.ok) {
      problems.push_back("cannot find the declaration of " + m->qualifiedName + " in its class");
      continue;
    }
    ms.push_back(mm);
  }

  // The regions of the bodies (their text moves or changes) and of the in-class declarations
  struct Region {
    std::string file;
    std::size_t start, end;
    bool contains(const std::string& f, std::size_t o) const { return f == file && o >= start && o < end; }
  };
  std::vector<Region> bodies, signatures;
  for (const Method& m : ms) {
    const FunctionText& def = m.outside.ok ? m.outside : m.inClass;
    if (def.bodyOpen != std::string::npos) bodies.push_back(Region{def.file, def.bodyOpen, def.bodyClose + 1});
    signatures.push_back(Region{m.inClass.file, m.inClass.declStart, m.inClass.paramsClose + 1});
    if (m.outside.ok) signatures.push_back(Region{m.outside.file, m.outside.paramsOpen, m.outside.paramsClose + 1});
  }
  auto inBody = [&](const std::string& f, std::size_t o) {
    for (const Region& r : bodies) {
      if (r.contains(f, o)) return true;
    }
    return false;
  };
  auto inSignature = [&](const std::string& f, std::size_t o) {
    for (const Region& r : signatures) {
      if (r.contains(f, o)) return true;
    }
    return false;
  };
  auto isClassOrBase = [&](EntityId c) { return c == cls.id || xr.isBaseOf(c, cls.id); };

  // In the bodies and signatures: members named without qualification
  for (const auto& kv : xr.entities()) {
    const Entity& e = kv.second;
    if (methodIds.count(e.id) || e.kind == Kind::Constructor || e.kind == Kind::Destructor) continue;
    EntityId owner = e.parent;
    if (e.kind == Kind::Enumerator && owner != 0) {
      const Entity* en = xr.entity(owner);  // the enumeration
      if (en != nullptr && en->kind == Kind::Enum && !isClassOrBase(owner)) owner = en->parent;
    }
    if (owner == 0 || !isClassOrBase(owner)) continue;
    bool member = e.kind == Kind::Field || e.kind == Kind::StaticDataMember || e.kind == Kind::MemberFunction ||
                  e.kind == Kind::Enumerator || e.isType() || e.kind == Kind::FunctionTemplate;
    if (!member) continue;
    bool instanceMember = (e.kind == Kind::Field || e.kind == Kind::MemberFunction || e.kind == Kind::FunctionTemplate) &&
                          !e.isStatic;
    for (const Reference& r : e.references) {
      if (r.isDeclaration() || r.inSystemHeader) continue;
      SourceText& st = sourceText(r.pos.file);
      std::size_t off = st.offset(r.pos);
      if (off == std::string::npos || st.identifierAt(off) != e.name) continue;
      bool body = inBody(r.pos.file, off);
      if (!body && !inSignature(r.pos.file, off)) continue;
      // Qualified already?
      std::size_t p = st.skipSpaceBackward(off);
      const std::string& t = st.text();
      if (p > 0 && (t[p - 1] == '.' || (t[p - 1] == '>' && p >= 2 && t[p - 2] == '-') ||
                    (t[p - 1] == ':' && p >= 2 && t[p - 2] == ':'))) {
        continue;
      }
      if (instanceMember) {
        if (body) changes.add(r.pos.file, off, off, "This->");
      } else {
        changes.add(r.pos.file, off, off, relName + "::");
      }
    }
  }
  // "this" in the bodies
  for (const Region& b : bodies) {
    SourceText& st = sourceText(b.file);
    for (std::size_t i = b.start; i < b.end; ++i) {
      std::string w = st.identifierAt(i);
      if (w.empty()) continue;
      if (w == "this") changes.add(b.file, i, i + 4, "This");
      i += w.size() - 1;
    }
  }

  // The calls
  int calls = 0;
  for (const Entity* m : methods) {
    for (const Reference& r : m->references) {
      if (r.isDeclaration() || r.inSystemHeader) continue;
      SourceText& st = sourceText(r.pos.file);
      const std::string& t = st.text();
      std::string macro;
      std::size_t off = nameOffset(xr, r.pos, method, &macro);
      if (off == std::string::npos) {
        if (!macro.empty()) problems.push_back(where(r.pos) + ": " + method + " is used in the macro " + macro);
        continue;
      }
      std::size_t open = st.skipSpace(off + method.size());
      if (open >= t.size() || t[open] != '(') {
        problems.push_back(where(r.pos) + ": " + method + " is used without being called (e.g. &" + className + "::" +
                           method + ")");
        continue;
      }
      std::string self = inBody(r.pos.file, off) ? "This" : "this";
      // What comes before the name
      std::size_t q = off;  // the start of the qualified name
      for (;;) {
        std::size_t p = st.skipSpaceBackward(q);
        if (p >= 2 && t[p - 1] == ':' && t[p - 2] == ':') {
          std::size_t b = st.skipSpaceBackward(p - 2);
          if (b > 0 && t[b - 1] == '>') {
            std::size_t mm = st.matching(b - 1);
            if (mm == std::string::npos) break;
            b = st.skipSpaceBackward(mm);
          }
          std::size_t s = b;
          while (s > 0 && isIdentChar(t[s - 1])) --s;
          if (s == b) break;
          q = s;
          continue;
        }
        break;
      }
      std::size_t p = st.skipSpaceBackward(q);
      std::size_t removeFrom = q;
      std::string first;
      if (p > 0 && t[p - 1] == '.' && !(p >= 2 && t[p - 2] == '.')) {
        std::size_t s = expressionStart(st, p - 1);
        std::string obj = trim(changes.text(r.pos.file, s, p - 1));
        first = "&" + obj;
        removeFrom = s;
      } else if (p >= 2 && t[p - 1] == '>' && t[p - 2] == '-') {
        std::size_t s = expressionStart(st, p - 2);
        std::string obj = trim(changes.text(r.pos.file, s, p - 2));
        first = obj == "this" ? self : obj;
        removeFrom = s;
      } else {
        first = self;  // implicit or qualified: the object is *this
      }
      changes.drop(r.pos.file, removeFrom, off);
      changes.add(r.pos.file, removeFrom, off, "");
      std::size_t afterOpen = st.skipSpace(open + 1);
      bool noArgs = afterOpen < t.size() && t[afterOpen] == ')';
      changes.add(r.pos.file, open + 1, open + 1, first + (noArgs ? "" : ", "));
      ++calls;
    }
  }

  if (!problems.empty()) {
    for (const std::string& p : problems) std::cerr << "rose-m2g: " << p << "\n";
    std::cerr << "rose-m2g: nothing changed\n";
    return 1;
  }

  // The declarations and definitions
  std::string nl = ot.newline();
  std::string after;  // what goes after the outermost class
  for (const Method& m : ms) {
    const FunctionText& d = m.inClass;
    SourceText& st = sourceText(d.file);
    std::string thisParam = std::string(m.entity->isConst ? "const " : "") + relName + "* This";
    std::string specs = changes.text(d.file, d.declStart, d.qualStart);  // decl-specifiers, return type
    std::string params = changes.text(d.file, d.paramsOpen + 1, d.paramsClose);
    std::string quals = functionSpecifiers(st.text().substr(d.paramsClose + 1, d.specEnd - d.paramsClose - 1));
    std::vector<std::string> ps = splitParameters(params);
    std::string withDefaults = thisParam, without = thisParam;
    for (const std::string& p : ps) {
      std::string tp = trim(p);
      if (tp == "void") continue;
      withDefaults += ", " + tp;
      without += ", " + withoutDefault(tp);
    }
    std::string signature = method + "(" + withDefaults + ")" + (quals.empty() ? "" : " " + quals);
    std::string friendDecl = "friend " + trim(specs) + " " + method + "(" + without + ")" + (quals.empty() ? "" : " " + quals) + ";";
    if (m.outside.ok) {
      // The definition stays outside the class: its name and parameters change
      const FunctionText& o = m.outside;
      SourceText& os = sourceText(o.file);
      std::string oparams = changes.text(o.file, o.paramsOpen + 1, o.paramsClose);
      std::string ps2;
      for (const std::string& p : splitParameters(oparams)) {
        std::string tp = trim(p);
        if (tp != "void") ps2 += ", " + tp;
      }
      std::string oquals = functionSpecifiers(os.text().substr(o.paramsClose + 1, o.specEnd - o.paramsClose - 1));
      changes.drop(o.file, o.qualStart, o.specEnd);
      changes.add(o.file, o.qualStart, o.specEnd,
                  method + "(" + thisParam + ps2 + ")" + (oquals.empty() ? "" : " " + oquals));
      if (d.bodyOpen != std::string::npos) {
        problems.push_back(where(st.position(d.nameStart)) + ": " + method + " is defined twice");
      }
      after += nl + nl + trim(specs) + " " + signature + ";";
    } else if (d.bodyOpen != std::string::npos) {
      // The definition moves after the class, indented as the class
      std::string body = changes.text(d.file, d.bodyOpen, d.bodyClose + 1);
      std::string from = st.indentation(st.position(d.declStart).line);
      std::string to = ot.indentation(outerDef.line);
      if (from.size() > to.size() && from.compare(0, to.size(), to) == 0) {
        std::string extra = from.substr(to.size()), out;
        std::size_t lineStart = 0;
        for (std::size_t i = 0; i <= body.size(); ++i) {
          if (i == body.size() || body[i] == '\n') {
            std::string line = body.substr(lineStart, i - lineStart);
            if (lineStart > 0 && line.compare(0, extra.size(), extra) == 0) line = line.substr(extra.size());
            out += line;
            if (i < body.size()) out += '\n';
            lineStart = i + 1;
          }
        }
        body = out;
      }
      std::string s = trim(specs);
      bool isInline = (" " + s + " ").find(" inline ") != std::string::npos ||
                      (" " + s + " ").find(" constexpr ") != std::string::npos;
      after += nl + nl + (isInline ? "" : "inline ") + s + " " + signature + " " + body;
    } else {
      // Defined in another translation unit
      after += nl + nl + trim(specs) + " " + signature + ";";
    }
    changes.drop(d.file, d.declStart, d.end);
    changes.add(d.file, d.declStart, d.end, friendDecl);
  }
  changes.add(outerDef.file, outerEnd, outerEnd, after);
  if (!problems.empty()) {
    for (const std::string& p : problems) std::cerr << "rose-m2g: " << p << "\n";
    std::cerr << "rose-m2g: nothing changed\n";
    return 1;
  }

  Edits edits;
  changes.addTo(edits);
  std::vector<std::string> conflicts = edits.conflicts();
  if (!conflicts.empty()) {
    std::cerr << "rose-m2g: overlapping changes (at " << conflicts.front() << "); nothing changed\n";
    return 1;
  }
  if (dryRun) {
    for (const std::string& f : edits.files()) {
      std::cout << "==== " << displayName(f) << "\n" << edits.apply(f, sourceText(f).text());
    }
  } else {
    std::string error;
    if (!edits.write(&error)) {
      std::cerr << "rose-m2g: " << error << "\n";
      return 1;
    }
  }
  std::cout << (dryRun ? "would convert " : "converted ") << cls.qualifiedName << "::" << method
            << (ms.size() > 1 ? " (" + std::to_string(ms.size()) + " overloads)" : std::string()) << " to a global function; "
            << calls << (calls == 1 ? " call" : " calls") << " changed\n";
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
  if (feArgs.size() < 2 || toolArgs.size() != 1) {
    std::cerr << usage;
    return 2;
  }
  std::size_t colons = toolArgs[0].rfind("::");
  if (colons == std::string::npos || colons == 0 || colons + 2 >= toolArgs[0].size()) {
    std::cerr << usage;
    return 2;
  }
  std::string className = toolArgs[0].substr(0, colons), method = toolArgs[0].substr(colons + 2);

  recordCrossReferences();
  buildAst(false);  // only the cross-references are needed
  feArgs.push_back("-rose:skipfinalCompileStep");
  SgProject* project = frontend(feArgs);
  if (project == nullptr || project->get_frontendErrorCode() != 0) {
    std::cerr << "rose-m2g: the source could not be parsed\n";
    return 1;
  }
  return convert(crossReferences(), className, method, dryRun);
}
