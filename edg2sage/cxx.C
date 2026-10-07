// C++ declarations: namespaces and using declarations/directives.
#include "edg2sage.h"

using namespace edg;
using namespace Sawyer::Message;

namespace edg2sage {

// The first declaration of a namespace (created on demand for a namespace
// that is referenced before, or without, being translated).
SgNamespaceDeclarationStatement* Translator::namespaceDeclarationFor(a_namespace_ptr ns) {
  while (ns != nullptr && ns->is_namespace_alias) ns = ns->variant.assoc_namespace;
  if (ns == nullptr) throw Unsupported("unknown namespace");
  auto it = firstNamespaceDecl.find(ns);
  if (it != firstNamespaceDecl.end()) return it->second;
  SgScopeStatement* scope = parentScopeOf(&ns->source_corresp, globalScope);
  SgName name = nameOf(&ns->source_corresp);
  SgNamespaceDeclarationStatement* decl = SageBuilder::buildNamespaceDeclaration_nfi(name, name.is_null(), scope);
  SgNamespaceDeclarationStatement* first = isSgNamespaceDeclarationStatement(decl->get_firstNondefiningDeclaration());
  if (first == nullptr) first = decl;
  setPosition(decl, ns->source_corresp.decl_position);
  if (first != decl) setPosition(first, ns->source_corresp.decl_position);
  firstNamespaceDecl[ns] = first;
  if (ns->variant.assoc_scope != nullptr && scopes.count(ns->variant.assoc_scope) == 0) {
    scopes[ns->variant.assoc_scope] = decl->get_definition();
  }
  return first;
}

void Translator::translateNamespace(SeqCursor& cursor, a_namespace_ptr ns, a_src_seq_secondary_decl_ptr sec,
                                    SgScopeStatement* scope) {
  cursor.advance();
  a_source_position start = sec ? sec->decl_position : ns->source_corresp.decl_position;
  a_decl_position_supplement_ptr dpi = sec ? sec->decl_pos_info : ns->source_corresp.decl_pos_info;
  if (dpi != nullptr && dpi->specifiers_range.start.seq != 0) start = dpi->specifiers_range.start;
  SgName name = nameOf(&ns->source_corresp);

  if (ns->is_namespace_alias) {
    // namespace N = M;
    SgNamespaceDeclarationStatement* target = namespaceDeclarationFor(ns->variant.assoc_namespace);
    SgNamespaceAliasDeclarationStatement* alias = new SgNamespaceAliasDeclarationStatement(name, target);
    alias->set_firstNondefiningDeclaration(alias);
    alias->set_definingDeclaration(alias);
    alias->set_parent(scope);
    setPosition(alias, start, ns->source_corresp.decl_position);
    SgNamespaceSymbol* sym = new SgNamespaceSymbol(name, target);
    sym->set_aliasDeclaration(alias);
    sym->set_isAlias(true);
    scope->insert_symbol(name, sym);
    appendStatementTo(scope, alias);
    return;
  }

  // A namespace definition, or an extension of a namespace ("reopening").
  SgNamespaceDeclarationStatement* decl = SageBuilder::buildNamespaceDeclaration_nfi(name, name.is_null(), scope);
  if (ns->is_inline) decl->set_isInlinedNamespace(true);
  SgNamespaceDeclarationStatement* first = isSgNamespaceDeclarationStatement(decl->get_firstNondefiningDeclaration());
  if (first == nullptr) first = decl;
  if (firstNamespaceDecl.count(ns) == 0) firstNamespaceDecl[ns] = first;
  SgNamespaceDefinitionStatement* def = decl->get_definition();
  if (ns->variant.assoc_scope != nullptr) scopes[ns->variant.assoc_scope] = def;
  appendStatementTo(scope, decl);

  scopeStack.push_back(def);
  SageBuilder::pushScopeStack(def);
  a_source_position end = start;
  while (!cursor.atEnd()) {
    if (cursor.kind() == iek_src_seq_end_of_construct) {
      a_src_seq_end_of_construct_ptr eoc = (a_src_seq_end_of_construct_ptr)cursor.ptr();
      cursor.advance();
      if ((void*)eoc->entity.ptr == (void*)ns) {
        end = eoc->position;
        break;
      }
      continue;
    }
    translateDeclarationEntry(cursor, def);
  }
  if (pendingBaseTypeDecl != nullptr) {
    SgDeclarationStatement* base = pendingBaseTypeDecl;
    pendingBaseTypeDecl = nullptr;
    appendStatementTo(def, base);
  }
  SageBuilder::popScopeStack();
  scopeStack.pop_back();
  setPosition(decl, start, end);
  setPosition(def, start, end);
  if (first != decl && first->get_startOfConstruct() == nullptr) setPosition(first, start);
}

SgDeclarationStatement* Translator::translateUsingDeclaration(a_using_decl_ptr ud, SgScopeStatement* scope) {
  if (ud->compiler_generated) return nullptr;
  SgDeclarationStatement* result = nullptr;
  if (ud->is_using_directive) {
    // using namespace N;
    if (ud->entity.kind != iek_namespace) throw Unsupported("using directive");
    SgNamespaceDeclarationStatement* nd = namespaceDeclarationFor((a_namespace_ptr)ud->entity.ptr);
    result = new SgUsingDirectiveStatement(nd);
  } else {
    // using N::name;
    SgDeclarationStatement* decl = nullptr;
    SgInitializedName* iname = nullptr;
    char* ptr = ud->entity.ptr;
    switch ((an_il_entry_kind)ud->entity.kind) {
      case iek_routine:
        decl = functionDeclarationFor((a_routine_ptr)ptr);
        break;
      case iek_variable:
        iname = variableFor((a_variable_ptr)ptr);
        break;
      case iek_field:
        iname = fieldFor((a_field_ptr)ptr);
        break;
      case iek_type: {
        a_type_ptr t = (a_type_ptr)ptr;
        if (t->kind == tk_class || t->kind == tk_struct || t->kind == tk_union) {
          decl = classDeclarationFor(t);
        } else if (t->kind == tk_integer && t->variant.integer.enum_type) {
          decl = enumDeclarationFor(t);
        } else if (t->kind == tk_typeref && typeref_is_typedef(t)) {
          decl = typedefDeclarationFor(t);
        }
        break;
      }
      case iek_namespace:
        decl = namespaceDeclarationFor((a_namespace_ptr)ptr);
        break;
      case iek_constant: {
        auto en = enumerators.find((a_constant_ptr)ptr);
        if (en != enumerators.end()) iname = en->second;
        break;
      }
      default:
        break;
    }
    if (decl == nullptr && iname == nullptr) throw Unsupported("using declaration of this kind of entity");
    result = new SgUsingDeclarationStatement(decl, iname);
  }
  result->set_firstNondefiningDeclaration(result);
  result->set_parent(scope);
  setPosition(result, ud->position);
  if (ud->is_class_member) setAccess(result, ud->access);
  return result;
}

// Direct base classes, in declaration order
void Translator::translateBaseClasses(a_class_type_supplement_ptr ctsp, SgClassDefinition* cdef) {
  for (a_base_class_ptr bc = ctsp->direct_base_classes; bc != nullptr; bc = bc->next_direct) {
    if (!bc->direct) continue;
    a_type_ptr bt = skip_typerefs(bc->type);
    SgClassDeclaration* bdecl = classDeclarationFor(bt);
    SgBaseClass* b = new SgBaseClass(bdecl, true);
    b->set_parent(cdef);
    SgBaseClassModifier* mod = b->get_baseClassModifier();
    if (mod == nullptr) {
      mod = new SgBaseClassModifier();
      b->set_baseClassModifier(mod);
      mod->set_parent(b);
    }
    if (bc->is_virtual) mod->setVirtual();
    an_access_specifier access = bc->derivation != nullptr ? bc->derivation->access : as_public;
    switch (access) {
      case as_public:
        mod->get_accessModifier().setPublic();
        break;
      case as_protected:
        mod->get_accessModifier().setProtected();
        break;
      case as_private:
        mod->get_accessModifier().setPrivate();
        break;
      default:
        break;
    }
    cdef->append_inheritance(b);
  }
}

// The mem-initializer list of a constructor definition: "C() : B(1), m(2) {}"
void Translator::translateConstructorInitializers(a_scope_ptr fscope, SgMemberFunctionDeclaration* decl) {
  if (decl == nullptr || fscope == nullptr) return;
  SgCtorInitializerList* list = decl->get_CtorInitializerList();
  if (list == nullptr) {
    list = new SgCtorInitializerList();
    list->set_firstNondefiningDeclaration(list);
    decl->set_CtorInitializerList(list);
    list->set_parent(decl);
    setCompilerGenerated(list);
  }
  for (a_constructor_init_ptr ci = fscope->variant.routine.constructor_inits; ci != nullptr; ci = ci->next) {
    if (ci->compiler_generated) continue;  // implicit initialization of a base or member
    SgName name;
    SgType* type = nullptr;
    switch (ci->kind) {
      case cik_field:
        name = nameOf(&ci->variant.field->source_corresp);
        type = convertType(ci->variant.field->type);
        break;
      case cik_direct_base_class:
      case cik_virtual_base_class: {
        a_type_ptr bt = skip_typerefs(ci->variant.base_class->type);
        name = classDeclarationFor(bt)->get_name();
        type = convertType(bt);
        break;
      }
      case cik_delegation:
        name = isSgClassDefinition(decl->get_scope()) ? isSgClassDefinition(decl->get_scope())->get_declaration()->get_name()
                                                     : decl->get_name();
        type = isSgClassDefinition(decl->get_scope()) ? isSgClassDefinition(decl->get_scope())->get_declaration()->get_type()
                                                     : nullptr;
        break;
      default:
        continue;
    }
    SgInitializer* init = convertDynamicInit(ci->initializer, type);
    if (init == nullptr) {
      // "m()": value initialization
      SgExprListExp* args = SageBuilder::buildExprListExp_nfi();
      setCompilerGenerated(args);
      init = SageBuilder::buildConstructorInitializer_nfi(nullptr, args, type, false, false, true, true);
      args->set_parent(init);
    }
    if (SgConstructorInitializer* c = isSgConstructorInitializer(init)) {
      c->set_need_name(false);
      c->set_need_parenthesis_after_name(true);
    }
    SgInitializedName* in = SageBuilder::buildInitializedName_nfi(name, type, init);
    init->set_parent(in);
    in->set_scope(isSgClassDefinition(decl->get_scope()) ? decl->get_scope() : currentScope());
#if EXTRA_SOURCE_POSITIONS_IN_IL
    if (ci->ctor_init_range.start.seq != 0) {
      setPosition(in, ci->ctor_init_range.start, ci->ctor_init_range.end);
      if (init->get_startOfConstruct() == nullptr || init->get_startOfConstruct()->isCompilerGenerated()) {
        setPosition(init, ci->ctor_init_range.start, ci->ctor_init_range.end);
      }
    } else
#endif
    {
      setCompilerGenerated(in, true);
    }
    list->append_ctor_initializer(in);
    in->set_parent(list);
  }
}

}  // namespace edg2sage
