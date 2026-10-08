// Cross-references for refactoring tools (refactor/RoseRefactor.h).
//
// When they are recorded, the EDG front end writes its cross-reference listing (--xref): one
// record per reference to a symbol, with the position of the name.  A copy of EDG's
// symbol_ref.c (edg2sage/patches/symbol_ref.c.sed) calls onReference() below for each record,
// while the symbol is in memory (symbols can be freed before the back end runs): the symbol's
// IL entity identifies the entity, so that the symbols of one entity (e.g. an injected class
// name and the class) are one entity.  At the end of back_end(), while the IL is still in
// memory, each entity is described from its IL entity (kind, type, enclosing class, access,
// base classes, overridden virtual functions, ...).  Each record also notes the scopes on the
// front end's scope stack (the scopes in which an unqualified name is looked up there), and each
// entity the scope it is declared in, so that tools can tell where a name would refer to another
// entity.
#include "edg2sage.h"
#include "cmd_line.h"
#include "il_to_str.h"
#include "scope_stk.h"
#include "symbol_ref.h"
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
extern void (*edg2sage_xref_hook)(a_symbol_ptr, char, a_const_char*, a_line_number, int, a_symbol_reference_kind);
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
  int scopes;  // CrossReferences::scopes() index
};
std::map<RR::EntityId, Pending> pending;
std::vector<Record> records;
std::map<a_symbol_ptr, std::pair<RR::EntityId, std::string>> symbolEntities;
// The blocks that are in the declarative region of an enclosing scope (scope numbers)
std::map<a_scope_number, a_scope_number> regions;
// The class types of the class scopes in the scopes of the records (by entity)
std::map<RR::EntityId, a_type_ptr> classTypes;
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

// The entity of a class (as parentOf())
RR::EntityId classEntity(a_type_ptr t) {
  if (t != nullptr && (t->kind == tk_class || t->kind == tk_struct || t->kind == tk_union) &&
      t->variant.class_struct_union.is_prototype_instantiation &&
      t->variant.class_struct_union.extra_info != nullptr &&
      t->variant.class_struct_union.extra_info->assoc_template != nullptr) {
    return (RR::EntityId)(uintptr_t)t->variant.class_struct_union.extra_info->assoc_template;
  }
  return (RR::EntityId)(uintptr_t)t;
}

// Whether a class is an anonymous union (or structure), whose members are declared in the
// enclosing scope
bool isAnonymousClass(a_type_ptr t) {
  return t != nullptr && (t->kind == tk_class || t->kind == tk_struct || t->kind == tk_union) &&
         (t->variant.class_struct_union.is_nonstd_anonymous_union_type ||
          (t->variant.class_struct_union.extra_info != nullptr &&
           t->variant.class_struct_union.extra_info->anonymous_union_kind != auk_none));
}

// The scope that an IL scope entry stands for (unknown for the members of an anonymous union,
// whose scope is that of their declaration in the scope stack: see xrefCollect())
RR::Scope scopeOf(a_scope_ptr scope) {
  RR::Scope s;
  if (scope == nullptr) return s;
  switch (scope->kind) {
    case sck_file:
      s.kind = RR::Scope::Kind::Global;
      break;
    case sck_namespace:
      s.kind = RR::Scope::Kind::Namespace;
      s.id = (RR::EntityId)(uintptr_t)scope->variant.assoc_namespace;
      break;
    case sck_class_struct_union:
    case sck_enum:
      if (scope->kind == sck_class_struct_union && isAnonymousClass(scope->variant.assoc_type)) break;
      s.kind = RR::Scope::Kind::Class;
      s.id = classEntity(scope->variant.assoc_type);
      break;
    case sck_function:
      s.kind = RR::Scope::Kind::Function;
      s.id = (std::uint64_t)scope->number;
      break;
    default: {
      s.kind = RR::Scope::Kind::Local;
      auto r = regions.find(scope->number);
      s.id = (std::uint64_t)(r != regions.end() ? r->second : scope->number);
      break;
    }
  }
  return s;
}

// Whether a scope of the scope stack declares the variable of a range-based for statement
bool declaresRangeForVariable(a_scope_stack_entry* ssep) {
  for (a_symbol_ptr sym = assoc_pointers_block_of(ssep)->symbols; sym != nullptr; sym = sym->next_in_scope) {
    if (sym->kind == sk_variable && sym->variant.variable.ptr != nullptr &&
        sym->variant.variable.ptr->is_enhanced_for_iterator) {
      return true;
    }
  }
  return false;
}

// The number of the scope whose declarative region the scope at this depth of the scope stack
// is in.  In C++, the outermost block of a statement controlled by a condition is in the region
// of the condition; the condition and the outermost block of the body of a for statement in that
// of its init-statement; the outermost block of the body of a range-based for statement in that
// of its variable.  (A name declared there may not be declared again in that block.)
a_scope_number regionOf(int depth) {
  const a_scope_stack_entry& e = scope_stack[depth];
  if (depth > 0 && C_dialect == C_dialect_cplusplus) {
    a_scope_stack_entry& p = scope_stack[depth - 1];
    bool same = false;
    if (!p.is_dissociated_from_loop_scope) {
      if (e.kind == sck_block) {
        same = p.kind == sck_condition ||
               (e.is_loop_scope && (p.is_for_init_block || declaresRangeForVariable(&p)));
      } else if (e.kind == sck_condition) {
        same = p.is_for_init_block;
      }
    }
    if (same) {
      a_scope_number r = regionOf(depth - 1);
      regions[e.number] = r;
      return r;
    }
  }
  return e.number;
}

// The scope in which the entity of a scope stack entry (a class, a function or a namespace) is
// declared, or null
a_scope_ptr declaringScope(const a_scope_stack_entry& e) {
  switch (e.kind) {
    case sck_class_struct_union:
    case sck_class_reactivation:
      return e.assoc_type != nullptr ? e.assoc_type->source_corresp.parent_scope : nullptr;
    case sck_function:
      return e.assoc_routine != nullptr ? e.assoc_routine->source_corresp.parent_scope : nullptr;
    case sck_namespace:
    case sck_namespace_extension:
    case sck_namespace_reactivation:
      return e.assoc_namespace != nullptr ? e.assoc_namespace->source_corresp.parent_scope : nullptr;
    default:
      return nullptr;
  }
}

// The scopes of the front end's scope stack, innermost first, in which an unqualified name is
// looked up at this point.  In the body of a template (also when the front end parses it
// generically), the scope that makes the template parameters visible is a template instantiation
// scope with the number of the template parameter scope, and the scopes below the instantiation
// context are those of the point of instantiation, which are not visible: the scopes that
// enclose the template follow instead (from the IL).  The scopes that only control visibility
// (pragmas, access checking) are left out.  A mem-initializer-id (the member that
// a constructor initializes) is looked up without the constructor's scope, where its parameters
// are declared.
int currentScopes(bool memInitializerId) {
  std::vector<RR::Scope> chain;
  bool global = false;
  int top = depth_scope_stack;
  if (memInitializerId) {
    for (int depth = top; depth >= 0; --depth) {
      if (scope_stack[depth].kind == sck_function) {
        top = depth - 1;
        break;
      }
    }
  }
  for (int depth = top; depth >= 0; --depth) {
    const a_scope_stack_entry& e = scope_stack[depth];
    RR::Scope s;
    switch (e.kind) {
      case sck_file:
        s.kind = RR::Scope::Kind::Global;
        global = true;
        break;
      case sck_namespace:
      case sck_namespace_extension:
      case sck_namespace_reactivation:
        s.kind = RR::Scope::Kind::Namespace;
        s.id = (RR::EntityId)(uintptr_t)e.assoc_namespace;
        break;
      case sck_class_struct_union:
      case sck_class_reactivation:
      case sck_enum:
        // (Not an enumeration that is not scoped: its enumerators are declared in the enclosing
        // scope.  An anonymous union is not known to be one while its members are declared.)
        if (e.kind == sck_enum && e.assoc_type != nullptr && !e.assoc_type->variant.integer.is_scoped_enum) break;
        s.kind = RR::Scope::Kind::Class;
        s.id = classEntity(e.assoc_type);
        if (e.kind != sck_enum) classTypes.emplace(s.id, e.assoc_type);
        break;
      case sck_function:
        s.kind = RR::Scope::Kind::Function;
        s.id = (std::uint64_t)e.number;
        break;
      case sck_block:
      case sck_condition:
        s.kind = RR::Scope::Kind::Local;
        s.id = (std::uint64_t)regionOf(depth);
        break;
      case sck_func_prototype:
      case sck_template_declaration:
      case sck_template_instantiation:
        s.kind = RR::Scope::Kind::Local;
        s.id = (std::uint64_t)e.number;
        break;
      default:
        break;
    }
    if (e.kind == sck_instantiation_context) {
      // The scopes that enclose the outermost class, function or namespace above the context
      for (int d = depth + 1; d <= top; ++d) {
        a_scope_ptr p = declaringScope(scope_stack[d]);
        if (p == nullptr) continue;
        for (; p != nullptr; p = p->kind == sck_file ? nullptr : p->parent) {
          RR::Scope ps = scopeOf(p);
          if (ps.kind == RR::Scope::Kind::Global) global = true;
          if (ps.kind != RR::Scope::Kind::None && std::find(chain.begin(), chain.end(), ps) == chain.end()) {
            chain.push_back(ps);
          }
        }
        break;
      }
      break;
    }
    if (s.kind != RR::Scope::Kind::None && (chain.empty() || chain.back() != s)) chain.push_back(s);
  }
  if (!global) chain.push_back(RR::Scope{RR::Scope::Kind::Global, 0});
  return RR::impl::current().addScopes(chain);
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
    case iek_template: {
      // Only the scope (the other properties of templates are those of their symbols)
      e.scope = scopeOf(((a_template_ptr)il)->source_corresp.parent_scope);
      break;
    }
    default:
      break;
  }
  if (sc != nullptr) {
    if (sc->name != nullptr) e.name = sc->name;
    e.parent = parentOf(sc);
    e.scope = scopeOf(sc->parent_scope);
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
void onReference(a_symbol_ptr sym, char code, a_const_char* fileName, a_line_number line, int column,
                 a_symbol_reference_kind flags) {
  if (sym == nullptr || fileName == nullptr) return;
  if (sym->kind == sk_keyword || sym->kind == sk_undefined) return;
  an_il_entry_kind iek = iek_none;
  char* il = hasILEntry(sym) ? il_entry_for_symbol_null_okay(sym, &iek) : nullptr;
  // In an instance of a template, the name of a template parameter is recorded again with the
  // template argument (a type or a value) as its entity: not a reference to the argument (the
  // records of the template have the parameter)
  if (sym->is_template_param && il != nullptr &&
      ((iek == iek_type && ((a_type_ptr)il)->kind != tk_template_param) ||
       (iek == iek_constant && ((a_constant_ptr)il)->kind != ck_template_param))) {
    return;
  }
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
  // A member that a constructor initializes is looked up in its class (also by GCC when a parameter
  // has its name; a base class is not)
  bool memInitializerId = (flags & SRK_INITIALIZATION) != 0 &&
                          (flags & (SRK_DECLARATION | SRK_DEFINITION)) == 0 && sym->kind == sk_field;
  records.push_back(Record{id, code, fileName, (unsigned long)line, column, currentScopes(memInitializerId)});
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
  regions.clear();
  classTypes.clear();
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
  xr.setCplusplus(C_dialect == C_dialect_cplusplus);
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
    ref.scopes = r.scopes;
    e->references.push_back(ref);
    // A label is declared in the function that defines it; an entity whose IL entity does not
    // tell its scope (the member of an anonymous union, a parameter in the identifier list of a
    // function in K&R C), in the innermost scope of the scope stack where it is first declared
    // (an anonymous union is not a scope)
    if (e->scope.kind == RR::Scope::Kind::None && (r.code == 'd' || r.code == 'D') &&
        e->kind != RR::Kind::Macro) {
      for (const RR::Scope& s : xr.scopes(r.scopes)) {
        if (s.kind == RR::Scope::Kind::Class) {
          auto c = classTypes.find(s.id);
          if (c != classTypes.end() && isAnonymousClass(c->second)) continue;
        }
        if (e->kind != RR::Kind::Label || s.kind == RR::Scope::Kind::Function) {
          e->scope = s;
          break;
        }
      }
    }
  }
  pending.clear();
  records.clear();
  symbolEntities.clear();
  regions.clear();
  classTypes.clear();
  addOverrides(xr);
  xr.finish();
  // ROSE_REFACTOR_DUMP: lists the entities declared outside system headers (for debugging)
  if (std::getenv("ROSE_REFACTOR_DUMP") != nullptr) {
    for (const auto& kv : xr.entities()) {
      const RR::Entity& e = kv.second;
      if (e.declaredInSystemHeader()) continue;
      std::fprintf(stderr, "%llx %s [%s] %s type='%s' parent=%llx scope=%d:%llx access=%s%s%s%s%s%s\n",
                   (unsigned long long)e.id, e.qualifiedName.c_str(), e.name.c_str(), RR::kindName(e.kind),
                   e.type.c_str(), (unsigned long long)e.parent, (int)e.scope.kind,
                   (unsigned long long)e.scope.id, RR::accessName(e.access),
                   e.isStatic ? " static" : "", e.isVirtual ? " virtual" : "", e.isInTemplate ? " in-template" : "",
                   e.isTemplateInstance ? " instance" : "", e.isImplicit ? " implicit" : "");
      for (const RR::BaseClass& b : e.bases) {
        std::fprintf(stderr, "    base %llx %s %s..%s\n", (unsigned long long)b.entity, RR::accessName(b.access),
                     b.start.str().c_str(), b.end.str().c_str());
      }
      for (RR::EntityId o : e.overrides) std::fprintf(stderr, "    overrides %llx\n", (unsigned long long)o);
      for (const RR::Reference& r : e.references) {
        std::string chain;
        for (const RR::Scope& s : xr.scopes(r.scopes)) {
          char buf[64];
          std::snprintf(buf, sizeof buf, " %d:%llx", (int)s.kind, (unsigned long long)s.id);
          chain += buf;
        }
        std::fprintf(stderr, "    %c %s%s  in%s\n", r.code, r.pos.str().c_str(), r.inSystemHeader ? " (system)" : "",
                     chain.c_str());
      }
    }
  }
}

}  // namespace edg2sage
