// Translation of declarations: the walk over EDG's source sequence lists, and
// variables, functions, classes, enums, typedefs, pragmas, ...
#include "edg2sage.h"

#include <climits>

using namespace edg;
using namespace Sawyer::Message;

namespace edg2sage {

void SeqCursor::normalize() {
  for (;;) {
    if (cur == nullptr) {
      if (sublistParent == nullptr) break;
      cur = sublistParent->next;
      sublistParent = nullptr;
      continue;
    }
    if (ss_entry_kind(cur) == iek_src_seq_sublist) {
      sublistParent = cur;
      cur = assoc_sublist_of(cur)->source_sequence_list;
      continue;
    }
    break;
  }
}

// ---------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------

void Translator::appendStatementTo(SgScopeStatement* scope, SgStatement* stmt) {
  if (stmt == nullptr) return;
  if (SgGlobal* g = isSgGlobal(scope)) {
    SgDeclarationStatement* d = isSgDeclarationStatement(stmt);
    ROSE_ASSERT(d != nullptr);
    g->append_declaration(d);
  } else if (SgClassDefinition* c = isSgClassDefinition(scope)) {
    SgDeclarationStatement* d = isSgDeclarationStatement(stmt);
    ROSE_ASSERT(d != nullptr);
    c->append_member(d);
  } else if (SgNamespaceDefinitionStatement* n = isSgNamespaceDefinitionStatement(scope)) {
    SgDeclarationStatement* d = isSgDeclarationStatement(stmt);
    ROSE_ASSERT(d != nullptr);
    n->append_declaration(d);
  } else if (SgBasicBlock* b = isSgBasicBlock(scope)) {
    b->append_statement(stmt);
  } else if (SgForStatement* f = isSgForStatement(scope)) {
    // Declarations in a for-init-statement
    f->get_for_init_stmt()->append_init_stmt(stmt);
    stmt->set_parent(f->get_for_init_stmt());
    return;
  } else {
    scope->append_statement(stmt);
  }
  stmt->set_parent(scope);
}

void Translator::setAccess(SgDeclarationStatement* decl, an_access_specifier access) {
  SgAccessModifier& am = decl->get_declarationModifier().get_accessModifier();
  switch (access) {
    case as_public:
      am.setPublic();
      break;
    case as_protected:
      am.setProtected();
      break;
    case as_private:
      am.setPrivate();
      break;
    default:
      am.setUndefined();
      break;
  }
}

void Translator::setDeclarationModifiers(SgDeclarationStatement* decl, a_source_correspondence* scp,
                                         a_storage_class sc) {
  SgStorageModifier& sm = decl->get_declarationModifier().get_storageModifier();
  switch (sc) {
    case sc_extern:
      sm.setExtern();
      break;
    case sc_static:
      sm.setStatic();
      break;
    case sc_register:
      sm.setRegister();
      break;
    case sc_auto:
      if (!isCxx) sm.setAuto();
      break;
    default:
      break;
  }
  if (scp != nullptr && scp->is_class_member) {
    setAccess(decl, (an_access_specifier)scp->access);
  } else if (isCxx) {
    decl->get_declarationModifier().get_accessModifier().setUndefined();
  }
}

void Translator::skipToEndOfConstruct(SeqCursor& cursor, void* entity) {
  while (!cursor.atEnd()) {
    if (cursor.kind() == iek_src_seq_end_of_construct) {
      a_src_seq_end_of_construct_ptr eoc = (a_src_seq_end_of_construct_ptr)cursor.ptr();
      if ((void*)eoc->entity.ptr == entity) {
        cursor.advance();
        return;
      }
    }
    cursor.advance();
  }
}

// A tag type defined as part of another declaration ("struct S {...} x;",
// "typedef struct {...} T;") becomes the base type defining declaration of the
// declaration that follows it.
void Translator::attachPendingBaseTypeDeclaration(SgDeclarationStatement* decl) {
  if (pendingBaseTypeDecl == nullptr) return;
  SgDeclarationStatement* base = pendingBaseTypeDecl;
  pendingBaseTypeDecl = nullptr;
  pendingBaseType = nullptr;
  if (SgVariableDeclaration* vd = isSgVariableDeclaration(decl)) {
    vd->set_baseTypeDefiningDeclaration(base);
    vd->set_variableDeclarationContainsBaseTypeDefiningDeclaration(true);
    base->set_parent(vd);
  } else if (SgTypedefDeclaration* td = isSgTypedefDeclaration(decl)) {
    td->set_declaration(base);
    td->set_typedefBaseTypeContainsDefiningDeclaration(true);
    base->set_parent(td);
  } else {
    // Cannot be attached (e.g. a function returning a struct defined in its
    // declaration): emit the definition as a separate declaration.
    appendStatementTo(isSgScopeStatement(decl->get_parent()) ? isSgScopeStatement(decl->get_parent()) : currentScope(),
                      base);
  }
}

// ---------------------------------------------------------------------------------
// The source sequence walk
// ---------------------------------------------------------------------------------

void Translator::translateDeclarationList(SeqCursor& cursor, SgScopeStatement* scope, void* endEntity) {
  while (!cursor.atEnd()) {
    if (cursor.kind() == iek_src_seq_end_of_construct) {
      a_src_seq_end_of_construct_ptr eoc = (a_src_seq_end_of_construct_ptr)cursor.ptr();
      cursor.advance();
      if (endEntity != nullptr && (void*)eoc->entity.ptr == endEntity) return;
      continue;
    }
    translateDeclarationEntry(cursor, scope);
  }
  if (pendingBaseTypeDecl != nullptr) {
    SgDeclarationStatement* base = pendingBaseTypeDecl;
    pendingBaseTypeDecl = nullptr;
    appendStatementTo(scope, base);
  }
}

void Translator::translateDeclarationEntry(SeqCursor& cursor, SgScopeStatement* scope) {
  a_source_sequence_entry_ptr entry = cursor.cur;
  an_il_entry_kind kind = cursor.kind();
  char* ptr = cursor.ptr();
  a_src_seq_secondary_decl_ptr sec = nullptr;
  if (kind == iek_src_seq_secondary_decl) {
    sec = (a_src_seq_secondary_decl_ptr)ptr;
    kind = (an_il_entry_kind)sec->entity.kind;
    ptr = sec->entity.ptr;
  }
  try {
    switch (kind) {
      case iek_type:
        translateTypeDeclaration(cursor, (a_type_ptr)ptr, sec, scope);
        return;  // advances the cursor itself
      case iek_variable: {
        a_variable_ptr var = (a_variable_ptr)ptr;
        SgDeclarationStatement* d = translateVariable(var, sec, scope);
        cursor.advance();
        bool embedded = sec ? sec->embedded_source_sequence_entries : var->embedded_source_sequence_entries;
        if (embedded) translateDeclarationList(cursor, scope, var);
        if (d) appendStatementTo(scope, d);
        return;
      }
      case iek_routine: {
        a_routine_ptr routine = (a_routine_ptr)ptr;
        cursor.advance();
        bool embedded = sec ? sec->embedded_source_sequence_entries : routine->embedded_source_sequence_entries;
        if (embedded) translateDeclarationList(cursor, scope, routine);
        SgFunctionDeclaration* d = translateRoutine(routine, sec, scope);
        if (d != nullptr) appendStatementTo(scope, d);
        return;
      }
      case iek_field: {
        SgClassDefinition* cdef = isSgClassDefinition(scope);
        SgDeclarationStatement* d = cdef ? translateField((a_field_ptr)ptr, cdef) : nullptr;
        cursor.advance();
        if (d) appendStatementTo(scope, d);
        return;
      }
      case iek_pragma: {
        SgPragmaDeclaration* d = translatePragma((a_pragma_ptr)ptr);
        cursor.advance();
        if (d) appendStatementTo(scope, d);
        return;
      }
      case iek_static_assertion: {
        SgDeclarationStatement* d = translateStaticAssertion((a_static_assertion_ptr)ptr);
        cursor.advance();
        if (d) appendStatementTo(scope, d);
        return;
      }
      case iek_statement: {
        // The block of a GNU statement expression appearing in a declaration:
        // translated together with the expression.
        a_statement_ptr st = (a_statement_ptr)ptr;
        cursor.advance();
        skipToEndOfConstruct(cursor, st);
        return;
      }
      default:
        // macros, constants of "manifest constant" macros, ... are not part of the AST
        cursor.advance();
        return;
    }
  } catch (const Unsupported& u) {
    warnings++;
    mlog[WARN] << "skipping declaration (" << u.what << ")\n";
    if (cursor.cur == entry) cursor.advance();
  }
}

// ---------------------------------------------------------------------------------
// Variables and fields
// ---------------------------------------------------------------------------------

SgDeclarationStatement* Translator::translateVariable(a_variable_ptr var, a_src_seq_secondary_decl_ptr sec,
                                                      SgScopeStatement* scope) {
  a_type_ptr declaredType = sec != nullptr && sec->declared_type != nullptr ? sec->declared_type
                            : var->declared_type != nullptr ? var->declared_type
                                                            : var->type;
  SgType* type = convertType(declaredType);
  SgName name = nameOf(&var->source_corresp);
  bool unnamed = name.is_null();
  if (unnamed && var->is_anonymous_parent_object) name = SgName("");

  SgInitializer* init = nullptr;
  if (sec == nullptr) init = convertVariableInitializer(var);

  SgInitializedName* iname = SageBuilder::buildInitializedName_nfi(name, type, init);
  SgVariableDeclaration* decl = new SgVariableDeclaration(iname);
  decl->set_firstNondefiningDeclaration(decl);
  if (sec == nullptr && var->storage_class != sc_extern) decl->set_definingDeclaration(decl);
  iname->set_scope(scope);
  decl->set_parent(scope);
  if (init) init->set_parent(iname);

  a_storage_class sc = sec ? sec->declared_storage_class : var->declared_storage_class;
  setDeclarationModifiers(decl, &var->source_corresp, sc);
  if (var->is_thread_local) decl->get_declarationModifier().get_storageModifier().set_thread_local_storage(true);
  if (var->is_constexpr) decl->set_is_constexpr(true);

  // Positions: the declaration spans the specifiers through the declarator/initializer.
  a_source_position start = sec ? sec->decl_position : var->source_corresp.decl_position;
  a_source_position end = start;
  a_decl_position_supplement_ptr dpi = sec ? sec->decl_pos_info : var->source_corresp.decl_pos_info;
  if (dpi != nullptr) {
    if (dpi->specifiers_range.start.seq != 0) start = dpi->specifiers_range.start;
    if (dpi->variant.declarator_range.end.seq != 0) end = dpi->variant.declarator_range.end;
  }
  if (sec == nullptr && var->initializer_range.end.seq != 0) end = var->initializer_range.end;
  setPosition(decl, start, end);
  setPosition(iname, sec ? sec->decl_position : var->source_corresp.decl_position);
  if (SgDeclarationStatement* vdef = iname->get_declptr()) {
    if (vdef != decl) setPosition(vdef, start, end);
  }

  // Symbol: all declarations of the variable share the symbol of the first one.
  auto prev = variables.find(var);
  if (prev == variables.end()) {
    variables[var] = iname;
    if (!name.is_null()) {
      SgVariableSymbol* sym = new SgVariableSymbol(iname);
      scope->insert_symbol(name, sym);
    }
  } else {
    iname->set_prev_decl_item(prev->second);
    if (sec == nullptr) {
      // The defining declaration: later references use it for its initializer.
      prev->second->set_definition(decl);
    }
  }
  attachPendingBaseTypeDeclaration(decl);
  return decl;
}

SgDeclarationStatement* Translator::translateField(a_field_ptr field, SgClassDefinition* cdef) {
  SgType* type = convertType(field->type);
  SgName name = nameOf(&field->source_corresp);
  SgInitializer* init = nullptr;
  if (field->initializer != nullptr) init = convertDynamicInit(field->initializer, type);
  SgInitializedName* iname = SageBuilder::buildInitializedName_nfi(name, type, init);
  SgVariableDeclaration* decl = new SgVariableDeclaration(iname);
  decl->set_firstNondefiningDeclaration(decl);
  decl->set_definingDeclaration(decl);
  iname->set_scope(cdef);
  decl->set_parent(cdef);
  if (init) init->set_parent(iname);
  if (field->is_bit_field) {
    SgExpression* width = nullptr;
    if (field->bit_size_constant != nullptr) {
      width = convertConstant(field->bit_size_constant);
    } else {
      width = SageBuilder::buildUnsignedLongVal_nfi(field->bit_size, "");
      setCompilerGenerated(width);
    }
    decl->set_bitfield(width);
    width->set_parent(decl);
  }
  if (field->is_mutable) decl->get_declarationModifier().get_storageModifier().setMutable();
  setAccess(decl, (an_access_specifier)field->source_corresp.access);

  a_source_position start = field->source_corresp.decl_position, end = start;
  if (a_decl_position_supplement_ptr dpi = field->source_corresp.decl_pos_info) {
    if (dpi->specifiers_range.start.seq != 0) start = dpi->specifiers_range.start;
    if (dpi->variant.declarator_range.end.seq != 0) end = dpi->variant.declarator_range.end;
  }
  setPosition(decl, start, end);
  setPosition(iname, field->source_corresp.decl_position);
  if (SgDeclarationStatement* vdef = iname->get_declptr()) {
    if (vdef != decl) setPosition(vdef, start, end);
  }
  fields[field] = iname;
  if (!name.is_null()) cdef->insert_symbol(name, new SgVariableSymbol(iname));
  attachPendingBaseTypeDeclaration(decl);
  return decl;
}

SgInitializedName* Translator::variableFor(a_variable_ptr var) {
  auto it = variables.find(var);
  if (it != variables.end()) return it->second;
  // Not declared on any list we translated (e.g. a compiler-generated variable):
  // create a declaration that is not part of a statement list.
  SgScopeStatement* scope = parentScopeOf(&var->source_corresp);
  SgName name = nameOf(&var->source_corresp);
  if (name.is_null()) name = SgName("__edg_unnamed_variable");
  SgInitializedName* iname = SageBuilder::buildInitializedName_nfi(name, convertType(var->type), nullptr);
  SgVariableDeclaration* decl = new SgVariableDeclaration(iname);
  decl->set_firstNondefiningDeclaration(decl);
  iname->set_scope(scope);
  decl->set_parent(scope);
  setPosition(decl, var->source_corresp.decl_position);
  setPosition(iname, var->source_corresp.decl_position);
  variables[var] = iname;
  scope->insert_symbol(name, new SgVariableSymbol(iname));
  return iname;
}

SgVariableSymbol* Translator::variableSymbolFor(a_variable_ptr var) {
  SgInitializedName* iname = variableFor(var);
  SgVariableSymbol* sym = isSgVariableSymbol(iname->search_for_symbol_from_symbol_table());
  if (sym == nullptr) {
    sym = new SgVariableSymbol(iname);
    SgScopeStatement* scope = iname->get_scope() ? iname->get_scope() : currentScope();
    scope->insert_symbol(iname->get_name(), sym);
  }
  return sym;
}

SgInitializedName* Translator::fieldFor(a_field_ptr field) {
  auto it = fields.find(field);
  if (it != fields.end()) return it->second;
  throw Unsupported("reference to a field of a class that was not translated");
}

SgVariableSymbol* Translator::fieldSymbolFor(a_field_ptr field) {
  SgInitializedName* iname = fieldFor(field);
  SgVariableSymbol* sym = isSgVariableSymbol(iname->search_for_symbol_from_symbol_table());
  if (sym == nullptr) {
    sym = new SgVariableSymbol(iname);
    iname->get_scope()->insert_symbol(iname->get_name(), sym);
  }
  return sym;
}

// ---------------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------------

SgFunctionParameterList* Translator::buildParameterList(a_routine_ptr routine, bool defining) {
  SgFunctionParameterList* params = SageBuilder::buildFunctionParameterList_nfi();
  a_type_ptr rtype = skip_typerefs(routine->type);
  a_routine_type_supplement_ptr rtsp = rtype->variant.routine.extra_info;
  if (defining) {
    a_scope_ptr fscope = functionScopeOf(routine);
    a_variable_ptr pv = fscope ? fscope->variant.routine.parameters : nullptr;
    a_param_type_ptr pt = rtsp->param_type_list;
    for (; pv != nullptr; pv = pv->next) {
      SgType* t = convertType(pv->declared_type ? pv->declared_type : pv->type);
      SgInitializer* defaultArg = nullptr;
      if (pt != nullptr && pt->default_arg_expr != nullptr) {
        try {
          SgExpression* e = convertExpression(pt->default_arg_expr);
          defaultArg = SageBuilder::buildAssignInitializer_nfi(e, t);
          setPosition(defaultArg, pt->default_arg_expr->position);
        } catch (const Unsupported&) {
        }
      }
      SgInitializedName* in = SageBuilder::buildInitializedName_nfi(nameOf(&pv->source_corresp), t, defaultArg);
      if (defaultArg) defaultArg->set_parent(in);
      setPosition(in, pv->source_corresp.decl_position);
      params->append_arg(in);
      in->set_parent(params);
      variables[pv] = in;
      if (pt != nullptr) pt = pt->next;
    }
  } else {
    for (a_param_type_ptr pt = rtsp->param_type_list; pt != nullptr; pt = pt->next) {
      SgType* t = convertType(pt->declared_type ? pt->declared_type : pt->type);
      SgInitializer* defaultArg = nullptr;
      if (pt->default_arg_expr != nullptr) {
        try {
          SgExpression* e = convertExpression(pt->default_arg_expr);
          defaultArg = SageBuilder::buildAssignInitializer_nfi(e, t);
          setPosition(defaultArg, pt->default_arg_expr->position);
        } catch (const Unsupported&) {
        }
      }
      SgName pname = pt->name ? SgName(pt->name) : SgName("");
      SgInitializedName* in = SageBuilder::buildInitializedName_nfi(pname, t, defaultArg);
      if (defaultArg) defaultArg->set_parent(in);
      if (pt->decl_pos_info != nullptr && pt->decl_pos_info->identifier_range.start.seq != 0) {
        setPosition(in, pt->decl_pos_info->identifier_range.start);
      } else {
        setPosition(in, routine->source_corresp.decl_position);
      }
      params->append_arg(in);
      in->set_parent(params);
    }
  }
  if (rtsp->has_ellipsis) {
    SgInitializedName* in = SageBuilder::buildInitializedName_nfi(SgName(""), SgTypeEllipse::createType(), nullptr);
    setCompilerGenerated(in);
    params->append_arg(in);
    in->set_parent(params);
  }
  return params;
}

// The SgFunctionDeclaration constructor creates an (empty) parameter list.
static void replaceParameterList(SgFunctionDeclaration* decl, SgFunctionParameterList* params) {
  SgFunctionParameterList* old = decl->get_parameterList();
  decl->set_parameterList(params);
  params->set_parent(decl);
  if (old != nullptr && old != params) delete old;
}

SgFunctionDeclaration* Translator::functionDeclarationFor(a_routine_ptr routine) {
  auto it = firstRoutineDecl.find(routine);
  if (it != firstRoutineDecl.end()) return it->second;
  // Referenced before (or without) any declaration we translated, e.g. an
  // implicitly declared C function or a builtin: create a nondefining
  // declaration that is not part of any statement list.
  SgScopeStatement* scope = parentScopeOf(&routine->source_corresp, globalScope);
  SgName name = nameOf(&routine->source_corresp);
  SgFunctionType* ftype = convertFunctionType(routine->type);
  SgFunctionParameterList* params = buildParameterList(routine, false);
  SgFunctionDeclaration* decl = nullptr;
  if (isSgClassDefinition(scope)) {
    decl = new SgMemberFunctionDeclaration(name, ftype, nullptr);
  } else {
    decl = new SgFunctionDeclaration(name, ftype, nullptr);
  }
  replaceParameterList(decl, params);
  decl->set_firstNondefiningDeclaration(decl);
  decl->setForward();
  decl->set_scope(scope);
  decl->set_parent(scope);
  for (SgInitializedName* in : params->get_args()) in->set_scope(scope);
  setPosition(decl, routine->source_corresp.decl_position);
  setPosition(params, routine->source_corresp.decl_position);
  setDeclarationModifiers(decl, &routine->source_corresp, routine->storage_class);
  if (routine->source_corresp.decl_position.seq == 0) {
    setCompilerGenerated(decl);
    setCompilerGenerated(params);
  }
  firstRoutineDecl[routine] = decl;
  SgFunctionSymbol* sym = isSgMemberFunctionDeclaration(decl)
                              ? new SgMemberFunctionSymbol(isSgMemberFunctionDeclaration(decl))
                              : new SgFunctionSymbol(decl);
  scope->insert_symbol(name, sym);
  return decl;
}

SgFunctionSymbol* Translator::functionSymbolFor(a_routine_ptr routine) {
  SgFunctionDeclaration* first = functionDeclarationFor(routine);
  SgFunctionSymbol* sym = isSgFunctionSymbol(first->search_for_symbol_from_symbol_table());
  if (sym == nullptr) {
    sym = isSgMemberFunctionDeclaration(first) ? new SgMemberFunctionSymbol(isSgMemberFunctionDeclaration(first))
                                                : new SgFunctionSymbol(first);
    SgScopeStatement* scope = first->get_scope() ? first->get_scope() : globalScope;
    scope->insert_symbol(first->get_name(), sym);
  }
  return sym;
}

SgFunctionDeclaration* Translator::translateRoutine(a_routine_ptr routine, a_src_seq_secondary_decl_ptr sec,
                                                   SgScopeStatement* scope) {
  a_scope_ptr fscope = functionScopeOf(routine);
  bool isDefinition = (sec == nullptr) && fscope != nullptr && !routine->is_defaulted && !routine->is_deleted;
  SgScopeStatement* semanticScope = parentScopeOf(&routine->source_corresp, scope);
  bool isMember = isSgClassDefinition(semanticScope) != nullptr;
  SgName name = nameOf(&routine->source_corresp);

  a_type_ptr declaredType = sec != nullptr && sec->declared_type != nullptr ? sec->declared_type
                            : routine->declared_type != nullptr ? routine->declared_type
                                                                : routine->type;
  SgFunctionType* ftype = convertFunctionType(skip_typerefs(declaredType)->kind == tk_routine ? declaredType
                                                                                              : routine->type,
                                              isMember ? isSgClassDefinition(semanticScope) : nullptr);
  SgFunctionParameterList* params = buildParameterList(routine, isDefinition);

  SgFunctionDeclaration* decl = isMember ? new SgMemberFunctionDeclaration(name, ftype, nullptr)
                                         : new SgFunctionDeclaration(name, ftype, nullptr);
  replaceParameterList(decl, params);
  decl->set_scope(semanticScope);
  decl->set_parent(scope);

  // Link to the first nondefining declaration (created on demand if this is
  // the first declaration and also a definition).
  SgFunctionDeclaration* first = nullptr;
  auto it = firstRoutineDecl.find(routine);
  if (it != firstRoutineDecl.end()) {
    first = it->second;
  } else if (isDefinition) {
    first = functionDeclarationFor(routine);  // hidden nondefining declaration
    setPosition(first, routine->source_corresp.decl_position);
  } else {
    first = decl;
    firstRoutineDecl[routine] = decl;
    SgFunctionSymbol* sym = isMember ? new SgMemberFunctionSymbol(isSgMemberFunctionDeclaration(decl))
                                     : new SgFunctionSymbol(decl);
    semanticScope->insert_symbol(name, sym);
  }
  decl->set_firstNondefiningDeclaration(first);

  // Modifiers
  a_storage_class sc = sec ? sec->declared_storage_class : routine->declared_storage_class;
  setDeclarationModifiers(decl, &routine->source_corresp, sc);
  SgFunctionModifier& fm = decl->get_functionModifier();
  if (routine->is_inline) fm.setInline();
  if (routine->is_virtual) fm.setVirtual();
  if (routine->pure_virtual) fm.setPureVirtual();
  if (routine->is_explicit_constructor) fm.setExplicit();
  if (routine->is_defaulted) fm.setMarkedDefault();
  if (routine->is_deleted) fm.setMarkedDelete();
  if (routine->is_declared_constexpr) decl->set_is_constexpr(true);
  a_routine_type_supplement_ptr rtsp = skip_typerefs(routine->type)->variant.routine.extra_info;
  if (!rtsp->prototyped && isDefinition && !isCxx && rtsp->param_type_list != nullptr) {
    decl->set_oldStyleDefinition(true);
  }

  // Positions
  a_source_position start = sec ? sec->decl_position : routine->source_corresp.decl_position;
  a_source_position end = start;
  a_decl_position_supplement_ptr dpi = sec ? sec->decl_pos_info : routine->source_corresp.decl_pos_info;
  if (dpi != nullptr) {
    if (dpi->specifiers_range.start.seq != 0) start = dpi->specifiers_range.start;
    if (dpi->variant.declarator_range.end.seq != 0) end = dpi->variant.declarator_range.end;
  }
  setPosition(params, start, end);

  if (isDefinition) {
    SgBasicBlock* body = SageBuilder::buildBasicBlock_nfi();
    SgFunctionDefinition* def = new SgFunctionDefinition(decl, body);
    decl->set_definition(def);
    def->set_parent(decl);
    body->set_parent(def);
    decl->set_definingDeclaration(decl);
    first->set_definingDeclaration(decl);
    definingRoutineDecl[routine] = decl;
    for (SgInitializedName* in : params->get_args()) {
      in->set_scope(def);
      if (!in->get_name().is_null() && !isSgTypeEllipse(in->get_type())) {
        def->insert_symbol(in->get_name(), new SgVariableSymbol(in));
      }
    }
    scopes[fscope] = def;
    a_statement_ptr bodyStmt = fscope->assoc_block;
    if (bodyStmt != nullptr) {
      setPosition(body, bodyStmt->position, bodyStmt->variant.block.extra_info->final_position);
      end = bodyStmt->variant.block.extra_info->final_position;
    }
    setPosition(def, start, end);
    if (classNesting > 0) {
      deferredBodies.push_back(std::make_pair(routine, decl));
    } else {
      translateFunctionBody(routine, decl);
    }
  } else {
    decl->setForward();
    auto d = definingRoutineDecl.find(routine);
    if (d != definingRoutineDecl.end()) decl->set_definingDeclaration(d->second);
    for (SgInitializedName* in : params->get_args()) in->set_scope(semanticScope);
  }
  setPosition(decl, start, end);
  attachPendingBaseTypeDeclaration(decl);
  return decl;
}

void Translator::translateFunctionBody(a_routine_ptr routine, SgFunctionDeclaration* defining) {
  a_scope_ptr fscope = functionScopeOf(routine);
  SgFunctionDefinition* def = defining->get_definition();
  if (fscope == nullptr || def == nullptr) return;
  a_routine_ptr savedRoutine = currentRoutine;
  SgFunctionDefinition* savedDef = currentFunctionDefinition;
  currentRoutine = routine;
  currentFunctionDefinition = def;
  scopeStack.push_back(def);
  SageBuilder::pushScopeStack(def);
  try {
    if (fscope->assoc_block != nullptr) convertBlock(fscope->assoc_block, def->get_body());
  } catch (const Unsupported& u) {
    warnings++;
    mlog[WARN] << "incomplete translation of the body of " << defining->get_name().getString() << " (" << u.what
               << ")\n";
  }
  SageBuilder::popScopeStack();
  scopeStack.pop_back();
  currentRoutine = savedRoutine;
  currentFunctionDefinition = savedDef;
}

void Translator::finishDeferredFunctionBodies() {
  while (!deferredBodies.empty()) {
    std::vector<std::pair<a_routine_ptr, SgFunctionDeclaration*>> work;
    work.swap(deferredBodies);
    for (auto& w : work) translateFunctionBody(w.first, w.second);
  }
}

// ---------------------------------------------------------------------------------
// Types: classes, enums, typedefs
// ---------------------------------------------------------------------------------

void Translator::translateTypeDeclaration(SeqCursor& cursor, a_type_ptr type, a_src_seq_secondary_decl_ptr sec,
                                          SgScopeStatement* scope) {
  if (type->kind == tk_typeref) {
    cursor.advance();
    if (typeref_is_typedef(type)) {
      SgTypedefDeclaration* td = translateTypedef(type, sec, scope);
      if (sec ? sec->embedded_source_sequence_entries : type->variant.typeref.embedded_source_sequence_entries) {
        translateDeclarationList(cursor, scope, type);
      }
      if (td) appendStatementTo(scope, td);
    } else if (type->variant.typeref.embedded_source_sequence_entries) {
      skipToEndOfConstruct(cursor, type);
    }
    return;
  }

  bool isClass = (type->kind == tk_class || type->kind == tk_struct || type->kind == tk_union);
  bool isEnum = (type->kind == tk_enum && type->variant.integer.enum_type);
  if (!isClass && !isEnum) {
    cursor.advance();
    return;
  }

  bool autonomous = sec ? sec->autonomous_tag_decl : type->autonomous_primary_tag_decl;
  if (sec != nullptr) {
    // A declaration that is not a definition ("struct S;", or the first
    // mention of a tag in another declaration).
    cursor.advance();
    if (!autonomous) {
      if (isClass) classDeclarationFor(type);
      else enumDeclarationFor(type);
      return;
    }
    SgDeclarationStatement* d = nullptr;
    if (isClass) {
      SgClassDeclaration* first = classDeclarationFor(type);
      SgClassDeclaration* fwd = nullptr;
      if (first->get_parent() == scope && first->get_definingDeclaration() == nullptr &&
          !firstUsedAsStatement.count(first)) {
        // The hidden first declaration becomes this forward declaration statement.
        fwd = first;
      } else {
        fwd = new SgClassDeclaration(first->get_name(), first->get_class_type(), first->get_type(), nullptr);
        fwd->set_firstNondefiningDeclaration(first);
        fwd->set_definingDeclaration(first->get_definingDeclaration());
        fwd->setForward();
        fwd->set_scope(first->get_scope());
        fwd->set_isUnNamed(first->get_isUnNamed());
      }
      firstUsedAsStatement.insert(first);
      setPosition(fwd, sec->decl_position);
      d = fwd;
    } else {
      SgEnumDeclaration* first = enumDeclarationFor(type);
      SgEnumDeclaration* fwd = new SgEnumDeclaration(first->get_name(), first->get_type());
      fwd->set_firstNondefiningDeclaration(first);
      fwd->set_definingDeclaration(first->get_definingDeclaration());
      fwd->setForward();
      fwd->set_scope(first->get_scope());
      setPosition(fwd, sec->decl_position);
      d = fwd;
    }
    if (type->source_corresp.is_class_member) setAccess(d, (an_access_specifier)type->source_corresp.access);
    appendStatementTo(scope, d);
    return;
  }

  // A definition
  SgDeclarationStatement* def = nullptr;
  if (isClass) {
    def = translateClassDefinition(cursor, type, scope);
  } else {
    def = translateEnumDefinition(cursor, type, scope);
  }
  if (type->source_corresp.is_class_member) setAccess(def, (an_access_specifier)type->source_corresp.access);
  if (autonomous) {
    appendStatementTo(scope, def);
  } else {
    if (pendingBaseTypeDecl != nullptr) appendStatementTo(scope, pendingBaseTypeDecl);
    pendingBaseTypeDecl = def;
    pendingBaseType = type;
    def->set_parent(scope);
  }
}

SgClassDeclaration* Translator::translateClassDefinition(SeqCursor& cursor, a_type_ptr type, SgScopeStatement* scope) {
  SgClassDeclaration* first = classDeclarationFor(type);
  SgClassDeclaration* def = new SgClassDeclaration(first->get_name(), first->get_class_type(), first->get_type(), nullptr);
  SgClassDefinition* cdef = new SgClassDefinition(def);
  def->set_definition(cdef);
  cdef->set_parent(def);
  def->set_firstNondefiningDeclaration(first);
  def->set_definingDeclaration(def);
  first->set_definingDeclaration(def);
  def->set_scope(first->get_scope());
  def->set_parent(scope);
  def->set_isUnNamed(first->get_isUnNamed());
  definingClassDecl[type] = def;

  a_class_type_supplement_ptr ctsp = type->variant.class_struct_union.extra_info;
  if (ctsp != nullptr && ctsp->assoc_scope != nullptr) scopes[ctsp->assoc_scope] = cdef;

  a_source_position start = type->source_corresp.decl_position;
  if (a_decl_position_supplement_ptr dpi = type->source_corresp.decl_pos_info) {
    if (dpi->specifiers_range.start.seq != 0) start = dpi->specifiers_range.start;
  }
  a_source_position end = start;

  // Members follow the type's entry, up to the end-of-construct entry.
  cursor.advance();
  scopeStack.push_back(cdef);
  SageBuilder::pushScopeStack(cdef);
  classNesting++;
  SgDeclarationStatement* savedPending = pendingBaseTypeDecl;
  pendingBaseTypeDecl = nullptr;
  while (!cursor.atEnd()) {
    if (cursor.kind() == iek_src_seq_end_of_construct) {
      a_src_seq_end_of_construct_ptr eoc = (a_src_seq_end_of_construct_ptr)cursor.ptr();
      cursor.advance();
      if ((void*)eoc->entity.ptr == (void*)type) {
        end = eoc->position;
        break;
      }
      continue;
    }
    translateDeclarationEntry(cursor, cdef);
  }
  if (pendingBaseTypeDecl != nullptr) appendStatementTo(cdef, pendingBaseTypeDecl);
  pendingBaseTypeDecl = savedPending;
  classNesting--;
  SageBuilder::popScopeStack();
  scopeStack.pop_back();

  setPosition(def, start, end);
  setPosition(cdef, start, end);
  if (classNesting == 0) finishDeferredFunctionBodies();
  return def;
}

SgEnumDeclaration* Translator::translateEnumDefinition(SeqCursor& cursor, a_type_ptr type, SgScopeStatement* scope) {
  SgEnumDeclaration* first = enumDeclarationFor(type);
  SgEnumDeclaration* def = new SgEnumDeclaration(first->get_name(), first->get_type());
  def->set_firstNondefiningDeclaration(first);
  def->set_definingDeclaration(def);
  first->set_definingDeclaration(def);
  def->set_scope(first->get_scope());
  def->set_parent(scope);
  def->set_isUnNamed(first->get_isUnNamed());
  def->set_isScopedEnum(first->get_isScopedEnum());
  definingEnumDecl[type] = def;
  SgType* enumType = first->get_type();
  SgScopeStatement* enumeratorScope = first->get_scope();

  a_constant_ptr list = type->variant.integer.is_scoped_enum
                            ? (type->variant.integer.enum_info.assoc_scope ? type->variant.integer.enum_info.assoc_scope->constants
                                                                           : nullptr)
                            : type->variant.integer.enum_info.constant_list;
  for (a_constant_ptr c = list; c != nullptr; c = c->next) {
    SgInitializer* init = nullptr;
    a_decl_position_supplement_ptr dpi = c->source_corresp.decl_pos_info;
    bool explicitValue = dpi != nullptr && dpi->variant.enum_value_range.start.seq != 0;
    if (explicitValue) {
      SgExpression* value = nullptr;
      if (c->expr != nullptr) {
        try {
          value = convertExpression(c->expr);
        } catch (const Unsupported&) {
        }
      }
      if (value == nullptr) {
        // A literal value (or a folded expression without a backing expression)
        a_boolean ovf = FALSE;
        long long v = (long long)value_of_integer_constant(c, &ovf);
        std::string text = sourceText(dpi->variant.enum_value_range.start, dpi->variant.enum_value_range.end);
        if (!isNumericLiteral(text)) text = std::to_string(v);
        if (v >= INT_MIN && v <= INT_MAX) {
          value = SageBuilder::buildIntVal_nfi((int)v, text);
        } else {
          value = SageBuilder::buildLongLongIntVal_nfi(v, text);
        }
        setPosition(value, dpi->variant.enum_value_range.start, dpi->variant.enum_value_range.end);
      }
      init = SageBuilder::buildAssignInitializer_nfi(value, enumType);
      setPosition(init, dpi->variant.enum_value_range.start, dpi->variant.enum_value_range.end);
    }
    SgInitializedName* in = SageBuilder::buildInitializedName_nfi(nameOf(&c->source_corresp), enumType, init);
    if (init) init->set_parent(in);
    setPosition(in, c->source_corresp.decl_position);
    def->append_enumerator(in);
    in->set_parent(def);
    in->set_scope(enumeratorScope);
    enumerators[c] = in;
    if (!def->get_isScopedEnum()) {
      enumeratorScope->insert_symbol(in->get_name(), new SgEnumFieldSymbol(in));
    }
  }

  a_source_position start = type->source_corresp.decl_position;
  if (a_decl_position_supplement_ptr dpi = type->source_corresp.decl_pos_info) {
    if (dpi->specifiers_range.start.seq != 0) start = dpi->specifiers_range.start;
  }
  cursor.advance();
  a_source_position end = start;
  // Skip to the end of the enum definition.
  while (!cursor.atEnd()) {
    if (cursor.kind() == iek_src_seq_end_of_construct) {
      a_src_seq_end_of_construct_ptr eoc = (a_src_seq_end_of_construct_ptr)cursor.ptr();
      cursor.advance();
      if ((void*)eoc->entity.ptr == (void*)type) {
        end = eoc->position;
        break;
      }
      continue;
    }
    cursor.advance();
  }
  setPosition(def, start, end);
  return def;
}

SgEnumFieldSymbol* Translator::enumeratorSymbolFor(a_constant_ptr enumerator) {
  auto it = enumerators.find(enumerator);
  if (it == enumerators.end()) return nullptr;
  SgInitializedName* in = it->second;
  SgEnumFieldSymbol* sym = isSgEnumFieldSymbol(in->search_for_symbol_from_symbol_table());
  if (sym == nullptr) {
    sym = new SgEnumFieldSymbol(in);
    in->get_scope()->insert_symbol(in->get_name(), sym);
  }
  return sym;
}

SgTypedefDeclaration* Translator::translateTypedef(a_type_ptr type, a_src_seq_secondary_decl_ptr sec,
                                                  SgScopeStatement* scope) {
  a_type_ptr baseType = sec != nullptr && sec->declared_type != nullptr ? sec->declared_type : type->variant.typeref.type;
  SgName name = nameOf(&type->source_corresp);
  SgTypedefDeclaration* decl = nullptr;
  auto existing = typedefDecls.find(type);
  if (existing != typedefDecls.end() && sec == nullptr && existing->second->get_parent() == scope &&
      !typedefInStatementList.count(existing->second)) {
    decl = existing->second;  // created on demand before its declaration was reached
  } else {
    SgType* base = convertType(baseType);
    decl = new SgTypedefDeclaration(name, base, nullptr, nullptr, nullptr);
    decl->set_scope(scope);
    decl->set_parent(scope);
    if (existing == typedefDecls.end()) {
      decl->set_firstNondefiningDeclaration(decl);
      decl->set_definingDeclaration(decl);
      decl->set_type(SgTypedefType::createType(decl));
      typedefDecls[type] = decl;
      scope->insert_symbol(name, new SgTypedefSymbol(decl));
    } else {
      // A repeated typedef (allowed in C11 and C++)
      decl->set_firstNondefiningDeclaration(existing->second);
      decl->set_definingDeclaration(existing->second);
      decl->set_type(existing->second->get_type());
    }
  }
  typedefInStatementList.insert(decl);
  if (type->source_corresp.is_class_member) setAccess(decl, (an_access_specifier)type->source_corresp.access);
  a_source_position start = sec ? sec->decl_position : type->source_corresp.decl_position;
  a_source_position end = start;
  a_decl_position_supplement_ptr dpi = sec ? sec->decl_pos_info : type->source_corresp.decl_pos_info;
  if (dpi != nullptr) {
    if (dpi->specifiers_range.start.seq != 0) start = dpi->specifiers_range.start;
    if (dpi->variant.declarator_range.end.seq != 0) end = dpi->variant.declarator_range.end;
  }
  setPosition(decl, start, end);
  attachPendingBaseTypeDeclaration(decl);
  setTypedefBaseDeclaration(decl);
  return decl;
}

// ---------------------------------------------------------------------------------
// Pragmas and static assertions
// ---------------------------------------------------------------------------------

SgPragmaDeclaration* Translator::translatePragma(a_pragma_ptr pp) {
  if (pp->ignore_in_back_end || pp->pragma_text == nullptr) return nullptr;
  SgPragma* pragma = new SgPragma(std::string(pp->pragma_text));
  SgPragmaDeclaration* decl = new SgPragmaDeclaration(pragma);
  pragma->set_parent(decl);
  decl->set_firstNondefiningDeclaration(decl);
  decl->set_definingDeclaration(nullptr);
  setPosition(decl, pp->position);
  Sg_File_Info* fi = fileInfo(pp->position);
  pragma->set_startOfConstruct(fi);
  fi->set_parent(pragma);
  Sg_File_Info* fe = fileInfo(pp->position);
  pragma->set_endOfConstruct(fe);
  fe->set_parent(pragma);
  return decl;
}

SgDeclarationStatement* Translator::translateStaticAssertion(a_static_assertion_ptr sa) {
  SgExpression* cond = convertConstant(sa->condition);
  SgName msg("");
  if (sa->string_literal != nullptr && sa->string_literal->kind == ck_string) {
    msg = SgName(std::string(sa->string_literal->variant.string.value,
                             sa->string_literal->variant.string.length > 0 ? sa->string_literal->variant.string.length - 1 : 0));
  }
  SgStaticAssertionDeclaration* decl = new SgStaticAssertionDeclaration(cond, msg);
  cond->set_parent(decl);
  decl->set_firstNondefiningDeclaration(decl);
  setPosition(decl, sa->position);
  return decl;
}

// ---------------------------------------------------------------------------------
// Declarations inside function bodies (stmk_decl statements)
// ---------------------------------------------------------------------------------

void Translator::translateDeclarationStatement(a_statement_ptr stmt, SgScopeStatement* scope) {
  // The declarations of the statement follow the statement's own source
  // sequence entry, up to the next statement (or the end of the block).
  if (stmt->source_sequence_entry == nullptr) {
    // No source sequence information: translate the declared entities directly.
    for (an_il_entity_list_entry_ptr e = stmt->variant.decl.entities; e != nullptr; e = e->next) {
      try {
        if (e->entity.kind == iek_variable) {
          appendStatementTo(scope, translateVariable((a_variable_ptr)e->entity.ptr, nullptr, scope));
        }
      } catch (const Unsupported& u) {
        warnings++;
        mlog[WARN] << "skipping local declaration (" << u.what << ")\n";
      }
    }
    return;
  }
  SeqCursor cursor(stmt->source_sequence_entry);
  cursor.advance();
  while (!cursor.atEnd()) {
    an_il_entry_kind k = cursor.kind();
    if (k == iek_statement || k == iek_src_seq_end_of_construct) break;
    translateDeclarationEntry(cursor, scope);
  }
  if (pendingBaseTypeDecl != nullptr) {
    SgDeclarationStatement* base = pendingBaseTypeDecl;
    pendingBaseTypeDecl = nullptr;
    appendStatementTo(scope, base);
  }
}

}  // namespace edg2sage
