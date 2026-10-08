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
//   --force     rename even where the new name may clash or another entity shares a reference
//
// Renaming changes the source files in place: the source file and the headers it includes,
// except system headers.  The positions of the names come from the EDG front end's
// cross-reference listing, so names are found in all their uses (including qualified names,
// using-declarations, base classes, template arguments and template instances); a class is
// renamed with its constructors and destructor, a virtual function with the functions that
// override it or that it overrides.
#include "rose.h"
#include "RoseRefactor.h"

#include <cctype>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <set>

using namespace RoseRefactor;

namespace {

const char* usage =
    "usage: rose-ren [options] source.cpp name                list the entities called name\n"
    "       rose-ren [options] source.cpp name[:index] new    rename one of them to new\n"
    "options: the compiler's (-I, -D, -std=...), --dry-run, --force\n";

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

// The entities called name that are declared outside system headers, grouped
std::vector<Item> itemsNamed(const CrossReferences& xr, const std::string& name) {
  std::vector<Item> items;
  std::set<EntityId> done;
  for (const Entity* e : xr.named(name)) {
    // (constructors and destructors are renamed with their class)
    if (done.count(e->id) || e->isImplicit || e->kind == Kind::Constructor || e->kind == Kind::Destructor) continue;
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
      if ((g.kind == Kind::Constructor || g.kind == Kind::Destructor) && classes.count(g.parent)) targets.insert(&g);
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

  // New name clashes: entities called newName in the same class or namespace
  std::set<EntityId> parents;
  for (const Entity* e : targets) {
    if (e->name == oldName && !e->isLocal && e->kind != Kind::Parameter) parents.insert(e->parent);
  }
  for (const Entity* e : xr.named(newName)) {
    if (e->isImplicit || e->isLocal || e->kind == Kind::Parameter) continue;
    if (parents.count(e->parent)) {
      Position p = e->declaration();
      std::cerr << "rose-ren: " << (force ? "warning: " : "") << newName << " is already declared there: "
                << describe(*e);
      if (p.valid()) std::cerr << " at " << displayName(p.file) << ":" << p.line;
      std::cerr << "\n";
      if (!force) ++problems;
    }
  }

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
  feArgs.push_back("-rose:skipfinalCompileStep");
  SgProject* project = frontend(feArgs);
  if (project == nullptr || project->get_frontendErrorCode() != 0) {
    std::cerr << "rose-ren: the source could not be parsed\n";
    return 1;
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
