// rose-ren: lists the C++ entities with a given name, or renames one of them.
//
//   rose-ren [options] source.cpp name                lists the entities called "name":
//                                                      [index] file:line  full name  type (kind)
//   rose-ren [options] source.cpp name[:index] new    renames one of them ("name" alone if there
//                                                      is only one) in its declarations and
//                                                      everywhere it is referenced
//
// The options are those of the compiler (-I, -D, -std=..., ...), and:
//   --dry-run   show the changes without writing the files
//   --force     rename even where a name would refer to something else afterwards, or another
//               entity shares a reference
//   --msvc      parse as Visual C++ does, with its headers (those of the INCLUDE environment
//               variable) instead of GCC's; --msvc=<folder>: the headers of a portable Visual C++
//               in that folder (include, ucrt\include, sdk\include); --msvc-version=<_MSC_VER>
//               (the default comes from the headers)
//
// Renaming changes the source files in place: the source file and the headers it includes,
// except system headers.  The positions of the names come from the EDG front end's
// cross-reference listing, so names are found in all their uses (including qualified names,
// using-declarations, base classes, template arguments and template instances); a class is
// renamed with its constructors and destructor, a virtual function with the functions that
// override it or that it overrides.  The uses of the name that the front end did not resolve
// (in code that the preprocessor skips, for example) are listed, and not renamed.
//
// Nothing is renamed where a name would refer to something else afterwards: the new name declared
// in the same scope; a use of the renamed entity where unqualified name lookup of the new name
// would find another entity first (declared in an inner scope, a member of a class, a template
// parameter), or a use of another entity called the new name where it would find the renamed
// one (lookup is simulated with the scopes that the front end had at each use; a class scope
// includes the base classes; qualified names are not looked up there); a member that would hide
// a member of a base class, or be hidden by one of a derived class; a member with the name of its
// class; a name declared in a template with the name of one of its template parameters; macros.
//
// The front end parses the bodies of all templates, also of those that are not instantiated.
// If it rejects the source, it parses it as GCC does (the bodies of templates only where they
// are instantiated), and then as Visual C++ does (names used in templates are also looked up in
// dependent base classes; see rose-using), so that code that only Visual C++ compiles can be
// renamed too.
#include "rose.h"
#include "RoseRefactor.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <set>
#include <tuple>

using namespace RoseRefactor;

namespace {

const char* usage =
    "usage: rose-ren [options] source.cpp name                list the entities called name\n"
    "       rose-ren [options] source.cpp name[:index] new    rename one of them to new\n"
    "options: the compiler's (-I, -D, -std=...), --dry-run, --force,\n"
    "         --msvc[=<Visual C++ folder>] (Visual C++ and its headers), --msvc-version=<_MSC_VER>\n";

// A renamable item: an entity with the entities declared at the same positions (a template and
// its instances)
struct Item {
  const Entity* main = nullptr;
  std::vector<const Entity*> entities;
  Position decl;
};

std::string displayName(const std::string& file) {
  // Relative to the current directory if inside it
  char buf[4096];
  std::string cwd = getcwd(buf, sizeof buf) ? buf : "";
#ifdef _WIN32
  for (char& c : cwd) if (c == '/') c = '\\';
  std::string f = file;
  for (char& c : f) if (c == '/') c = '\\';
  if (!cwd.empty() && f.size() > cwd.size() + 1 && _strnicmp(f.c_str(), cwd.c_str(), cwd.size()) == 0 &&
      f[cwd.size()] == '\\') {
    return f.substr(cwd.size() + 1);
  }
  return f;
#else
  if (!cwd.empty() && file.size() > cwd.size() + 1 && file.compare(0, cwd.size(), cwd) == 0 &&
      file[cwd.size()] == '/') {
    return file.substr(cwd.size() + 1);
  }
  return file;
#endif
}

bool isKeyword(const std::string& s) {
  static const std::set<std::string> kw = {
      "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
      "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const", "consteval", "constexpr",
      "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype", "default",
      "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
      "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not",
      "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register",
      "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
      "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true", "try", "typedef",
      "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
      "xor", "xor_eq"};
  return kw.count(s) != 0;
}

bool isIdentifier(const std::string& s) {
  if (s.empty() || !(std::isalpha((unsigned char)s[0]) || s[0] == '_')) return false;
  for (char c : s) {
    if (!(std::isalnum((unsigned char)c) || c == '_')) return false;
  }
  return true;
}

// Whether e is a constructor or destructor, or a constructor template (a member function template
// with the name of its class)
bool isConstructorOrDestructor(const CrossReferences& xr, const Entity& e) {
  if (e.kind == Kind::Constructor || e.kind == Kind::Destructor) return true;
  if (e.kind != Kind::FunctionTemplate || e.scope.kind != Scope::Kind::Class) return false;
  const Entity* c = xr.entity(e.scope.id);
  return c != nullptr && c->name == e.name;
}

// The entities called name that are declared outside system headers, grouped
std::vector<Item> itemsNamed(const CrossReferences& xr, const std::string& name) {
  std::vector<Item> items;
  std::set<EntityId> done;
  for (const Entity* e : xr.named(name)) {
    // (constructors and destructors are renamed with their class)
    if (done.count(e->id) || e->isImplicit || isConstructorOrDestructor(xr, *e)) continue;
    std::vector<const Entity*> group = xr.sameDeclaration(*e);
    for (const Entity* g : group) done.insert(g->id);
    Item item;
    item.entities = group;
    // The main entity: the template (or the entity itself) rather than an instance
    for (const Entity* g : group) {
      if (g->name != name || g->isImplicit) continue;
      if (item.main == nullptr || (item.main->isTemplateInstance && !g->isTemplateInstance)) item.main = g;
    }
    if (item.main == nullptr) continue;
    for (const Entity* g : group) {
      Position p = g->declaration();
      if (p.valid() && (!item.decl.valid() || p < item.decl)) item.decl = p;
    }
    if (!item.decl.valid()) continue;  // declared only in system headers
    items.push_back(item);
  }
  std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
    if (a.decl != b.decl) return a.decl < b.decl;
    return a.main->qualifiedName < b.main->qualifiedName;
  });
  return items;
}

std::string describe(const Entity& e) {
  std::string s = e.qualifiedName.empty() ? e.name : e.qualifiedName;
  std::string what = kindName(e.kind);
  if (e.isLocal && e.kind == Kind::Variable) what = "local variable";
  if (e.isStatic && e.kind == Kind::MemberFunction) what = "static member function";
  if (e.isVirtual) what = (e.isPureVirtual ? "pure virtual " : "virtual ") + what;
  if (!e.type.empty()) s += "  " + e.type;
  return s + "  (" + what + ")";
}

void list(const std::vector<Item>& items, const std::string& name) {
  if (items.empty()) {
    std::cout << "no entity called " << name << " is declared outside system headers\n";
    return;
  }
  for (std::size_t i = 0; i < items.size(); ++i) {
    std::size_t refs = 0;
    for (const Entity* e : items[i].entities) {
      for (const Reference& r : e->references) {
        if (!r.isDeclaration()) ++refs;
      }
    }
    std::cout << "[" << i << "] " << displayName(items[i].decl.file) << ":" << items[i].decl.line << "  "
              << describe(*items[i].main) << ", " << refs << (refs == 1 ? " reference" : " references") << "\n";
  }
}

// ---------------------------------------------------------------------------------------------
// Where the new name would refer to something else
// ---------------------------------------------------------------------------------------------

bool isTag(const Entity& e) {
  return e.kind == Kind::Class || e.kind == Kind::Struct || e.kind == Kind::Union || e.kind == Kind::Enum;
}

// Whether two entities' names can denote each other: in C, tags (struct, union and enum names)
// are apart from the other names; labels and macros are apart from everything else
bool sameNameSpace(const CrossReferences& xr, const Entity& a, const Entity& b) {
  if ((a.kind == Kind::Label) != (b.kind == Kind::Label)) return false;
  if ((a.kind == Kind::Macro) != (b.kind == Kind::Macro)) return false;
  return xr.cplusplus() || isTag(a) == isTag(b);
}

// Whether the name at a reference is written qualified (after "::", "." or "->"), so that it is
// not looked up in the scopes that enclose it
bool isQualified(const SourceText& st, std::size_t offset) {
  std::size_t p = st.skipSpaceBackward(offset);
  const std::string& t = st.text();
  if (p >= 8 && t.compare(p - 8, 8, "template") == 0 &&
      (p == 8 || !(std::isalnum((unsigned char)t[p - 9]) || t[p - 9] == '_'))) {
    p = st.skipSpaceBackward(p - 8);  // x.template f<T>()
  }
  if (p >= 2 && (t.compare(p - 2, 2, "::") == 0 || t.compare(p - 2, 2, "->") == 0)) return true;
  return p >= 1 && t[p - 1] == '.';
}

// What unqualified lookup considers for a name: all names; types only, after struct, class, union
// or enum (an elaborated type specifier); types and namespaces, before "::"
enum class LookupKind { All, Types, TypesAndNamespaces };

LookupKind lookupKind(const SourceText& st, std::size_t offset, std::size_t length) {
  const std::string& t = st.text();
  std::size_t next = st.skipSpace(offset + length);
  if (next + 1 < t.size() && t.compare(next, 2, "::") == 0) return LookupKind::TypesAndNamespaces;
  std::size_t p = st.skipSpaceBackward(offset);
  std::size_t b = p;
  while (b > 0 && (std::isalnum((unsigned char)t[b - 1]) || t[b - 1] == '_')) --b;
  std::string word = t.substr(b, p - b);
  if (word == "struct" || word == "class" || word == "union" || word == "enum") return LookupKind::Types;
  return LookupKind::All;
}

// Unqualified name lookup of the new name, as it would be after the renaming: the entities with
// the new name (those renamed to it included) that are declared in the innermost of the scopes
// enclosing a reference that declares any.  In a class scope, those are the members of the
// class, or else those found in its base classes (all members, wherever declared); in the other
// scopes, only the declarations that precede the reference count.
class Lookup {
public:
  Lookup(const CrossReferences& xr, const std::set<const Entity*>& targets, const std::string& oldName,
         const std::string& newName)
      : xr_(xr) {
    for (const Entity* e : xr.named(newName)) {
      if (!targets.count(e)) add(e);
    }
    for (const Entity* e : targets) {
      if (e->name == oldName) add(e);
    }
  }
  // The entities that the new name would refer to at r, written in place of the name of an
  // entity like e
  std::vector<const Entity*> at(const Reference& r, const Entity& e, LookupKind kind) {
    std::vector<const Entity*> found;
    for (const Scope& s : xr_.scopes(r.scopes)) {
      if (s.kind == Scope::Kind::Class) {
        std::set<EntityId> seen;
        inClass(s.id, e, kind, found, seen);
      } else {
        for (const Entity* c : candidates_) {
          if (considered(*c, e, kind) && c->scope == s && declaredBefore(*c, r)) found.push_back(c);
        }
      }
      if (!found.empty()) break;
    }
    return found;
  }

private:
  void add(const Entity* e) {
    if (e->isImplicit || e->scope.kind == Scope::Kind::None) return;
    const Reference* first = nullptr;
    for (const Reference& r : e->references) {
      if (r.isDeclaration() && (first == nullptr || r.order < first->order)) first = &r;
    }
    declared_[e] = first;
    candidates_.push_back(e);
  }
  // Whether c is declared before the reference r: by their positions in one file (the front end
  // may record a declaration later, a static array with the addresses of labels, for example),
  // else in the order of the translation unit
  bool declaredBefore(const Entity& c, const Reference& r) {
    const Reference* d = declared_[&c];
    if (d == nullptr) return true;
    if (d->pos.file == r.pos.file) return d->pos < r.pos;
    return d->order < r.order;
  }
  bool considered(const Entity& c, const Entity& e, LookupKind kind) const {
    if (!sameNameSpace(xr_, c, e)) return false;
    switch (kind) {
      case LookupKind::Types:
        return c.isType();
      case LookupKind::TypesAndNamespaces:
        return c.isType() || c.kind == Kind::Namespace;
      default:
        return true;
    }
  }
  // Member name lookup in class cls: its members (its own name included: the injected class
  // name), or else those found in its base classes
  void inClass(EntityId cls, const Entity& e, LookupKind kind, std::vector<const Entity*>& found,
               std::set<EntityId>& seen) {
    if (!seen.insert(cls).second) return;
    std::size_t n = found.size();
    for (const Entity* c : candidates_) {
      if (considered(*c, e, kind) && ((c->scope.kind == Scope::Kind::Class && c->scope.id == cls) || c->id == cls)) {
        found.push_back(c);
      }
    }
    if (found.size() > n) return;
    if (const Entity* ce = xr_.entity(cls)) {
      for (const BaseClass& b : ce->bases) inClass(b.entity, e, kind, found, seen);
    }
  }
  const CrossReferences& xr_;
  std::vector<const Entity*> candidates_;
  std::map<const Entity*, const Reference*> declared_;  // the first declaration
};

// Whether another entity declared at the same positions as e (a template and its instances, or
// the entities in them) is reported instead of e: the one declared in the source rather than by
// a template instantiation, preferably a template
bool isRepeatedInstance(const CrossReferences& xr, const Entity& e) {
  auto rank = [](const Entity* x) {
    bool declared = false;
    for (const Reference& r : x->references) declared |= r.code == 'd' || r.code == 'D';
    bool isTemplate =
        x->kind == Kind::ClassTemplate || x->kind == Kind::FunctionTemplate || x->kind == Kind::VariableTemplate;
    return std::make_tuple(x->isTemplateInstance, !declared, !isTemplate, x->id);
  };
  std::vector<const Entity*> group = xr.sameDeclaration(e);
  return *std::min_element(group.begin(), group.end(),
                           [&](const Entity* a, const Entity* b) { return rank(a) < rank(b); }) != &e;
}

std::string where(const Position& p) {
  return displayName(p.file) + ":" + std::to_string(p.line) + ":" + std::to_string(p.column);
}

// The places where renaming the targets (the entities called oldName among them) to newName
// would change what a name refers to: newName declared again in the same scope; a reference to a
// renamed entity where newName would find another entity (declared in an inner scope, or in a
// class that comes first); a reference to another entity called newName where the renamed entity
// would be found instead; a member renamed to the name of a member of a base or derived class, or
// of its class; a name in a template that is the name of a template parameter; and macros.
// Returns the number of problems (none with force: then they are warnings).
int nameClashes(const CrossReferences& xr, const std::set<const Entity*>& targets, const std::string& oldName,
                const std::string& newName, bool force) {
  const char* severity = force ? "warning: " : "";
  int problems = 0;
  std::set<std::string> reported;
  auto report = [&](const std::string& message) {
    if (!reported.insert(message).second) return;
    if (reported.size() <= 20) std::cerr << "rose-ren: " << severity << message << "\n";
    if (!force) ++problems;
  };
  std::vector<const Entity*> renamed;     // (an instance of a template is reported as the template)
  std::vector<const Entity*> renamedAll;  // (the references to a function template are those of its instances)
  for (const Entity* e : targets) {
    if (e->name != oldName || e->isImplicit) continue;
    renamedAll.push_back(e);
    if (!isRepeatedInstance(xr, *e)) renamed.push_back(e);
  }
  auto declaredAt = [](const Entity& e) {
    Position p = e.declaration();
    return p.valid() ? ", declared at " + where(p) : std::string();
  };

  // Macros: a macro called newName would replace the renamed name, and a renamed macro would
  // replace newName where it is used
  bool renamingMacro = false;
  for (const Entity* e : renamed) renamingMacro |= e->kind == Kind::Macro;
  for (const Entity* x : xr.named(newName)) {
    if (targets.count(x) || x->isImplicit || isRepeatedInstance(xr, *x)) continue;
    if (x->kind == Kind::Macro) {
      report(newName + " is a macro" + declaredAt(*x));
    } else if (renamingMacro) {
      report("the renamed macro would replace the name of " + describe(*x) + declaredAt(*x));
    }
  }

  // newName declared again in the scope of a renamed entity (the class or namespace, for those
  // whose scope is not known)
  for (const Entity* x : xr.named(newName)) {
    if (targets.count(x) || x->isImplicit || isRepeatedInstance(xr, *x)) continue;
    for (const Entity* e : renamed) {
      if (!sameNameSpace(xr, *x, *e) || e->kind == Kind::Macro) continue;
      bool same = e->scope.kind != Scope::Kind::None
                      ? x->scope == e->scope
                      : !e->isLocal && e->kind != Kind::Parameter && !x->isLocal && x->kind != Kind::Parameter &&
                            x->parent == e->parent;
      if (same) {
        report(newName + " is already declared in the same scope: " + describe(*x) + declaredAt(*x));
        break;
      }
    }
  }

  // Members: a member of a base class or of a derived class called newName
  for (const Entity* e : renamed) {
    if (e->scope.kind != Scope::Kind::Class || e->kind == Kind::Constructor) continue;
    for (const Entity* x : xr.named(newName)) {
      if (targets.count(x) || x->isImplicit || isRepeatedInstance(xr, *x) || x->scope.kind != Scope::Kind::Class ||
          x->scope == e->scope) {
        continue;
      }
      if (!sameNameSpace(xr, *x, *e)) continue;
      // (A constructor stands for the name of its class.)
      const Entity* c = x->kind == Kind::Constructor ? xr.entity(x->scope.id) : nullptr;
      if (xr.isBaseOf(x->scope.id, e->scope.id)) {
        report("the renamed member would hide " +
               (c != nullptr ? "the name of the base class " + describe(*c) : describe(*x) + " of a base class") +
               declaredAt(c != nullptr ? *c : *x));
      } else if (xr.isBaseOf(e->scope.id, x->scope.id)) {
        report((c != nullptr ? "the name of the derived class " + describe(*c) : describe(*x) + " of a derived class") +
               declaredAt(c != nullptr ? *c : *x) + ", would hide the renamed member");
      }
    }
  }

  // In C++, a member may not have the name of its class (constructors and destructors aside)
  auto isClass = [&](const Entity& e) {
    if (!xr.cplusplus()) return false;
    return e.kind == Kind::Class || e.kind == Kind::Struct || e.kind == Kind::Union || e.kind == Kind::ClassTemplate;
  };
  auto isMemberOf = [](const Entity& m, const Entity& c) {
    return m.scope.kind == Scope::Kind::Class && m.scope.id == c.id && m.kind != Kind::Constructor &&
           m.kind != Kind::Destructor && !m.isImplicit;
  };
  for (const Entity* e : renamed) {
    if (e->scope.kind == Scope::Kind::Class) {
      const Entity* c = xr.entity(e->scope.id);
      if (c != nullptr && isClass(*c) && c->name == newName && !targets.count(c) && isMemberOf(*e, *c)) {
        report("the renamed member would have the name of its class, " + describe(*c));
      }
    }
    if (isClass(*e)) {
      for (const Entity* x : xr.named(newName)) {
        if (!targets.count(x) && !isRepeatedInstance(xr, *x) && isMemberOf(*x, *e)) {
          report(describe(*x) + declaredAt(*x) + ", would have the name of its renamed class");
        }
      }
    }
  }

  // A template parameter may not be declared again in its template, nor have the name of the
  // template
  auto declaredWithin = [&](const Entity& e, const Scope& s) {
    for (const Reference& r : e.references) {
      if (!r.isDeclaration()) continue;
      for (const Scope& c : xr.scopes(r.scopes)) {
        if (c == s) return true;
      }
    }
    return false;
  };
  for (const Entity* x : xr.named(newName)) {
    if (targets.count(x) || x->isImplicit || isRepeatedInstance(xr, *x) || x->kind == Kind::Macro ||
        x->kind == Kind::Label) {
      continue;
    }
    for (const Entity* e : renamed) {
      if (e->kind == Kind::Macro || e->kind == Kind::Label) continue;
      if (x->kind == Kind::TemplateParameter && x->scope.kind != Scope::Kind::None && declaredWithin(*e, x->scope)) {
        report("the renamed " + describe(*e) + " would be declared in the template of the template parameter " +
               newName + declaredAt(*x));
        break;
      }
      if (e->kind == Kind::TemplateParameter && e->scope.kind != Scope::Kind::None && declaredWithin(*x, e->scope)) {
        report(describe(*x) + declaredAt(*x) + ", is declared in the template of the renamed template parameter");
        break;
      }
    }
  }

  // The references written unqualified, as unqualified lookup would see them after the renaming
  Lookup lookup(xr, targets, oldName, newName);
  // (Not those at a declaration of the entity: the front end records the implicit use of the
  // variable of a condition or of a range-based for statement where it is declared.)
  auto unqualified = [&](const Entity& e, const Reference& r, const std::string& name, LookupKind& kind) {
    if (r.isDeclaration() || r.inSystemHeader || r.scopes < 0) return false;
    for (const Reference& d : e.references) {
      if (d.isDeclaration() && d.pos == r.pos) return false;
    }
    std::size_t off = nameOffset(xr, r.pos, name);
    if (off == std::string::npos) return false;
    const SourceText& st = sourceText(r.pos.file);
    if (isQualified(st, off)) return false;
    kind = lookupKind(st, off, name.size());
    return true;
  };
  // (Where lookup finds both a renamed entity and another one, they are declared in the same
  // scope, which is reported above.)
  std::set<Position> seen;
  for (const Entity* e : renamedAll) {
    if (e->kind == Kind::Macro || e->kind == Kind::Label) continue;
    for (const Reference& r : e->references) {
      LookupKind kind;
      if (!unqualified(*e, r, oldName, kind) || !seen.insert(r.pos).second) continue;
      std::vector<const Entity*> found = lookup.at(r, *e, kind);
      if (found.empty() || std::any_of(found.begin(), found.end(), [&](const Entity* f) { return targets.count(f); })) {
        continue;
      }
      report(where(r.pos) + ": here " + newName + " would refer to " + describe(*found[0]) + declaredAt(*found[0]));
    }
  }
  seen.clear();
  for (const Entity* x : xr.named(newName)) {
    if (targets.count(x) || x->isImplicit || x->kind == Kind::Macro || x->kind == Kind::Label) continue;
    for (const Reference& r : x->references) {
      LookupKind kind;
      if (!unqualified(*x, r, newName, kind) || !seen.insert(r.pos).second) continue;
      std::vector<const Entity*> found = lookup.at(r, *x, kind);
      if (std::find(found.begin(), found.end(), x) != found.end()) continue;
      for (const Entity* e : found) {
        if (!targets.count(e)) continue;
        report(where(r.pos) + ": here " + newName + " would refer to the renamed " + describe(*e) + " instead of " +
               describe(*x));
        break;
      }
    }
  }
  if (reported.size() > 20) {
    std::cerr << "rose-ren: " << severity << "and " << reported.size() - 20 << " more\n";
  }
  return problems;
}

// The virtual functions related to e by overriding (in both directions), e included
void overrideFamily(const CrossReferences& xr, const Entity* e, std::set<const Entity*>& family) {
  std::vector<const Entity*> work{e};
  while (!work.empty()) {
    const Entity* f = work.back();
    work.pop_back();
    if (!family.insert(f).second) continue;
    for (EntityId o : f->overrides) {
      if (const Entity* oe = xr.entity(o)) work.push_back(oe);
    }
    for (const auto& kv : xr.entities()) {
      const Entity& g = kv.second;
      if (std::find(g.overrides.begin(), g.overrides.end(), f->id) != g.overrides.end()) work.push_back(&g);
    }
    for (const Entity* s : xr.sameDeclaration(*f)) work.push_back(s);
  }
}

int rename(const CrossReferences& xr, const std::vector<Item>& items, const std::string& oldName, int index,
           const std::string& newName, bool dryRun, bool force) {
  if (items.empty()) {
    std::cerr << "rose-ren: no entity called " << oldName << " is declared outside system headers\n";
    return 1;
  }
  if (index < 0) {
    if (items.size() > 1) {
      std::cerr << "rose-ren: " << items.size() << " entities are called " << oldName
                << "; choose one with " << oldName << ":index\n";
      list(items, oldName);
      return 1;
    }
    index = 0;
  }
  if (index >= (int)items.size()) {
    std::cerr << "rose-ren: there is no entity " << oldName << ":" << index << "\n";
    list(items, oldName);
    return 1;
  }
  if (!isIdentifier(newName) || isKeyword(newName)) {
    std::cerr << "rose-ren: " << newName << " is not a valid identifier\n";
    return 1;
  }
  const Item& item = items[index];

  // The entities to rename
  std::set<const Entity*> targets(item.entities.begin(), item.entities.end());
  bool virtualFunction = false;
  for (const Entity* e : item.entities) virtualFunction |= e->isVirtual;
  if (virtualFunction) {
    std::set<const Entity*> family;
    for (const Entity* e : item.entities) overrideFamily(xr, e, family);
    targets.insert(family.begin(), family.end());
  }
  // A class is renamed with its constructors and destructor
  std::set<EntityId> classes;
  for (const Entity* e : targets) {
    if (e->kind == Kind::Class || e->kind == Kind::Struct || e->kind == Kind::Union || e->kind == Kind::ClassTemplate) {
      classes.insert(e->id);
    }
  }
  if (!classes.empty()) {
    for (const auto& kv : xr.entities()) {
      const Entity& g = kv.second;
      if (isConstructorOrDestructor(xr, g) &&
          (classes.count(g.parent) || (g.scope.kind == Scope::Kind::Class && classes.count(g.scope.id)))) {
        targets.insert(&g);
      }
    }
  }

  int problems = 0;
  for (const Entity* e : targets) {
    if (e->declaredInSystemHeader() && e->name == oldName) {
      std::cerr << "rose-ren: " << e->qualifiedName << " is declared in a system header ("
                << displayName(e->references.front().pos.file) << ") and cannot be renamed\n";
      ++problems;
    }
  }

  // Where the new name would clash with, hide or be hidden by another entity
  problems += nameClashes(xr, targets, oldName, newName, force);

  // The names to replace
  Edits edits;
  std::set<Position> done;
  std::set<std::string> files;
  int count = 0, systemRefs = 0;
  for (const Entity* e : targets) {
    for (const Reference& r : e->references) {
      if (!done.insert(r.pos).second) continue;
      if (r.inSystemHeader) {
        if (!r.isDeclaration() && e->name == oldName) ++systemRefs;
        continue;
      }
      SourceText& st = sourceText(r.pos.file);
      std::string macro;
      std::size_t off = nameOffset(xr, r.pos, oldName, &macro);
      if (off == std::string::npos) {
        // A name in the definition of a macro cannot be renamed; other references are not
        // written with the name (an implicit constructor call, for example)
        if (!macro.empty() && e->name == oldName) {
          std::cerr << "rose-ren: " << (force ? "warning: " : "") << displayName(r.pos.file) << ":" << r.pos.line
                    << ":" << r.pos.column << ": " << oldName << " is in the definition of the macro " << macro
                    << ", which is not changed\n";
          if (!force) ++problems;
        } else if (r.isDeclaration() && e->name == oldName && !e->isImplicit) {
          std::cerr << "rose-ren: " << (force ? "warning: " : "") << displayName(r.pos.file) << ":" << r.pos.line
                    << ":" << r.pos.column << ": the declaration is not written as " << oldName << "\n";
          if (!force) ++problems;
        }
        continue;
      }
      // Another entity referenced by the same name here (e.g. in a template, for other template
      // arguments) would be renamed too
      for (const Entity* other : xr.at(r.pos)) {
        if (targets.count(other) || other->name != oldName || other->isImplicit || !other->isDeclared()) continue;
        std::cerr << "rose-ren: " << (force ? "warning: " : "") << displayName(r.pos.file) << ":" << r.pos.line
                  << ":" << r.pos.column << ": this " << oldName << " also refers to " << describe(*other) << "\n";
        if (!force) ++problems;
      }
      edits.replace(r.pos.file, off, off + oldName.size(), newName);
      files.insert(r.pos.file);
      ++count;
      if (dryRun) {
        std::cout << displayName(r.pos.file) << ":" << r.pos.line << ":" << r.pos.column << ": " << st.line(r.pos.line)
                  << "\n";
      }
    }
  }
  if (systemRefs > 0) {
    std::cerr << "rose-ren: warning: " << oldName << " is also referenced " << systemRefs
              << " times in system headers (e.g. by library templates), which are not changed\n";
  }
  // The other uses of the name that the front end did not resolve (a local variable or a
  // parameter cannot be used elsewhere)
  bool local = false;
  for (const Entity* e : item.entities) local |= e->isLocal || e->kind == Kind::Parameter;
  std::vector<Position> unresolved;
  if (!local) unresolved = unresolvedOccurrences(xr, oldName);
  if (!unresolved.empty()) {
    std::cerr << "rose-ren: warning: not renamed: " << unresolved.size() << (unresolved.size() == 1 ? " use" : " uses")
              << " of " << oldName << " that the front end did not resolve (in code that the preprocessor skips, "
              << "in macro definitions, members of template parameters"
              << (frontEndOptionsUsed() > 0 ? ", templates that are not instantiated" : "") << "):\n";
    for (const Position& p : unresolved) {
      std::cerr << "  " << displayName(p.file) << ":" << p.line << ":" << p.column << ": "
                << sourceText(p.file).line(p.line) << "\n";
    }
  }
  if (problems > 0) {
    std::cerr << "rose-ren: nothing changed (use --force to rename anyway)\n";
    return 1;
  }
  if (count == 0) {
    std::cerr << "rose-ren: no reference to rename\n";
    return 1;
  }
  if (!dryRun) {
    std::string error;
    if (!edits.write(&error)) {
      std::cerr << "rose-ren: " << error << "\n";
      return 1;
    }
  }
  std::cout << (dryRun ? "would rename " : "renamed ") << describe(*item.main) << " to " << newName << ": " << count
            << (count == 1 ? " occurrence" : " occurrences") << " in " << files.size()
            << (files.size() == 1 ? " file" : " files") << "\n";
  return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  ROSE_INITIALIZE;
  bool dryRun = false, force = false;
  std::vector<char*> args;
  for (int i = 0; i < argc; ++i) {
    if (std::strcmp(argv[i], "--dry-run") == 0) {
      dryRun = true;
    } else if (std::strcmp(argv[i], "--force") == 0) {
      force = true;
    } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
      std::cout << usage;
      return 0;
    } else {
      args.push_back(argv[i]);
    }
  }
  std::vector<std::string> feArgs, toolArgs;
  splitCommandLine((int)args.size(), args.data(), feArgs, toolArgs);
  if (feArgs.size() < 2 || toolArgs.empty() || toolArgs.size() > 2) {
    std::cerr << usage;
    return 2;
  }
  std::string oldName = toolArgs[0];
  int index = -1;
  std::size_t colon = oldName.rfind(':');
  if (colon != std::string::npos && colon + 1 < oldName.size() && std::isdigit((unsigned char)oldName[colon + 1])) {
    index = std::atoi(oldName.c_str() + colon + 1);
    oldName = oldName.substr(0, colon);
  }

  recordCrossReferences();
  buildAst(false);  // only the cross-references are needed
  // The bodies of all templates are parsed (by default, as GCC does, only where they are
  // instantiated); if the front end rejects the source, it is parsed as GCC does, then as Visual
  // C++ does (names in templates are also looked up in dependent base classes)
  setFrontEndOptions({"--no_defer_parse_function_templates"});
  setAlternativeFrontEndOptions({{}, {"--no_dep_name", "--no_parse_templates"}});
  feArgs.push_back("-rose:skipfinalCompileStep");
  SgProject* project = frontend(feArgs);
  if (project == nullptr || project->get_frontendErrorCode() != 0) {
    std::cerr << "rose-ren: the source could not be parsed\n";
    return 1;
  }
  if (frontEndOptionsUsed() > 0) {
    std::cerr << "rose-ren: note: parsed as "
              << (microsoftMode() ? "Visual C++ does without /permissive-" : frontEndOptionsUsed() == 1 ? "GCC does" : "Visual C++ does")
              << ": templates are only parsed where they are instantiated\n";
  }
  const CrossReferences& xr = crossReferences();
  if (xr.empty()) {
    std::cerr << "rose-ren: no cross-reference information\n";
    return 1;
  }
  std::vector<Item> items = itemsNamed(xr, oldName);
  if (toolArgs.size() == 1) {
    list(items, oldName);
    return items.empty() ? 1 : 0;
  }
  return rename(xr, items, oldName, index, toolArgs[1], dryRun, force);
}
