// Cross-references for refactoring tools (refactor/RoseRefactor.h).
//
// When they are recorded, the EDG front end writes its cross-reference listing (--xref): one
// record per reference to a symbol, with the position of the name.  A copy of EDG's
// symbol_ref.c (edg2sage/patches/symbol_ref.c.sed) calls onReference() below for each record,
// while the symbol is in memory (symbols can be freed before the back end runs): the symbol's
// IL entity identifies the entity, so that the symbols of one entity (e.g. an injected class
// name and the class) are one entity.  At the end of back_end(), while the IL is still in
// memory, each entity is described from its IL entity (kind, type, enclosing class, access,
// base classes, overridden virtual functions, ...).
#include "edg2sage.h"
#include "cmd_line.h"
#include "il_to_str.h"
#include "symbol_tbl.h"
#include "RoseRefactorImpl.h"

#include <algorithm>
#include <cstdio>
#include <set>
#include <cstdlib>
#include <fstream>

using namespace edg;
namespace RR = RoseRefactor;

namespace edg {
// In the patched copy of symbol_ref.c
extern void (*edg2sage_xref_hook)(a_symbol_ptr, char, a_const_char*, a_line_number, int);
}  // namespace edg

namespace edg2sage {

namespace {

std::string listingFile;  // the cross-reference listing of the current front end run

// What is recorded while the front end runs
struct Pending {
  int symbolKind = 0;
  char* il = nullptr;
  an_il_entry_kind iek = iek_none;
  std::string name, qualifiedName;
};
struct Record {
  RR::EntityId id;
  char code;
  std::string file;
  unsigned long line;
  int column;
};
std::map<RR::EntityId, Pending> pending;
std::vector<Record> records;
std::map<a_symbol_ptr, std::pair<RR::EntityId, std::string>> symbolEntities;
RR::EntityId nextSyntheticId = 1;

// Formatted file names (as in the listing) -> absolute name, and whether a system header
struct FileInfo {
  std::string name;
  bool system = false;
};
std::map<std::string, FileInfo> files;

void addFiles(a_source_file_ptr sf, bool system) {
  for (; sf != nullptr; sf = sf->next) {
    bool sys = system || sf->from_system_include_dir || sf->included_by_system_include;
    if (sf->file_name != nullptr) {
      FileInfo info;
      info.name = absolutePath(sf->file_name);
      info.system = sys;
      files[format_file_name(sf->file_name)] = info;
      files[sf->file_name] = info;
      if (sf->full_name != nullptr) files[sf->full_name] = info;
    }
    addFiles(sf->first_child_file, sys);
  }
}

const FileInfo& fileInfo(const std::string& listed) {
  auto it = files.find(listed);
  if (it == files.end()) {
    FileInfo info;
    info.name = absolutePath(listed);
    it = files.emplace(listed, info).first;
  }
  return it->second;
}

void appendText(a_const_char* str, an_il_to_str_output_control_block_ptr octl) {
  static_cast<std::string*>(octl->text_buffer)->append(str);
}

std::string typeText(a_type_ptr type) {
  std::string text;
  if (type == nullptr) return text;
  an_il_to_str_output_control_block octl;
  clear_il_to_str_output_control_block(&octl);
  octl.output_str = appendText;
  octl.text_buffer = &text;
  form_type(type, &octl);
  return text;
}

RR::Access accessOf(a_source_correspondence* sc) {
  if (sc == nullptr || !sc->is_class_member) return RR::Access::None;
  switch ((an_access_specifier)sc->access) {
    case as_public:
      return RR::Access::Public;
    case as_protected:
      return RR::Access::Protected;
    case as_private:
      return RR::Access::Private;
    default:
      return RR::Access::None;
  }
}

RR::EntityId parentOf(a_source_correspondence* sc) {
  if (sc == nullptr || sc->parent_scope == nullptr) return 0;
  a_scope_ptr scope = sc->parent_scope;
  switch (scope->kind) {
    case sck_class_struct_union: {
      // The members of the generic class of a class template (its prototype instantiation) are
      // members of the template
      a_type_ptr t = scope->variant.assoc_type;
      if (t != nullptr && t->variant.class_struct_union.is_prototype_instantiation &&
          t->variant.class_struct_union.extra_info != nullptr &&
          t->variant.class_struct_union.extra_info->assoc_template != nullptr) {
        return (RR::EntityId)(uintptr_t)t->variant.class_struct_union.extra_info->assoc_template;
      }
      return (RR::EntityId)(uintptr_t)t;
    }
    case sck_namespace:
      return (RR::EntityId)(uintptr_t)scope->variant.assoc_namespace;
    default:
      return 0;
  }
}

RR::Position positionOf(const a_source_position& pos) {
  RR::Position p;
  if (pos.seq == 0 || translator == nullptr) return p;
  a_line_number line = 0;
  std::string name = translator->fileNameOf(pos.seq, &line);
  if (name.empty() || line == 0) return p;
  p.file = absolutePath(name);
  p.line = (int)line;
  p.column = (int)pos.column;
  return p;
}

bool isNonreal(a_type_ptr cls) {
  a_type_ptr t = skip_typerefs(cls);
  return t != nullptr && (t->kind == tk_class || t->kind == tk_struct || t->kind == tk_union) &&
         (t->variant.class_struct_union.is_nonreal_class || t->variant.class_struct_union.is_prototype_instantiation);
}

bool isInstance(a_type_ptr cls) {
  a_type_ptr t = skip_typerefs(cls);
  return t != nullptr && (t->kind == tk_class || t->kind == tk_struct || t->kind == tk_union) &&
         t->variant.class_struct_union.is_template_class && !isNonreal(t);
}

a_type_ptr parentClass(a_source_correspondence* sc) {
  if (sc == nullptr || sc->parent_scope == nullptr || sc->parent_scope->kind != sck_class_struct_union) return nullptr;
  return sc->parent_scope->variant.assoc_type;
}

void appendName(a_const_char* str, an_il_to_str_output_control_block_ptr octl) {
  static_cast<std::string*>(octl->text_buffer)->append(str);
}

// Whether a symbol stands for an IL entity
bool hasILEntry(a_symbol_ptr sym) {
  switch (sym->kind) {
    case sk_constant:
    case sk_type:
    case sk_class_or_struct_tag:
    case sk_union_tag:
    case sk_enum_tag:
    case sk_variable:
    case sk_field:
    case sk_static_data_member:
    case sk_member_function:
    case sk_routine:
    case sk_label:
    case sk_namespace:
    case sk_class_template:
    case sk_function_template:
    case sk_variable_template:
    case sk_concept_template:
      return true;
    default:
      return false;
  }
}

// Fills in what the IL entity of a symbol tells about it
void describe(RR::Entity& e, int symbolKind, char* il, an_il_entry_kind iek) {
  a_source_correspondence* sc = nullptr;
  switch ((a_symbol_kind)symbolKind) {
    case sk_namespace:
      e.kind = RR::Kind::Namespace;
      break;
    case sk_class_or_struct_tag:
      e.kind = RR::Kind::Class;
      break;
    case sk_union_tag:
      e.kind = RR::Kind::Union;
      break;
    case sk_enum_tag:
      e.kind = RR::Kind::Enum;
      break;
    case sk_type:
      e.kind = RR::Kind::Typedef;
      break;
    case sk_constant:
      e.kind = RR::Kind::Enumerator;
      break;
    case sk_variable:
      e.kind = RR::Kind::Variable;
      break;
    case sk_parameter:
      e.kind = RR::Kind::Parameter;
      break;
    case sk_field:
      e.kind = RR::Kind::Field;
      break;
    case sk_static_data_member:
      e.kind = RR::Kind::StaticDataMember;
      e.isStatic = true;
      break;
    case sk_member_function:
      e.kind = RR::Kind::MemberFunction;
      break;
    case sk_routine:
      e.kind = RR::Kind::Function;
      break;
    case sk_label:
      e.kind = RR::Kind::Label;
      break;
    case sk_macro:
      e.kind = RR::Kind::Macro;
      break;
    case sk_class_template:
      e.kind = RR::Kind::ClassTemplate;
      break;
    case sk_function_template:
      e.kind = RR::Kind::FunctionTemplate;
      break;
    case sk_variable_template:
      e.kind = RR::Kind::VariableTemplate;
      break;
    case sk_concept_template:
      e.kind = RR::Kind::Concept;
      break;
    default:
      e.kind = RR::Kind::Other;
      break;
  }
  if (il == nullptr) return;
  switch (iek) {
    case iek_type: {
      a_type_ptr t = (a_type_ptr)il;
      sc = &t->source_corresp;
      if (t->kind == tk_class || t->kind == tk_struct || t->kind == tk_union) {
        if (e.kind != RR::Kind::ClassTemplate) {
          e.kind = t->kind == tk_class ? RR::Kind::Class : t->kind == tk_struct ? RR::Kind::Struct : RR::Kind::Union;
        }
        e.isInTemplate = isNonreal(t);
        e.isTemplateInstance = isInstance(t);
        a_class_type_supplement_ptr ctsp = t->variant.class_struct_union.extra_info;
        for (a_base_class_ptr bc = ctsp ? ctsp->direct_base_classes : nullptr; bc != nullptr; bc = bc->next_direct) {
          if (!bc->direct) continue;
          RR::BaseClass b;
          b.entity = (RR::EntityId)(uintptr_t)skip_typerefs(bc->type);
          an_access_specifier access = bc->derivation != nullptr ? bc->derivation->access : as_public;
          b.access = access == as_private ? RR::Access::Private
                     : access == as_protected ? RR::Access::Protected
                                              : RR::Access::Public;
          b.isVirtual = bc->is_virtual;
          b.start = positionOf(bc->base_specifier_range.start);
          b.end = positionOf(bc->base_specifier_range.end);
          e.bases.push_back(b);
        }
      } else if (t->kind == tk_enum) {
        e.kind = RR::Kind::Enum;
      } else if (t->kind == tk_template_param) {
        e.kind = RR::Kind::TemplateParameter;
      } else if (t->kind == tk_typeref) {
        if (e.kind == RR::Kind::Typedef || e.kind == RR::Kind::Other) e.kind = RR::Kind::Typedef;
        e.type = typeText(t->variant.typeref.type);
      }
      break;
    }
    case iek_variable: {
      a_variable_ptr v = (a_variable_ptr)il;
      sc = &v->source_corresp;
      e.type = typeText(v->type);
      if (e.kind != RR::Kind::Parameter && e.kind != RR::Kind::StaticDataMember) e.kind = RR::Kind::Variable;
      if (v->is_parameter) e.kind = RR::Kind::Parameter;
      if (v->source_corresp.is_class_member) {
        e.kind = RR::Kind::StaticDataMember;
        e.isStatic = true;
      }
      break;
    }
    case iek_field: {
      a_field_ptr f = (a_field_ptr)il;
      sc = &f->source_corresp;
      e.type = typeText(f->type);
      e.kind = RR::Kind::Field;
      break;
    }
    case iek_routine: {
      a_routine_ptr r = (a_routine_ptr)il;
      sc = &r->source_corresp;
      e.type = typeText(r->type);
      if (r->source_corresp.is_class_member) {
        e.kind = r->special_kind == sfk_constructor  ? RR::Kind::Constructor
                 : r->special_kind == sfk_destructor ? RR::Kind::Destructor
                                                     : RR::Kind::MemberFunction;
        e.isVirtual = r->is_virtual;
        e.isPureVirtual = r->pure_virtual;
        a_type_ptr rt = skip_typerefs(r->type);
        if (rt != nullptr && rt->kind == tk_routine && rt->variant.routine.extra_info != nullptr) {
          e.isConst = (rt->variant.routine.extra_info->qualifiers & TQ_CONST) != 0;
          // A static member function has no "this"
          e.isStatic = rt->variant.routine.extra_info->this_class == nullptr;
        }
      } else if (e.kind != RR::Kind::FunctionTemplate) {
        e.kind = RR::Kind::Function;
      }
      e.isImplicit = r->compiler_generated;
      if (r->is_prototype_instantiation) e.isInTemplate = true;
      break;
    }
    case iek_constant: {
      a_constant_ptr c = (a_constant_ptr)il;
      sc = &c->source_corresp;
      e.type = typeText(c->type);
      if (e.kind == RR::Kind::Other) e.kind = RR::Kind::Enumerator;
      break;
    }
    case iek_namespace: {
      a_namespace_ptr n = (a_namespace_ptr)il;
      sc = &n->source_corresp;
      e.kind = RR::Kind::Namespace;
      break;
    }
    case iek_label: {
      e.kind = RR::Kind::Label;
      break;
    }
    default:
      break;
  }
  if (sc != nullptr) {
    if (sc->name != nullptr) e.name = sc->name;
    e.parent = parentOf(sc);
    e.access = accessOf(sc);
    e.isLocal = sc->is_local_to_function;
    a_type_ptr pc = parentClass(sc);
    if (pc != nullptr) {
      if (isNonreal(pc)) e.isInTemplate = true;
      if (isInstance(pc)) e.isTemplateInstance = true;
    }
  }
}

// The parameter list and qualifiers of a function type ("(int, double) const" of
// "void (int, double) const")
std::string signatureOf(const std::string& type) {
  int depth = 0;
  for (std::size_t i = 0; i < type.size(); ++i) {
    char c = type[i];
    if (c == '<' || c == '[') ++depth;
    if (c == '>' || c == ']') --depth;
    if (c == '(' && depth == 0) {
      // "(" of the parameter list, unless it is the declarator of a returned function pointer
      return type.substr(i);
    }
  }
  return type;
}

// The member functions that override virtual functions of base classes: those with the same
// name, parameter types and qualifiers
void addOverrides(RR::CrossReferences& xr) {
  std::multimap<RR::EntityId, const RR::Entity*> members;
  for (const auto& kv : xr.entities()) {
    const RR::Entity& e = kv.second;
    if (e.kind == RR::Kind::MemberFunction || e.kind == RR::Kind::Destructor) members.emplace(e.parent, &e);
  }
  for (const auto& kv : xr.entities()) {
    const RR::Entity& f = kv.second;
    if (f.kind != RR::Kind::MemberFunction && f.kind != RR::Kind::Destructor) continue;
    if (!f.isVirtual || f.parent == 0) continue;
    const RR::Entity* cls = xr.entity(f.parent);
    if (cls == nullptr) continue;
    std::string sig = signatureOf(f.type);
    // The bases of the class, transitively
    std::set<RR::EntityId> seen;
    std::vector<RR::EntityId> work;
    for (const RR::BaseClass& b : cls->bases) work.push_back(b.entity);
    while (!work.empty()) {
      RR::EntityId b = work.back();
      work.pop_back();
      if (!seen.insert(b).second) continue;
      auto range = members.equal_range(b);
      for (auto it = range.first; it != range.second; ++it) {
        const RR::Entity* g = it->second;
        if (!g->isVirtual) continue;
        bool same = f.kind == RR::Kind::Destructor ? g->kind == RR::Kind::Destructor
                                                   : g->name == f.name && signatureOf(g->type) == sig;
        if (!same) continue;
        std::vector<RR::EntityId>& v = const_cast<RR::Entity&>(f).overrides;
        if (std::find(v.begin(), v.end(), g->id) == v.end()) v.push_back(g->id);
      }
      if (const RR::Entity* be = xr.entity(b)) {
        for (const RR::BaseClass& bb : be->bases) work.push_back(bb.entity);
      }
    }
  }
}

}  // namespace

// Whether back_end() builds the AST (refactoring tools may only need the cross-references)
bool xrefBuildsAst() { return RR::impl::buildsAst(); }

namespace {

// Called (by the patched symbol_ref.c) for each record of the cross-reference listing
void onReference(a_symbol_ptr sym, char code, a_const_char* fileName, a_line_number line, int column) {
  if (sym == nullptr || fileName == nullptr) return;
  if (sym->kind == sk_keyword || sym->kind == sk_undefined) return;
  an_il_entry_kind iek = iek_none;
  char* il = hasILEntry(sym) ? il_entry_for_symbol_null_okay(sym, &iek) : nullptr;
  std::string name = sym->header != nullptr && sym->header->identifier != nullptr ? sym->header->identifier : "";
  RR::EntityId id;
  if (il != nullptr) {
    id = (RR::EntityId)(uintptr_t)il;
  } else {
    // Symbols without an IL entity (macros): the memory of a symbol can be reused for another
    std::pair<RR::EntityId, std::string>& se = symbolEntities[sym];
    if (se.first == 0 || se.second != name) {
      se.first = nextSyntheticId++;
      se.second = name;
    }
    id = se.first;
  }
  if (pending.find(id) == pending.end()) {
    Pending& p = pending[id];
    p.symbolKind = sym->kind;
    p.il = il;
    p.iek = iek;
    p.name = name;
    an_il_to_str_output_control_block octl;
    clear_il_to_str_output_control_block(&octl);
    octl.output_str = appendName;
    octl.text_buffer = &p.qualifiedName;
    octl.keep_template_typedefs = FALSE;
    octl.render_auto_deduction_typerefs = TRUE;
    form_symbol_name(sym, &octl);
  }
  records.push_back(Record{id, code, fileName, (unsigned long)line, column});
}

}  // namespace

int xrefRuns() { return RR::impl::frontEndRuns(); }

void xrefRunAccepted(int run) { RR::impl::setFrontEndOptionsUsed(run); }

// The options for the EDG front end of the next run (called by edgCommandLine()); those about
// templates are only for C++
void xrefOptions(std::vector<std::string>& args, int run, bool cplusplus) {
  static const std::set<std::string> cplusplusOnly = {"--no_defer_parse_function_templates", "--no_dep_name",
                                                      "--no_parse_templates", "--no_ms_permissive"};
  for (const std::string& o : RR::impl::frontEndOptions(run)) {
    if (cplusplus || !cplusplusOnly.count(o)) args.push_back(o);
  }
  listingFile.clear();
  pending.clear();
  records.clear();
  symbolEntities.clear();
  RR::impl::current().clear();
  edg2sage_xref_hook = nullptr;
  if (!RR::impl::recording()) return;
  listingFile = RR::impl::temporaryFile("rose-xref");
  if (listingFile.empty()) return;
  RR::impl::removeAtExit(listingFile);
  args.push_back("--xref");
  args.push_back(listingFile);
  // The templates that are used are instantiated, so that the references in their bodies are
  // recorded too
  if (cplusplus) {
    args.push_back("--instantiate");
    args.push_back("used");
  }
  edg2sage_xref_hook = onReference;
}

// Describes the entities of the records (called at the end of back_end())
void xrefCollect() {
  edg2sage_xref_hook = nullptr;
  if (listingFile.empty()) return;
  files.clear();
  addFiles(il_header.primary_source_file, false);

  RR::CrossReferences& xr = RR::impl::current();
  for (auto& kv : pending) {
    const Pending& p = kv.second;
    RR::Entity& e = xr.add(kv.first);
    e.qualifiedName = p.qualifiedName;
    e.name = p.name;
    describe(e, p.symbolKind, p.il, p.iek);
    if (e.name.empty()) {
      std::size_t colons = p.qualifiedName.rfind("::");
      e.name = colons == std::string::npos ? p.qualifiedName : p.qualifiedName.substr(colons + 2);
    }
  }
  unsigned order = 0;
  for (const Record& r : records) {
    RR::Entity* e = const_cast<RR::Entity*>(xr.entity(r.id));
    if (e == nullptr) continue;
    RR::Reference ref;
    ref.order = order++;
    const FileInfo& fi = fileInfo(r.file);
    ref.pos = RR::Position(fi.name, (int)r.line, r.column);
    ref.code = r.code;
    ref.inSystemHeader = fi.system;
    e->references.push_back(ref);
  }
  pending.clear();
  records.clear();
  symbolEntities.clear();
  addOverrides(xr);
  xr.finish();
  // ROSE_REFACTOR_DUMP: lists the entities declared outside system headers (for debugging)
  if (std::getenv("ROSE_REFACTOR_DUMP") != nullptr) {
    for (const auto& kv : xr.entities()) {
      const RR::Entity& e = kv.second;
      if (e.declaredInSystemHeader()) continue;
      std::fprintf(stderr, "%llx %s [%s] %s type='%s' parent=%llx access=%s%s%s%s%s%s\n",
                   (unsigned long long)e.id, e.qualifiedName.c_str(), e.name.c_str(), RR::kindName(e.kind),
                   e.type.c_str(), (unsigned long long)e.parent, RR::accessName(e.access),
                   e.isStatic ? " static" : "", e.isVirtual ? " virtual" : "", e.isInTemplate ? " in-template" : "",
                   e.isTemplateInstance ? " instance" : "", e.isImplicit ? " implicit" : "");
      for (const RR::BaseClass& b : e.bases) {
        std::fprintf(stderr, "    base %llx %s %s..%s\n", (unsigned long long)b.entity, RR::accessName(b.access),
                     b.start.str().c_str(), b.end.str().c_str());
      }
      for (RR::EntityId o : e.overrides) std::fprintf(stderr, "    overrides %llx\n", (unsigned long long)o);
      for (const RR::Reference& r : e.references) {
        std::fprintf(stderr, "    %c %s%s\n", r.code, r.pos.str().c_str(), r.inSystemHeader ? " (system)" : "");
      }
    }
  }
}

}  // namespace edg2sage
