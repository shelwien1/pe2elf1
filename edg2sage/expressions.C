// Translation of expressions, constants and initializers.
#include "edg2sage.h"

#include "const_ints.h"
#include "float_pt.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

using namespace edg;
using namespace Sawyer::Message;

namespace edg2sage {

namespace {

template <class T>
SgExpression* unaryOp(SgExpression* operand) {
  return SageBuilder::buildUnaryExpression_nfi<T>(operand);
}

template <class T>
SgExpression* binaryOp(SgExpression* lhs, SgExpression* rhs) {
  return SageBuilder::buildBinaryExpression_nfi<T>(lhs, rhs);
}

// Expression positions: the full range if known, otherwise the node's position.
void rangeOf(an_expr_node_ptr expr, a_source_position& start, a_source_position& end) {
  start = expr->expr_range.start;
  end = expr->expr_range.end;
  if (start.seq == 0) start = expr->position;
  if (end.seq == 0) end = start;
}

// Characters of a string or character literal, escaped for use between quotes.
std::string escapeCharacter(unsigned long c, char quote) {
  switch (c) {
    case '\n': return "\\n";
    case '\t': return "\\t";
    case '\r': return "\\r";
    case '\a': return "\\a";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\v': return "\\v";
    case '\\': return "\\\\";
    default: break;
  }
  if (c == (unsigned char)quote) return std::string("\\") + quote;
  if (c >= 0x20 && c < 0x7f) return std::string(1, (char)c);
  char buf[16];
  if (c < 0x200) {
    std::snprintf(buf, sizeof buf, "\\%03lo", c);
  } else if (c < 0x10000 && !(c >= 0xd800 && c < 0xe000)) {
    std::snprintf(buf, sizeof buf, "\\u%04lx", c);
  } else {
    std::snprintf(buf, sizeof buf, "\\U%08lx", c);
  }
  return buf;
}

// Strips an encoding prefix (L, u, U, u8) from a literal's spelling; returns the
// position of the opening quote, or npos.
size_t literalStart(const std::string& text, char quote) {
  size_t i = 0;
  if (i < text.size() && (text[i] == 'L' || text[i] == 'U')) {
    i++;
  } else if (i < text.size() && text[i] == 'u') {
    i++;
    if (i < text.size() && text[i] == '8') i++;
  }
  if (i < text.size() && text[i] == quote) return i;
  return std::string::npos;
}

// A single (or concatenated) narrow/wide string literal spelled in the source:
// returns the characters between the first and last quote.
bool stringLiteralBody(const std::string& text, std::string& body) {
  size_t b = literalStart(text, '"');
  if (b == std::string::npos || text.size() < b + 2 || text.back() != '"') return false;
  body = text.substr(b + 1, text.size() - b - 2);
  return true;
}

bool isCharacterLiteral(const std::string& text) {
  size_t b = literalStart(text, '\'');
  return b != std::string::npos && text.size() >= b + 3 && text.back() == '\'';
}

}  // namespace

bool isNumericLiteral(const std::string& text) {
  if (text.empty()) return false;
  if (!(isdigit((unsigned char)text[0]) ||
        (text[0] == '.' && text.size() > 1 && isdigit((unsigned char)text[1])))) {
    return false;
  }
  for (size_t i = 0; i < text.size(); i++) {
    char c = text[i];
    if (isalnum((unsigned char)c) || c == '.' || c == '\'') continue;
    if ((c == '+' || c == '-') && i > 0 &&
        (text[i - 1] == 'e' || text[i - 1] == 'E' || text[i - 1] == 'p' || text[i - 1] == 'P')) {
      continue;
    }
    return false;
  }
  return true;
}

namespace {

bool isCastOperator(an_expr_operator_kind k) {
  switch (k) {
    case eok_cast:
    case eok_lvalue_cast:
    case eok_ref_cast:
    case eok_base_class_cast:
    case eok_derived_class_cast:
    case eok_pm_base_class_cast:
    case eok_pm_derived_class_cast:
    case eok_dynamic_cast:
    case eok_ref_dynamic_cast:
    case eok_bool_cast:
      return true;
    default:
      return false;
  }
}

}  // namespace

static std::string literalSpelling(Translator* t, a_constant_ptr con, an_expr_node_ptr node);

// ---------------------------------------------------------------------------------
// Positions
// ---------------------------------------------------------------------------------

void Translator::setExpressionPosition(SgExpression* e, an_expr_node_ptr expr) {
  if (e == nullptr || expr == nullptr) return;
  if (e->get_startOfConstruct() != nullptr) return;  // already positioned
  if (expr->compiler_generated) {
    setCompilerGenerated(e);
  } else {
    a_source_position start, end;
    rangeOf(expr, start, end);
    if (start.seq == 0) {
      setCompilerGenerated(e);
    } else {
      setPosition(e, start, end);
      if (expr->position.seq != 0 &&
          (expr->position.seq != start.seq || expr->position.column != start.column)) {
        Sg_File_Info* o = fileInfo(expr->position);
        delete e->get_operatorPosition();
        e->set_operatorPosition(o);
        o->set_parent(e);
      }
    }
  }
  if (expr->is_parenthesized) e->set_need_paren(true);
}

bool Translator::isImplicitNode(an_expr_node_ptr expr) {
  return expr != nullptr && expr->compiler_generated;
}

// Skips conversions that have no representation in Sage (they are implied by the
// types of the operands).
an_expr_node_ptr Translator::skipImplicitSteps(an_expr_node_ptr expr) {
  while (expr != nullptr && expr->kind == enk_operation) {
    an_expr_operator_kind k = expr->variant.operation.kind;
    if (k == eok_array_to_pointer || k == eok_lvalue_adjust || k == eok_class_rvalue_adjust ||
        k == eok_ref_indirect || k == eok_reference_to || k == eok_lvalue || k == eok_parens ||
        ((k == eok_address_of || k == eok_indirect) && expr->compiler_generated)) {
      if (expr->is_parenthesized) break;
      expr = expr->variant.operation.operands;
      continue;
    }
    break;
  }
  return expr;
}

// ---------------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------------

// An aggregate initializer without elements ("{}")
static bool isEmptyAggregate(SgExpression* e) {
  SgAggregateInitializer* ai = isSgAggregateInitializer(e);
  return ai != nullptr && (ai->get_initializers() == nullptr || ai->get_initializers()->get_expressions().empty());
}

// "T()": a value-initialized temporary.  The type is written without
// typedefs (ROSE qualifies the name of a class here, not of a typedef).
// "T()": a value-initialized temporary.  Classes are referred to through
// their default constructor (for a class without user-provided constructors,
// a declaration of the implicit one): ROSE's unparser omits the name of a class
// template instance in the arguments of a constructor call in a declaration
// ("H h((L<int>()))" became "H h((()))") unless it is printed as the
// qualifier of the constructor.  Other types, e.g. pointers, are referred to by
// their typedef name ("X()"; "struct S *()" would not parse).
SgConstructorInitializer* Translator::valueInitializedTemporary(SgType* type) {
  SgType* t = type != nullptr ? type->stripType(SgType::STRIP_TYPEDEF_TYPE | SgType::STRIP_MODIFIER_TYPE) : type;
  SgMemberFunctionDeclaration* ctor = nullptr;
  if (SgClassType* ct = isSgClassType(t)) {
    if (isCxx) ctor = implicitDefaultConstructor(ct);
  } else if (type != nullptr) {
    t = type->stripType(SgType::STRIP_MODIFIER_TYPE);
  }
  SgExprListExp* args = SageBuilder::buildExprListExp_nfi();
  SgConstructorInitializer* ci = SageBuilder::buildConstructorInitializer_nfi(ctor, args, t, true, false, true, true);
  args->set_parent(ci);
  if (isSgClassType(t)) ci->set_associated_class_unknown(false);
  return ci;
}

// The default constructor of a class: the declared one, or a (hidden)
// declaration of the implicit one.
SgMemberFunctionDeclaration* Translator::implicitDefaultConstructor(SgClassType* ct) {
  SgDeclarationStatement* first = ct->get_declaration();
  if (first != nullptr && first->get_firstNondefiningDeclaration() != nullptr) first = first->get_firstNondefiningDeclaration();
  auto ti = classTypes.find(first);
  if (ti == classTypes.end()) return nullptr;
  a_type_ptr cls = ti->second;
  auto it = implicitConstructors.find(cls);
  if (it != implicitConstructors.end()) return it->second;
  SgMemberFunctionDeclaration* d = nullptr;
  a_class_type_supplement_ptr ctsp = cls->variant.class_struct_union.extra_info;
  for (a_routine_ptr r = ctsp != nullptr && ctsp->assoc_scope ? ctsp->assoc_scope->routines : nullptr; r != nullptr; r = r->next) {
    if (r->special_kind != sfk_constructor) continue;
    a_type_ptr rt = skip_typerefs(r->type);
    a_routine_type_supplement_ptr rtsp = rt->kind == tk_routine ? rt->variant.routine.extra_info : nullptr;
    if (rtsp != nullptr && (rtsp->param_type_list == nullptr || rtsp->param_type_list->has_default_arg)) {
      d = isSgMemberFunctionDeclaration(functionDeclarationFor(r));
      break;
    }
  }
  SgClassDefinition* cdef = d == nullptr ? classDefinitionFor(cls) : nullptr;
  if (d == nullptr && cdef != nullptr) {
    SgClassDeclaration* cdecl = isSgClassDeclaration(first);
    SgName name = cdecl->get_name();
    if (SgTemplateInstantiationDecl* tid = isSgTemplateInstantiationDecl(cdecl)) name = tid->get_templateName();
    SgMemberFunctionType* ft = SageBuilder::buildMemberFunctionType(SgTypeVoid::createType(), new SgFunctionParameterTypeList(),
                                                                    cdef, 0);
    d = new SgMemberFunctionDeclaration(name, ft, nullptr);
    SgFunctionParameterList* params = SageBuilder::buildFunctionParameterList_nfi();
    SgFunctionParameterList* old = d->get_parameterList();
    d->set_parameterList(params);
    params->set_parent(d);
    if (old != nullptr && old != params) delete old;
    d->set_scope(cdef);
    d->set_parent(cdef);
    d->set_firstNondefiningDeclaration(d);
    d->setForward();
    d->get_specialFunctionModifier().setConstructor();
    setCompilerGenerated(d);
    setCompilerGenerated(params);
    cdef->insert_symbol(name, new SgMemberFunctionSymbol(d));
  }
  implicitConstructors[cls] = d;
  return d;
}

SgExpression* Translator::convertExpression(an_expr_node_ptr expr) {
  if (expr == nullptr) {
    SgExpression* e = SageBuilder::buildNullExpression_nfi();
    setCompilerGenerated(e);
    return e;
  }
  SgExpression* result = nullptr;
  switch (expr->kind) {
    case enk_operation:
      result = convertOperation(expr);
      break;
    case enk_constant:
      result = convertConstant(expr->variant.constant.ptr, expr);
      break;
    case enk_variable:
      result = convertVariableReference(expr->variant.variable.ptr, expr);
      break;
    case enk_field: {
      SgVariableSymbol* sym = fieldSymbolFor(expr->variant.field.ptr);
      result = SageBuilder::buildVarRefExp_nfi(sym);
      break;
    }
    case enk_routine:
      result = convertRoutineReference(expr->variant.routine.ptr, expr);
      break;
    case enk_temp_init:
      result = convertTempInit(expr);
      break;
    case enk_sizeof:
    case enk_alignof:
      result = convertSizeof(expr);
      break;
    case enk_statement:
      result = convertStatementExpression(expr);
      break;
    case enk_object_lifetime:
      result = convertExpression(expr->variant.object_lifetime.expr);
      if (expr->is_parenthesized) result->set_need_paren(true);
      return result;
    case enk_new_delete:
      result = convertNewDelete(expr);
      break;
    case enk_throw: {
      SgExpression* operand = nullptr;
      a_throw_supplement_ptr ti = expr->variant.throw_info;
      if (ti != nullptr && ti->dynamic_init != nullptr) {
        operand = initializerExpression(convertDynamicInit(ti->dynamic_init, convertType(ti->type)));
        if (SgConstructorInitializer* ci = isSgConstructorInitializer(operand)) {
          // "throw T(args)"
          ci->set_need_name(true);
          ci->set_need_parenthesis_after_name(true);
        } else if (isEmptyAggregate(operand)) {
          // "throw T()" of a class without a user-provided constructor
          operand = valueInitializedTemporary(convertType(ti->type));
        }
      }
      SgThrowOp* t = new SgThrowOp(operand, convertType(expr->type),
                                   operand ? SgThrowOp::throw_expression : SgThrowOp::rethrow);
      if (operand) operand->set_parent(t);
      result = t;
      break;
    }
    case enk_typeid: {
      an_expr_node_ptr te = expr->variant.typeid_info.type_with_opt_expr;
      SgTypeIdOp* t = nullptr;
      if (te != nullptr && te->kind == enk_type_operand) {
        t = new SgTypeIdOp(nullptr, convertType(te->variant.type_operand.type));
      } else {
        SgExpression* op = convertExpression(te);
        t = new SgTypeIdOp(op, nullptr);
        op->set_parent(t);
      }
      result = t;
      break;
    }
    case enk_condition: {
      a_condition_supplement_ptr cs = expr->variant.condition;
      if (cs != nullptr && cs->expr != nullptr) return convertExpression(cs->expr);
      throw Unsupported("condition declaration in expression");
    }
    case enk_c11_generic:
      // _Generic: the selected association
      result = convertExpression(expr->variant.c11_generic.result);
      if (expr->is_parenthesized) result->set_need_paren(true);
      return result;
#if BUILTIN_FUNCTIONS_ENABLED
    case enk_builtin_choose_expr: {
      an_expr_node_ptr ops = expr->variant.builtin_choose_expr.operands;
      an_expr_node_ptr chosen = ops != nullptr ? ops->next : nullptr;
      if (chosen != nullptr && !expr->variant.builtin_choose_expr.choose_first) chosen = chosen->next;
      if (chosen == nullptr) throw Unsupported("__builtin_choose_expr");
      return convertExpression(chosen);
    }
#endif
    case enk_braced_init_list: {
      SgExprListExp* list = convertArgumentList(expr->variant.braced_init_list);
      SgBracedInitializer* bi = SageBuilder::buildBracedInitializer_nfi(list, convertType(expr->type));
      result = bi;
      break;
    }
    case enk_initializer: {
      SgInitializer* init = convertDynamicInit(expr->variant.initializer.dyn_init, convertType(expr->type));
      if (init == nullptr) throw Unsupported("empty initializer expression");
      result = init;
      break;
    }
    case enk_type_operand:
      throw Unsupported("type operand");
    case enk_builtin_operation:
      throw Unsupported("builtin operation");
    case enk_reuse_value:
      throw Unsupported("reused value");
    case enk_lambda:
      result = convertLambda(expr->variant.init.source.lambda);
      break;
    case enk_address_of_ellipsis:
      throw Unsupported("address of ellipsis");
    default:
      throw Unsupported("expression kind " + std::to_string((int)expr->kind));
  }
  setExpressionPosition(result, expr);
  return result;
}

// A copy of a default argument expression, supplied by the front end (possibly
// converted to the parameter type).
static bool isGeneratedDefaultArgument(an_expr_node_ptr a) {
  for (int depth = 0; a != nullptr && depth < 10; depth++) {
    if (a->generated_default_arg) return true;
    if (a->kind != enk_operation || !a->compiler_generated || a->variant.operation.operands == nullptr) break;
    a = a->variant.operation.operands;
  }
  return false;
}

SgExprListExp* Translator::convertArgumentList(an_expr_node_ptr first) {
  SgExprListExp* list = SageBuilder::buildExprListExp_nfi();
  for (an_expr_node_ptr a = first; a != nullptr; a = a->next) {
    // Default arguments are not written in the call (they are trailing).
    if (isCxx && isGeneratedDefaultArgument(a)) break;
    SgExpression* e = convertExpression(a);
    list->append_expression(e);
    e->set_parent(list);
  }
  setCompilerGenerated(list);
  return list;
}

SgExpression* Translator::convertVariableReference(a_variable_ptr var, an_expr_node_ptr expr) {
  if (var->is_this_parameter) {
    a_type_ptr pt = skip_typerefs(var->type);
    a_type_ptr ct = pt->kind == tk_pointer ? pt->variant.pointer.type : nullptr;
    SgClassSymbol* csym = nullptr;
    if (ct != nullptr) {
      SgClassDeclaration* cd = classDeclarationFor(skip_typerefs(ct));
      if (cd != nullptr) {
        csym = isSgClassSymbol(cd->search_for_symbol_from_symbol_table());
      }
    }
    SgThisExp* t = SageBuilder::buildThisExp_nfi(csym);
    return t;
  }
  if (var->is_compound_literal) {
    // A file-scope compound literal is a compiler-generated variable.
    SgType* type = convertType(var->type);
    SgInitializer* init = convertVariableInitializer(var);
    SgAggregateInitializer* ai = isSgAggregateInitializer(init);
    if (ai == nullptr) {
      SgExprListExp* list = SageBuilder::buildExprListExp_nfi();
      if (init != nullptr) {
        list->append_expression(init);
        init->set_parent(list);
      }
      setCompilerGenerated(list);
      ai = SageBuilder::buildAggregateInitializer_nfi(list, type);
      setCompilerGenerated(ai);
    }
    ai->set_uses_compound_literal(true);
    ai->set_need_explicit_braces(true);
    SgName name = "__compound_literal_" + std::to_string(++compoundLiterals);
    SgInitializedName* iname = SageBuilder::buildInitializedName_nfi(name, type, ai);
    ai->set_parent(iname);
    iname->set_scope(currentScope());
    setCompilerGenerated(iname);
    SgVariableSymbol* sym = new SgVariableSymbol(iname);
    SgCompoundLiteralExp* cl = SageBuilder::buildCompoundLiteralExp_nfi(sym);
    return cl;
  }
  if (var->compiler_generated && var->source_corresp.name == nullptr && var->init_kind == initk_static &&
      var->initializer.constant != nullptr && var->initializer.constant->kind == ck_string) {
    // The implicit variable of __func__, __FUNCTION__ or __PRETTY_FUNCTION__:
    // the keyword if it is spelled in the source here, else its value.
    std::string text;
    if (expr != nullptr) {
      a_source_position start, end;
      rangeOf(expr, start, end);
      text = sourceText(start, end);
    }
    if (text == "__func__" || text == "__FUNCTION__" || text == "__PRETTY_FUNCTION__") {
      return SageBuilder::buildVarRefExp_nfi(functionNameSymbol(text, convertType(var->type)));
    }
    return convertStringConstant(var->initializer.constant, nullptr);
  }
  SgVariableSymbol* sym = variableSymbolFor(var);
  return SageBuilder::buildVarRefExp_nfi(sym);
}

// A hidden declaration of __func__ (or __FUNCTION__, __PRETTY_FUNCTION__) in the
// current function.
SgVariableSymbol* Translator::functionNameSymbol(const std::string& name, SgType* type) {
  SgScopeStatement* scope = currentFunctionDefinition != nullptr ? (SgScopeStatement*)currentFunctionDefinition
                                                                  : (SgScopeStatement*)globalScope;
  auto key = std::make_pair(scope, name);
  auto it = functionNameSymbols.find(key);
  if (it != functionNameSymbols.end()) return it->second;
  SgInitializedName* in = SageBuilder::buildInitializedName_nfi(SgName(name), type, nullptr);
  SgVariableDeclaration* decl = new SgVariableDeclaration(in);
  decl->set_firstNondefiningDeclaration(decl);
  decl->get_declarationModifier().get_storageModifier().setStatic();
  in->set_scope(scope);
  decl->set_parent(scope);
  setCompilerGenerated(decl);
  setCompilerGenerated(in);
  SgVariableSymbol* sym = new SgVariableSymbol(in);
  scope->insert_symbol(in->get_name(), sym);
  functionNameSymbols[key] = sym;
  return sym;
}

SgExpression* Translator::convertRoutineReference(a_routine_ptr routine, an_expr_node_ptr expr) {
  if (routine == nullptr) throw Unsupported("reference to unnamed routine");
#if DEFAULT_RECORD_FORM_OF_NAME_REFERENCE
  if (expr != nullptr && expr->kind == enk_routine && expr->variant.routine.name_reference != nullptr &&
      expr->variant.routine.name_reference->special_kind == sfk_gnu_sync_concrete_function) {
    // A GNU __sync_... or __atomic_... builtin, written with its generic name
    // and resolved to the version for the operand size (e.g. "..._8").
    std::string name = nameOf(&routine->source_corresp).getString();
    size_t u = name.find_last_of('_');
    if (u != std::string::npos && u + 1 < name.size() &&
        name.find_first_not_of("0123456789", u + 1) == std::string::npos) {
      return SageBuilder::buildFunctionRefExp_nfi(genericBuiltinSymbol(name.substr(0, u), routine));
    }
  }
#endif
  SgFunctionSymbol* sym = functionSymbolFor(routine);
  if (SgMemberFunctionSymbol* msym = isSgMemberFunctionSymbol(sym)) {
    // Outside a member access ("A::f(1)", "&A::f") the name is qualified as
    // needed (ROSE computes the qualifier); see also convertCall().
    return SageBuilder::buildMemberFunctionRefExp_nfi(msym, false, true);
  }
  return SageBuilder::buildFunctionRefExp_nfi(sym);
}

// A declaration for the generic name of an overloaded GNU builtin (not part
// of any statement list).
SgFunctionSymbol* Translator::genericBuiltinSymbol(const std::string& name, a_routine_ptr concrete) {
  auto it = genericBuiltins.find(name);
  if (it != genericBuiltins.end()) return it->second;
  SgFunctionType* ftype = convertFunctionType(concrete->type);
  SgFunctionDeclaration* decl = new SgFunctionDeclaration(SgName(name), ftype, nullptr);
  decl->get_parameterList()->set_parent(decl);
  decl->set_firstNondefiningDeclaration(decl);
  decl->setForward();
  decl->set_scope(globalScope);
  decl->set_parent(globalScope);
  setCompilerGenerated(decl);
  setCompilerGenerated(decl->get_parameterList());
  SgFunctionSymbol* sym = new SgFunctionSymbol(decl);
  globalScope->insert_symbol(SgName(name), sym);
  genericBuiltins[name] = sym;
  return sym;
}

SgExpression* Translator::convertCast(an_expr_node_ptr expr, SgExpression* operand, bool implicit) {
  an_expr_operator_kind k = expr->variant.operation.kind;
  SgType* type = convertType(expr->type);
  if (k == eok_ref_dynamic_cast || expr->variant.operation.is_reference_cast) {
    if (expr->variant.operation.is_rvalue_reference_cast) {
      type = SageBuilder::buildRvalueReferenceType(type);
    } else {
      type = SageBuilder::buildReferenceType(type);
    }
  }
  SgCastExp::cast_type_enum kind = SgCastExp::e_C_style_cast;
  if (!implicit) {
    if (k == eok_dynamic_cast || k == eok_ref_dynamic_cast) {
      kind = SgCastExp::e_dynamic_cast;
    } else if (expr->variant.operation.is_reinterpret_cast) {
      kind = SgCastExp::e_reinterpret_cast;
    } else if (expr->variant.operation.is_const_cast) {
      kind = SgCastExp::e_const_cast;
    } else if (expr->is_static_cast) {
      kind = SgCastExp::e_static_cast;
    }
  }
  SgCastExp* c = SageBuilder::buildCastExp_nfi(operand, type, kind);
  operand->set_parent(c);
  if (implicit) {
    setCompilerGenerated(c);
  } else if (SgDeclarationStatement* def = typeDefinitionInExpression(expr->type)) {
    c->set_castContainsBaseTypeDefiningDeclaration(true);
    def->set_parent(c);
  }
  return c;
}

// x.<anonymous member>: the selection of an unnamed struct/union member, which
// is implicit in the source (its members are selected as if they were members
// of the enclosing class).
static bool isAnonymousMemberSelection(an_expr_node_ptr e) {
  if (e == nullptr || e->kind != enk_operation) return false;
  an_expr_operator_kind k = e->variant.operation.kind;
  if (k != eok_dot_field && k != eok_points_to_field) return false;
  an_expr_node_ptr m = e->variant.operation.operands ? e->variant.operation.operands->next : nullptr;
  return m != nullptr && m->kind == enk_field && m->variant.field.ptr != nullptr &&
         m->variant.field.ptr->source_corresp.name == nullptr;
}

SgExpression* Translator::convertFieldSelection(an_expr_node_ptr expr, bool arrow) {
  an_expr_node_ptr objectNode = expr->variant.operation.operands;
  an_expr_node_ptr memberNode = objectNode->next;
  while (isAnonymousMemberSelection(objectNode)) {
    arrow = objectNode->variant.operation.kind == eok_points_to_field;
    objectNode = objectNode->variant.operation.operands;
  }
  if (isAnonymousMemberSelection(expr)) {
    // A selection of the anonymous member itself (e.g. to copy it)
    throw Unsupported("reference to an anonymous member");
  }
  if (expr->is_objectless_nonstatic_data_mem_ref) {
    // "sizeof(S::member)": a member named without an object (represented as
    // a member of a null pointer)
    return convertExpression(memberNode);
  }
  if (objectNode->kind == enk_variable && objectNode->variant.variable.ptr != nullptr &&
      objectNode->variant.variable.ptr->is_anonymous_parent_object) {
    // A member of an anonymous union is named directly.
    return convertExpression(memberNode);
  }
  if (memberNode->kind == enk_field && memberNode->variant.field.ptr != nullptr) {
    // A variable captured by a lambda, accessed through the closure object
    auto cap = capturedVariables.find(memberNode->variant.field.ptr);
    if (cap != capturedVariables.end()) {
      an_expr_node_ptr obj = skipImplicitSteps(objectNode);
      if (obj != nullptr && obj->kind == enk_variable && obj->variant.variable.ptr != nullptr &&
          obj->variant.variable.ptr->is_this_parameter) {
        SgExpression* e = convertVariableReference(cap->second, nullptr);
        if (expr->is_parenthesized) e->set_need_paren(true);
        return e;
      }
    }
  }
  SgExpression* object = convertExpression(objectNode);
  // The implicit "this->" of a member named in a member function (or of a
  // captured variable in a lambda) is not written.
  if (expr->compiler_generated && isSgThisExp(object)) setCompilerGenerated(object);
  SgExpression* member = convertExpression(memberNode);
  SgExpression* r = arrow ? binaryOp<SgArrowExp>(object, member) : binaryOp<SgDotExp>(object, member);
  return r;
}

// The classes in which C++ name lookup of member "name" in class "type" finds
// a declaration (more than one: the name is ambiguous there, e.g. a member of
// two base classes).
static void classesDeclaring(a_type_ptr type, const char* name, std::set<a_type_ptr>& found) {
  type = skip_typerefs(type);
  if (type == nullptr || (type->kind != tk_class && type->kind != tk_struct && type->kind != tk_union)) return;
  auto named = [name](const char* n) { return n != nullptr && std::strcmp(n, name) == 0; };
  a_class_type_supplement_ptr ctsp = type->variant.class_struct_union.extra_info;
  bool declares = false;
  for (a_field_ptr f = type->variant.class_struct_union.field_list; f != nullptr && !declares; f = f->next) {
    declares = named(f->source_corresp.name);
  }
  a_scope_ptr scope = ctsp != nullptr ? ctsp->assoc_scope : nullptr;
  for (a_routine_ptr r = scope ? scope->routines : nullptr; r != nullptr && !declares; r = r->next) {
    declares = named(r->source_corresp.name);
  }
  for (a_variable_ptr v = scope ? scope->variables : nullptr; v != nullptr && !declares; v = v->next) {
    declares = named(v->source_corresp.name);
  }
  if (declares) {
    found.insert(type);
    return;
  }
  for (a_base_class_ptr bc = ctsp ? ctsp->direct_base_classes : nullptr; bc != nullptr; bc = bc->next_direct) {
    classesDeclaring(bc->type, name, found);
  }
}

SgExpression* Translator::convertCall(an_expr_node_ptr expr) {
  an_expr_operator_kind k = expr->variant.operation.kind;
  an_expr_node_ptr first = expr->variant.operation.operands;
  SgExpression* function = nullptr;
  an_expr_node_ptr args = nullptr;
  switch (k) {
    case eok_call:
      function = convertExpression(first);
      args = first->next;
      break;
    case eok_dot_member_call:
    case eok_points_to_member_call: {
      an_expr_node_ptr objectNode = first->next;
      if (k == eok_dot_member_call && expr->compiler_generated && first->kind == enk_routine &&
          first->variant.routine.ptr != nullptr && first->variant.routine.ptr->special_kind == sfk_conversion) {
        // An implicit conversion by a conversion function ("operator T()"):
        // written as the object itself.
        SgExpression* object = convertExpression(objectNode);
        if (expr->is_parenthesized) object->set_need_paren(true);
        return object;
      }
      SgExpression* member = convertExpression(first);
      if (SgMemberFunctionRefExp* mref = isSgMemberFunctionRefExp(member)) {
        mref->set_virtual_call(expr->variant.operation.is_virtual_call);
        // "p->Base::f()": the name was written with a qualifier (or a call of
        // a virtual function is not virtual).
        bool qualified = expr->variant.operation.call_with_qualified_function_name;
        if (first->kind == enk_routine) {
          a_name_reference_ptr nr = first->variant.routine.name_reference;
          if (nr != nullptr && (nr->qualifier != nullptr || nr->is_global_qualified_name)) qualified = true;
          a_routine_ptr r = first->variant.routine.ptr;
          if (r != nullptr && r->is_virtual && !expr->variant.operation.is_virtual_call) qualified = true;
        }
        mref->set_need_qualifier(qualified);
        // "A::f()" in a member function (a qualified call on this): ROSE only
        // qualifies member function names outside member accesses.
        an_expr_node_ptr obj = skipImplicitSteps(objectNode);
        while (obj != nullptr && obj->kind == enk_operation && obj->compiler_generated &&
               (obj->variant.operation.kind == eok_base_class_cast || obj->variant.operation.kind == eok_cast)) {
          obj = skipImplicitSteps(obj->variant.operation.operands);
        }
        if (qualified && k == eok_points_to_member_call && obj != nullptr && obj->kind == enk_variable &&
            obj->variant.variable.ptr != nullptr && obj->variant.variable.ptr->is_this_parameter) {
          // Unless the name is ambiguous in the class of this ("A::f()" with
          // f in two bases): ROSE's name qualification then qualifies the
          // member access "this->A::f()", through the (implicit) base class cast.
          a_type_ptr thisType = skip_typerefs(obj->variant.variable.ptr->type);
          a_routine_ptr r = first->kind == enk_routine ? first->variant.routine.ptr : nullptr;
          std::set<a_type_ptr> found;
          if (thisType != nullptr && thisType->kind == tk_pointer && r != nullptr && r->source_corresp.name != nullptr) {
            classesDeclaring(thisType->variant.pointer.type, r->source_corresp.name, found);
          }
          if (found.size() <= 1) {
            function = member;
            args = objectNode->next;
            break;
          }
        }
      }
      SgExpression* object = convertExpression(objectNode);
      function = k == eok_dot_member_call ? binaryOp<SgDotExp>(object, member)
                                          : binaryOp<SgArrowExp>(object, member);
      setExpressionPosition(function, objectNode);
      args = objectNode->next;
      break;
    }
    case eok_dot_pm_call:
    case eok_points_to_pm_call: {
      an_expr_node_ptr objectNode = first->next;
      SgExpression* pm = convertExpression(first);
      SgExpression* object = convertExpression(objectNode);
      function = k == eok_dot_pm_call ? binaryOp<SgDotStarOp>(object, pm) : binaryOp<SgArrowStarOp>(object, pm);
      function->set_need_paren(true);
      setExpressionPosition(function, objectNode);
      args = objectNode->next;
      break;
    }
    default:
      throw Unsupported("call kind");
  }
  SgExprListExp* list = convertArgumentList(args);
  SgFunctionCallExp* call = SageBuilder::buildFunctionCallExp_nfi(function, list);
  function->set_parent(call);
  list->set_parent(call);
  if (expr->variant.operation.call_uses_operator_syntax) call->set_uses_operator_syntax(true);
  if (first != nullptr && first->kind == enk_routine && first->variant.routine.ptr != nullptr &&
      first->variant.routine.ptr->special_kind == sfk_udl_operator) {
    call->set_uses_operator_syntax(false);  // a user-defined literal (see setSpecialFunctionKind())
  }
  return call;
}

SgExpression* Translator::convertOperation(an_expr_node_ptr expr) {
  an_expr_operator_kind k = expr->variant.operation.kind;
  an_expr_node_ptr a = expr->variant.operation.operands;
  an_expr_node_ptr b = a != nullptr ? a->next : nullptr;
  an_expr_node_ptr c = b != nullptr ? b->next : nullptr;
  SgExpression* r = nullptr;

  auto transparent = [&]() -> SgExpression* {
    SgExpression* e = convertExpression(a);
    if (expr->is_parenthesized) e->set_need_paren(true);
    return e;
  };
  auto A = [&]() { return convertExpression(a); };
  auto B = [&]() { return convertExpression(b); };

  switch (k) {
    // Conversions without a Sage representation
    case eok_ref_indirect:
    case eok_reference_to:
    case eok_lvalue_adjust:
    case eok_class_rvalue_adjust:
    case eok_array_to_pointer:
    case eok_parens:
    case eok_lvalue:
      return transparent();

    case eok_address_of:
      if (expr->compiler_generated) return transparent();
      r = unaryOp<SgAddressOfOp>(A());
      break;
    case eok_indirect:
      if (expr->compiler_generated) return transparent();
      r = unaryOp<SgPointerDerefExp>(A());
      break;

    case eok_cast:
    case eok_lvalue_cast:
    case eok_ref_cast:
    case eok_base_class_cast:
    case eok_derived_class_cast:
    case eok_pm_base_class_cast:
    case eok_pm_derived_class_cast:
    case eok_dynamic_cast:
    case eok_ref_dynamic_cast:
    case eok_bool_cast: {
      bool implicit = expr->compiler_generated || expr->variant.operation.implicit_in_member_naming ||
                      expr->variant.operation.implicit_step_of_explicit_cast;
      if (implicit && expr->type == a->type) return transparent();
      r = convertCast(expr, A(), implicit);
      if (implicit) {
        if (expr->is_parenthesized) r->set_need_paren(true);
        return r;
      }
      break;
    }

    case eok_noexcept:
      r = SageBuilder::buildNoexceptOp_nfi(A());
      break;
    case eok_negate:
      r = unaryOp<SgMinusOp>(A());
      break;
    case eok_unary_plus:
      r = unaryOp<SgUnaryAddOp>(A());
      break;
    case eok_complement:
      r = unaryOp<SgBitComplementOp>(A());
      break;
    case eok_not:
    case eok_vector_not:
      r = unaryOp<SgNotOp>(A());
      break;
    case eok_vector_fill:
      // A scalar operand of a GNU vector operation ("v * 2"), implicitly
      // converted to a vector with the value in every element.
      r = A();
      break;
#if C99_IL_EXTENSIONS_SUPPORTED
    case eok_xconj:
      r = unaryOp<SgConjugateOp>(A());
      break;
    case eok_real_part:
      r = unaryOp<SgRealPartOp>(A());
      break;
    case eok_imag_part:
      r = unaryOp<SgImagPartOp>(A());
      break;
#endif
    case eok_post_incr:
      r = SageBuilder::buildPlusPlusOp_nfi(A(), SgUnaryOp::postfix);
      break;
    case eok_post_decr:
      r = SageBuilder::buildMinusMinusOp_nfi(A(), SgUnaryOp::postfix);
      break;
    case eok_pre_incr:
      r = SageBuilder::buildPlusPlusOp_nfi(A(), SgUnaryOp::prefix);
      break;
    case eok_pre_decr:
      r = SageBuilder::buildMinusMinusOp_nfi(A(), SgUnaryOp::prefix);
      break;

    case eok_add:
    case eok_padd:
#if C99_IL_EXTENSIONS_SUPPORTED
    case eok_fjadd:
    case eok_jfadd:
#endif
      r = binaryOp<SgAddOp>(A(), B());
      break;
    case eok_subtract:
    case eok_psubtract:
    case eok_pdiff:
#if C99_IL_EXTENSIONS_SUPPORTED
    case eok_fjsubtract:
    case eok_jfsubtract:
#endif
      r = binaryOp<SgSubtractOp>(A(), B());
      break;
    case eok_multiply:
#if C99_IL_EXTENSIONS_SUPPORTED
    case eok_jmultiply:
#endif
      r = binaryOp<SgMultiplyOp>(A(), B());
      break;
    case eok_divide:
#if C99_IL_EXTENSIONS_SUPPORTED
    case eok_jdivide:
#endif
      r = binaryOp<SgDivideOp>(A(), B());
      break;
    case eok_remainder:
      r = binaryOp<SgModOp>(A(), B());
      break;
    case eok_shiftl:
      r = binaryOp<SgLshiftOp>(A(), B());
      break;
    case eok_shiftr:
      r = binaryOp<SgRshiftOp>(A(), B());
      break;
    case eok_and:
      r = binaryOp<SgBitAndOp>(A(), B());
      break;
    case eok_or:
      r = binaryOp<SgBitOrOp>(A(), B());
      break;
    case eok_xor:
      r = binaryOp<SgBitXorOp>(A(), B());
      break;
    case eok_eq:
    case eok_vector_eq:
      r = binaryOp<SgEqualityOp>(A(), B());
      break;
    case eok_ne:
    case eok_vector_ne:
      r = binaryOp<SgNotEqualOp>(A(), B());
      break;
    case eok_gt:
    case eok_vector_gt:
      r = binaryOp<SgGreaterThanOp>(A(), B());
      break;
    case eok_lt:
    case eok_vector_lt:
      r = binaryOp<SgLessThanOp>(A(), B());
      break;
    case eok_ge:
    case eok_vector_ge:
      r = binaryOp<SgGreaterOrEqualOp>(A(), B());
      break;
    case eok_le:
    case eok_vector_le:
      r = binaryOp<SgLessOrEqualOp>(A(), B());
      break;
    case eok_spaceship:
      r = binaryOp<SgSpaceshipOp>(A(), B());
      break;
    case eok_land:
    case eok_vector_land:
      r = binaryOp<SgAndOp>(A(), B());
      break;
    case eok_lor:
    case eok_vector_lor:
      r = binaryOp<SgOrOp>(A(), B());
      break;
    case eok_comma:
      r = binaryOp<SgCommaOpExp>(A(), B());
      break;

    case eok_assign:
    case eok_bassign:
      r = binaryOp<SgAssignOp>(A(), B());
      break;
    case eok_add_assign:
    case eok_padd_assign:
      r = binaryOp<SgPlusAssignOp>(A(), B());
      break;
    case eok_subtract_assign:
    case eok_psubtract_assign:
      r = binaryOp<SgMinusAssignOp>(A(), B());
      break;
    case eok_multiply_assign:
      r = binaryOp<SgMultAssignOp>(A(), B());
      break;
    case eok_divide_assign:
      r = binaryOp<SgDivAssignOp>(A(), B());
      break;
    case eok_remainder_assign:
      r = binaryOp<SgModAssignOp>(A(), B());
      break;
    case eok_shiftl_assign:
      r = binaryOp<SgLshiftAssignOp>(A(), B());
      break;
    case eok_shiftr_assign:
      r = binaryOp<SgRshiftAssignOp>(A(), B());
      break;
    case eok_and_assign:
      r = binaryOp<SgAndAssignOp>(A(), B());
      break;
    case eok_or_assign:
      r = binaryOp<SgIorAssignOp>(A(), B());
      break;
    case eok_xor_assign:
      r = binaryOp<SgXorAssignOp>(A(), B());
      break;

    case eok_subscript:
    case eok_vector_subscript:
      r = binaryOp<SgPntrArrRefExp>(A(), B());
      break;

    case eok_dot_field:
    case eok_dot_static:
      r = convertFieldSelection(expr, false);
      break;
    case eok_points_to_field:
    case eok_points_to_static:
      r = convertFieldSelection(expr, true);
      break;
    case eok_pm_field:
    case eok_dot_pm_func_ptr:
      r = binaryOp<SgDotStarOp>(A(), B());
      break;
    case eok_pm_points_to_field:
    case eok_points_to_pm_func_ptr:
      r = binaryOp<SgArrowStarOp>(A(), B());
      break;

    case eok_question:
    case eok_vector_question: {
      SgExpression* cond = A();
      SgExpression* t = nullptr;
#if GNU_EXTENSIONS_ALLOWED
      if (expr->variant.operation.is_gnu_two_operand_question_mark &&
          (b == nullptr || b->kind == enk_reuse_value)) {
        t = SageInterface::deepCopy(cond);
      }
#endif
      if (t == nullptr) t = B();
      SgExpression* f = convertExpression(c);
      r = SageBuilder::buildConditionalExp_nfi(cond, t, f, convertType(expr->type));
      cond->set_parent(r);
      t->set_parent(r);
      f->set_parent(r);
      break;
    }

    case eok_call:
    case eok_dot_member_call:
    case eok_points_to_member_call:
    case eok_dot_pm_call:
    case eok_points_to_pm_call:
      r = convertCall(expr);
      break;

    case eok_va_start: {
      SgExpression* l = A();
      SgExpression* rr = B();
      r = new SgVarArgStartOp(l, rr, convertType(expr->type));
      l->set_parent(r);
      rr->set_parent(r);
      break;
    }
    case eok_va_start_single_operand: {
      SgExpression* l = A();
      r = new SgVarArgStartOneOperandOp(l, convertType(expr->type));
      l->set_parent(r);
      break;
    }
    case eok_va_arg: {
      SgExpression* l = A();
      r = SageBuilder::buildVarArgOp_nfi(l, convertType(expr->type));
      l->set_parent(r);
      break;
    }
    case eok_va_end: {
      SgExpression* l = A();
      r = new SgVarArgEndOp(l, convertType(expr->type));
      l->set_parent(r);
      break;
    }
    case eok_va_copy: {
      SgExpression* l = A();
      SgExpression* rr = B();
      r = new SgVarArgCopyOp(l, rr, convertType(expr->type));
      l->set_parent(r);
      rr->set_parent(r);
      break;
    }

    case eok_dot_vacuous_destructor_call:
    case eok_points_to_vacuous_destructor_call: {
      // "p->~T()" for a type without a destructor to call (ROSE prints the
      // parentheses as part of the pseudo-destructor reference)
      SgExpression* object = A();
      a_type_ptr ot = a->type;
      if (k == eok_points_to_vacuous_destructor_call && skip_typerefs(ot)->kind == tk_pointer) {
        ot = skip_typerefs(ot)->variant.pointer.type;
      }
      SgType* t = convertType(ot)->stripType(SgType::STRIP_MODIFIER_TYPE);
      if (isSgNamedType(t) == nullptr) {
        // "i.int::~int()": ROSE represents only pseudo-destructors of named
        // types; the call has no effect, like "(void)i".
        r = SageBuilder::buildCastExp_nfi(object, SgTypeVoid::createType(), SgCastExp::e_C_style_cast);
        object->set_parent(r);
        break;
      }
      SgPseudoDestructorRefExp* pd = new SgPseudoDestructorRefExp((Sg_File_Info*)nullptr, t);
      setPosition(pd, expr->position);
      r = k == eok_dot_vacuous_destructor_call ? binaryOp<SgDotExp>(object, pd) : binaryOp<SgArrowExp>(object, pd);
      break;
    }
    default:
      throw Unsupported("operator kind " + std::to_string((int)k));
  }
  setExpressionPosition(r, expr);
  return r;
}

SgExpression* Translator::convertSizeof(an_expr_node_ptr expr) {
  bool isType = expr->variant.sizeof_info.is_type;
  SgExpression* r = nullptr;
  if (expr->kind == enk_sizeof) {
    if (isType) {
      SgDeclarationStatement* def = typeDefinitionInExpression(expr->variant.sizeof_info.variant.type);
      SgSizeOfOp* so = SageBuilder::buildSizeOfOp_nfi(convertType(expr->variant.sizeof_info.variant.type));
      if (def != nullptr) {
        so->set_sizeOfContainsBaseTypeDefiningDeclaration(true);
        def->set_parent(so);
      }
      r = so;
    } else {
      r = SageBuilder::buildSizeOfOp_nfi(convertExpression(expr->variant.sizeof_info.variant.expr));
    }
  } else {
    if (isType) {
      r = SageBuilder::buildAlignOfOp_nfi(convertType(expr->variant.sizeof_info.variant.type));
    } else {
      r = SageBuilder::buildAlignOfOp_nfi(convertExpression(expr->variant.sizeof_info.variant.expr));
    }
  }
  return r;
}

SgExpression* Translator::convertStatementExpression(an_expr_node_ptr expr) {
  a_statement_ptr st = expr->variant.statement;
  SgBasicBlock* block = convertBlock(st);
  SgStatementExpression* se = SageBuilder::buildStatementExpression_nfi(block);
  block->set_parent(se);
  return se;
}

SgExpression* Translator::convertTempInit(an_expr_node_ptr expr) {
  a_dynamic_init_ptr dip = expr->variant.init.dynamic_init;
  SgType* type = convertType(expr->type);
  if (dip != nullptr && dip->is_compound_literal) {
    SgInitializer* init = convertDynamicInit(dip, type);
    SgAggregateInitializer* ai = isSgAggregateInitializer(init);
    if (ai == nullptr) {
      SgExprListExp* list = SageBuilder::buildExprListExp_nfi();
      if (init != nullptr) {
        list->append_expression(init);
        init->set_parent(list);
      }
      setCompilerGenerated(list);
      ai = SageBuilder::buildAggregateInitializer_nfi(list, type);
      setCompilerGenerated(ai);
    }
    ai->set_uses_compound_literal(true);
    ai->set_need_explicit_braces(true);
    SgName name = "__compound_literal_" + std::to_string(++compoundLiterals);
    SgInitializedName* iname = SageBuilder::buildInitializedName_nfi(name, type, ai);
    ai->set_parent(iname);
    iname->set_scope(currentScope());
    setCompilerGenerated(iname);
    SgVariableSymbol* sym = new SgVariableSymbol(iname);
    return SageBuilder::buildCompoundLiteralExp_nfi(sym);
  }
  SgInitializer* init = convertDynamicInit(dip, type);
  a_type_ptr tt = skip_typerefs(expr->type);
  bool transparentUnion = false;
#if GNU_EXTENSIONS_ALLOWED
  transparentUnion = tt->kind == tk_union && tt->variant.class_struct_union.is_transparent;
#endif
  if ((expr->compiler_generated || transparentUnion) && isSgAggregateInitializer(init) &&
      isSgAggregateInitializer(init)->get_initializers()->get_expressions().size() == 1) {
    // An implicit conversion to a GNU transparent union: just the argument
    SgExpressionPtrList& elements = isSgAggregateInitializer(init)->get_initializers()->get_expressions();
    SgExpression* e = elements[0];
    if (SgDesignatedInitializer* di = isSgDesignatedInitializer(e)) {
      e = di->get_memberInit();
      di->set_memberInit(nullptr);
    } else {
      elements.clear();
    }
    if (SgAssignInitializer* ai = isSgAssignInitializer(e)) e = initializerExpression(ai);
    e->set_parent(nullptr);
    return e;
  }
  if (isEmptyAggregate(init) && skip_typerefs(expr->type) != nullptr &&
      (skip_typerefs(expr->type)->kind == tk_class || skip_typerefs(expr->type)->kind == tk_struct ||
       skip_typerefs(expr->type)->kind == tk_union) && isCxx) {
    // "T()" of a class without a user-provided constructor
    return valueInitializedTemporary(type);
  }
  if (init == nullptr) {
    // Value-initialized temporary, e.g. "T()"
    SgConstructorInitializer* ci = valueInitializedTemporary(type);
    setCompilerGenerated(ci->get_args());
    return ci;
  }
  if (SgConstructorInitializer* ci = isSgConstructorInitializer(init)) {
    ci->set_need_name(true);
    ci->set_need_parenthesis_after_name(true);
    return ci;
  }
  if (isSgAssignInitializer(init)) {
    // A temporary initialized by an expression (e.g. a functional-notation
    // cast to a class type with a converting constructor): just the expression.
    return initializerExpression(init);
  }
  return init;
}

// A new-type-id cannot contain parentheses ("new (float (*)())"), but ROSE's
// unparser parenthesizes the type only when the new-expression has an initializer.
static bool newTypeNeedsParentheses(SgType* t) {
  bool indirect = false;
  for (;;) {
    t = t->stripType(SgType::STRIP_MODIFIER_TYPE);
    SgPointerType* p = isSgPointerType(t);  // includes pointers to members
    if (p == nullptr) break;
    t = p->get_base_type();
    indirect = true;
  }
  return indirect && (isSgFunctionType(t) != nullptr || isSgArrayType(t) != nullptr);
}

SgExpression* Translator::convertNewDelete(an_expr_node_ptr expr) {
  a_new_delete_supplement_ptr nd = expr->variant.new_delete;
  if (nd->is_new) {
    SgType* type = nullptr;
    a_type_ptr nt = skip_typerefs(nd->type);
    if (nd->number_of_elements != nullptr && nt != nullptr && nt->kind == tk_array) {
      // "new T[n]": the type allocated is an array whose bound is the
      // run-time number of elements.
      SgExpression* n = convertExpression(nd->number_of_elements);
      SgArrayType* at = SageBuilder::buildArrayType(convertType(nt->variant.array.element_type), n);
      n->set_parent(at);
      type = at;
    } else {
      type = convertType(nd->type);
    }
    SgExprListExp* placement = nullptr;
    if (nd->placement_new && nd->arg != nullptr) {
      // The arguments of the allocation function after the (implicit) size and
      // alignment are the placement arguments.
      an_expr_node_ptr first = nd->arg;
      if (nd->aligned_version && first != nullptr) first = first->next;
      placement = convertArgumentList(first);
    }
    SgConstructorInitializer* ctor = nullptr;
    if (nd->dynamic_init != nullptr) {
      SgInitializer* init = convertDynamicInit(nd->dynamic_init, type);
      ctor = isSgConstructorInitializer(init);
      if (ctor == nullptr && init != nullptr) {
        SgExprListExp* args = SageBuilder::buildExprListExp_nfi();
        SgExpression* e = initializerExpression(init);
        args->append_expression(e);
        e->set_parent(args);
        setCompilerGenerated(args);
        ctor = SageBuilder::buildConstructorInitializer_nfi(nullptr, args, type, false, false, true, true);
        setCompilerGenerated(ctor);
      }
    } else if (nd->has_new_initializer) {
      ctor = SageBuilder::buildConstructorInitializer_nfi(nullptr, SageBuilder::buildExprListExp_nfi(), type,
                                                         false, false, true, true);
      setCompilerGenerated(ctor->get_args());
      setCompilerGenerated(ctor);
    }
    if (ctor == nullptr && newTypeNeedsParentheses(type)) {
      // An empty initializer prints nothing, but the type is parenthesized.
      ctor = SageBuilder::buildConstructorInitializer_nfi(nullptr, SageBuilder::buildExprListExp_nfi(), type,
                                                         false, false, false, true);
      setCompilerGenerated(ctor->get_args());
      setCompilerGenerated(ctor);
    }
    if (ctor != nullptr) ctor->set_need_name(false);
    SgExpression* arraySize = nullptr;
    SgFunctionDeclaration* op = nd->routine != nullptr ? functionDeclarationFor(nd->routine) : nullptr;
    SgNewExp* n = new SgNewExp(type, placement, ctor, arraySize, nd->global_new_or_delete ? 1 : 0, op);
    if (placement) placement->set_parent(n);
    if (ctor) ctor->set_parent(n);
    if (arraySize) arraySize->set_parent(n);
    return n;
  }
  SgExpression* target = nullptr;
  if (nd->arg != nullptr) target = convertExpression(nd->arg);
  if (target == nullptr) throw Unsupported("delete without operand");
  SgFunctionDeclaration* op = nd->routine != nullptr ? functionDeclarationFor(nd->routine) : nullptr;
  SgDeleteExp* d = SageBuilder::buildDeleteExp_nfi(target, nd->array_delete, nd->global_new_or_delete, op);
  target->set_parent(d);
  return d;
}

// ---------------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------------

SgExpression* Translator::convertConstant(a_constant_ptr con, an_expr_node_ptr node) {
  if (con == nullptr) throw Unsupported("missing constant");

  // Enumerators
  auto en = enumerators.find(con);
  if (en != enumerators.end()) {
    SgInitializedName* iname = en->second;
    SgEnumDeclaration* ed = isSgEnumDeclaration(iname->get_parent());
    a_boolean ovf = FALSE;
    long long v = int_constant_is_signed(con) ? (long long)value_of_integer_constant(con, &ovf)
                                              : (long long)unsigned_value_of_integer_constant(con, &ovf);
    SgEnumVal* ev = SageBuilder::buildEnumVal_nfi(v, ed, iname->get_name());
    if (node == nullptr) setCompilerGenerated(ev);
    return ev;
  }

  // A constant that was folded from an expression: translate the expression
  // (ROSE keeps the original expression trees rather than folded values).
  an_expr_node_ptr backing = con->expr;
  if (backing == nullptr && con->local_expr_ref) {
    backing = find_local_expr_node((char*)con, lerk_constant_expr);
  }
  if (backing != nullptr && backing != node && foldedConstants.count(con) == 0) {
    foldedConstants.insert(con);
    try {
      SgExpression* e = convertExpression(backing);
      foldedConstants.erase(con);
      return e;
    } catch (const Unsupported&) {
      foldedConstants.erase(con);
      // fall back to the folded value
    }
  }

  SgExpression* r = nullptr;
  switch (con->kind) {
    case ck_integer:
      r = convertIntegerConstant(con, node);
      break;
    case ck_float:
      r = convertFloatConstant(con, node);
      break;
    case ck_string:
      r = convertStringConstant(con, node);
      break;
    case ck_address:
      r = convertAddressConstant(con, node);
      break;
#if C99_IL_EXTENSIONS_SUPPORTED
    case ck_complex: {
      a_type_ptr t = skip_typerefs(con->type);
      a_float_kind fk = t->variant.float_kind;
      an_internal_complex_value_ptr cv = con->variant.complex_value;
      auto part = [&](an_internal_float_value* v) -> SgValueExp* {
        a_boolean pinf = FALSE, ninf = FALSE, nan = FALSE;
        a_number_buffer buf = fp_to_string(fk, v, &pinf, &ninf, &nan);
        std::string str = buf.as_temp_characters();
        if (str.find_first_of(".eEnN") == std::string::npos) str += ".0";
        SgValueExp* e = SageBuilder::buildDoubleVal_nfi(std::strtod(str.c_str(), nullptr), str);
        setCompilerGenerated(e);
        return e;
      };
      SgValueExp* re = part(&cv->real);
      SgValueExp* im = part(&cv->imag);
      std::string text = literalSpelling(this, con, node);
      if (!isNumericLiteral(text)) text = "";
      if (isSgDoubleVal(re)->get_value() == 0.0) {
        delete re;
        r = SageBuilder::buildImaginaryVal_nfi(im, text);
      } else {
        r = SageBuilder::buildComplexVal_nfi(re, im, text);
        re->set_parent(r);
      }
      im->set_parent(r);
      break;
    }
    case ck_imaginary: {
      a_type_ptr t = skip_typerefs(con->type);
      a_float_kind fk = t->variant.float_kind;
      a_boolean pinf = FALSE, ninf = FALSE, nan = FALSE;
      a_number_buffer buf = fp_to_string(fk, &con->variant.float_value, &pinf, &ninf, &nan);
      std::string s = buf.as_temp_characters();
      SgValueExp* im = SageBuilder::buildDoubleVal_nfi(std::strtod(s.c_str(), nullptr), s);
      setCompilerGenerated(im);
      r = SageBuilder::buildImaginaryVal_nfi(im, s + "i");
      im->set_parent(r);
      break;
    }
#endif
    case ck_dynamic_init: {
      r = initializerExpression(convertDynamicInit(con->variant.dynamic_init.ptr, convertType(con->type)));
      if (r == nullptr) throw Unsupported("empty dynamic initialization constant");
      return r;
    }
    case ck_aggregate: {
      SgAggregateInitializer* ai = convertAggregate(con, convertType(con->type));
      ai->set_need_explicit_braces(true);
      // A GNU vector value (folded from "v * 2" in C++): a compound literal
      a_type_ptr t = skip_typerefs(con->type);
      if (t != nullptr && t->kind == tk_vector) ai->set_uses_compound_literal(true);
      r = ai;
      break;
    }
    case ck_ptr_to_member: {
      if (con->variant.ptr_to_member.is_function_ptr) {
        a_routine_ptr rout = con->variant.ptr_to_member.variant.routine;
        if (rout == nullptr) throw Unsupported("null pointer to member function");
        r = unaryOp<SgAddressOfOp>(convertRoutineReference(rout, nullptr));
      } else {
        a_field_ptr f = con->variant.ptr_to_member.variant.field;
        if (f == nullptr) throw Unsupported("null pointer to data member");
        r = unaryOp<SgAddressOfOp>(SageBuilder::buildVarRefExp_nfi(fieldSymbolFor(f)));
      }
      break;
    }
    case ck_void:
      r = SageBuilder::buildVoidVal_nfi();
      break;
    default:
      throw Unsupported("constant kind " + std::to_string((int)con->kind));
  }
  if (r->get_startOfConstruct() == nullptr) {
    if (node != nullptr) {
      setExpressionPosition(r, node);
    } else if (con->source_corresp.decl_position.seq != 0) {
      setPosition(r, con->source_corresp.decl_position,
                  con->end_position.seq != 0 ? con->end_position : con->source_corresp.decl_position);
    } else {
      setCompilerGenerated(r);
    }
  }
  return r;
}

// Source spelling of a literal constant, or "" if not available.
static std::string literalSpelling(Translator* t, a_constant_ptr con, an_expr_node_ptr node) {
  a_source_position start, end;
  if (node != nullptr) {
    rangeOf(node, start, end);
  } else {
    start = con->source_corresp.decl_position;
    end = con->end_position;
  }
  if (start.seq == 0 || end.seq == 0) return "";
  return t->sourceText(start, end);
}

SgExpression* Translator::convertIntegerConstant(a_constant_ptr con, an_expr_node_ptr node) {
  a_type_ptr t = skip_typerefs(con->type);
  std::string text = literalSpelling(this, con, node);
  bool charLiteral = isCharacterLiteral(text);
  if (!isNumericLiteral(text) && !charLiteral) text = "";

  a_boolean ovf = FALSE;
  bool isSigned = int_constant_is_signed(con);
  long long sv = isSigned ? (long long)value_of_integer_constant(con, &ovf)
                          : (long long)unsigned_value_of_integer_constant(con, &ovf);
  ovf = FALSE;
  unsigned long long uv = (unsigned long long)unsigned_value_of_integer_constant(con, &ovf);

  if (t->kind == tk_pointer || t->kind == tk_nullptr) {
    // A null pointer constant (or an integer cast to a pointer, folded)
    if (con->nullptr_keyword) return SageBuilder::buildNullptrValExp_nfi();
#if GNU_EXTENSIONS_ALLOWED
    if (con->null_keyword) {
      SgExpression* e = SageBuilder::buildLongIntVal_nfi(0, "__null");
      return e;
    }
#endif
    SgExpression* v = SageBuilder::buildIntVal_nfi((int)sv, text);
    if (sv != 0 || con->explicit_cast_applied) {
      setCompilerGenerated(v);
      SgCastExp* c = SageBuilder::buildCastExp_nfi(v, convertType(con->type), SgCastExp::e_C_style_cast);
      v->set_parent(c);
      return c;
    }
    return v;
  }
  if (t->kind != tk_integer) {
    return SageBuilder::buildLongLongIntVal_nfi(sv, text);
  }
  if (t->variant.integer.enum_type) {
    // A value of an enumeration type (e.g. a folded enumerator reference):
    // the enumerator with that value, if there is one.
    a_constant_ptr list = t->variant.integer.is_scoped_enum
                              ? (t->variant.integer.enum_info.assoc_scope ? t->variant.integer.enum_info.assoc_scope->constants
                                                                          : nullptr)
                              : t->variant.integer.enum_info.constant_list;
    for (a_constant_ptr e = list; e != nullptr; e = e->next) {
      if (e->kind != ck_integer) continue;
      a_boolean ovf2 = FALSE;
      long long ev = int_constant_is_signed(e) ? (long long)value_of_integer_constant(e, &ovf2)
                                               : (long long)unsigned_value_of_integer_constant(e, &ovf2);
      if (ev != sv) continue;
      SgInitializedName* en = enumeratorFor(e, t);
      if (en == nullptr) break;
      SgEnumDeclaration* ed = isSgEnumDeclaration(en->get_parent());
      return SageBuilder::buildEnumVal_nfi(sv, ed, en->get_name());
    }
    // Otherwise a cast of the value.
    SgExpression* v = SageBuilder::buildIntVal_nfi((int)sv, text);
    setCompilerGenerated(v);
    SgCastExp* c = SageBuilder::buildCastExp_nfi(v, convertType(con->type), SgCastExp::e_C_style_cast);
    v->set_parent(c);
    return c;
  }
  if (t->variant.integer.bool_type) {
    return SageBuilder::buildBoolValExp_nfi(uv != 0);
  }
  if (t->variant.integer.wchar_t_type) return SageBuilder::buildWcharVal_nfi((wchar_t)uv, text);
  if (t->variant.integer.char16_t_type) return SageBuilder::buildChar16Val_nfi((unsigned short)uv, text);
  if (t->variant.integer.char32_t_type) return SageBuilder::buildChar32Val_nfi((unsigned int)uv, text);
  if (t->variant.integer.char8_t_type) return SageBuilder::buildUnsignedCharVal_nfi((unsigned char)uv, text);
  switch (t->variant.integer.int_kind) {
    case ik_char:
      return SageBuilder::buildCharVal_nfi((char)sv, text);
    case ik_signed_char:
      return SageBuilder::buildSignedCharVal_nfi((signed char)sv, text);
    case ik_unsigned_char:
      return SageBuilder::buildUnsignedCharVal_nfi((unsigned char)uv, text);
    case ik_short:
      return SageBuilder::buildShortVal_nfi((short)sv, text);
    case ik_unsigned_short:
      return SageBuilder::buildUnsignedShortVal_nfi((unsigned short)uv, text);
    case ik_int:
      return SageBuilder::buildIntVal_nfi((int)sv, text);
    case ik_unsigned_int:
      return SageBuilder::buildUnsignedIntVal_nfi((unsigned int)uv, text);
    case ik_long:
      return SageBuilder::buildLongIntVal_nfi((long)sv, text);
    case ik_unsigned_long:
      return SageBuilder::buildUnsignedLongVal_nfi((unsigned long)uv, text);
    case ik_long_long:
    case ik_int128:
      return SageBuilder::buildLongLongIntVal_nfi(sv, text);
    case ik_unsigned_long_long:
    case ik_unsigned_int128:
      return SageBuilder::buildUnsignedLongLongIntVal_nfi(uv, text);
    default:
      return SageBuilder::buildLongLongIntVal_nfi(sv, text);
  }
}

SgExpression* Translator::convertFloatConstant(a_constant_ptr con, an_expr_node_ptr node) {
  a_type_ptr t = skip_typerefs(con->type);
  a_float_kind fk = t->variant.float_kind;
  a_boolean pinf = FALSE, ninf = FALSE, nan = FALSE;
  a_number_buffer buf = fp_to_string(fk, &con->variant.float_value, &pinf, &ninf, &nan);
  std::string value = buf.as_temp_characters();
  long double v = std::strtold(value.c_str(), nullptr);
  std::string text = literalSpelling(this, con, node);
  if (!isNumericLiteral(text)) {
    // Construct a literal of the right type
    if (pinf || ninf || nan) {
      text = nan ? "__builtin_nan(\"\")" : (ninf ? "(-__builtin_inf())" : "__builtin_inf()");
      if (fk == fk_float) text = nan ? "__builtin_nanf(\"\")" : (ninf ? "(-__builtin_inff())" : "__builtin_inff()");
      if (fk == fk_long_double) text = nan ? "__builtin_nanl(\"\")" : (ninf ? "(-__builtin_infl())" : "__builtin_infl()");
    } else {
      text = value;
      if (text.find_first_of(".eEnN") == std::string::npos) text += ".0";
      switch (fk) {
        case fk_float: text += "F"; break;
        case fk_long_double: text += "L"; break;
        case fk_float128: text += "Q"; break;
        case fk_float80: text += "W"; break;
        default: break;
      }
    }
  }
  switch (fk) {
    case fk_float:
    case fk_float32x:
      return SageBuilder::buildFloatVal_nfi((float)v, text);
    case fk_double:
    case fk_float64x:
      return SageBuilder::buildDoubleVal_nfi((double)v, text);
    case fk_long_double:
      return SageBuilder::buildLongDoubleVal_nfi(v, text);
    case fk_float80:
      return SageBuilder::buildFloat80Val_nfi(v, text);
    case fk_float128:
    case fk_std_float128:
      return SageBuilder::buildFloat128Val_nfi(v, text);
    case fk_float16:
    case fk_fp16:
    case fk_std_float16:
      return SageBuilder::buildFloat16Val_nfi((float)v, text);
    case fk_std_bfloat16:
      return SageBuilder::buildBFloat16Val_nfi((float)v, text);
    case fk_std_float32:
      return SageBuilder::buildFloat32Val_nfi((float)v, text);
    case fk_std_float64:
      return SageBuilder::buildFloat64Val_nfi((double)v, text);
    default:
      return SageBuilder::buildDoubleVal_nfi((double)v, text);
  }
}

SgExpression* Translator::convertStringConstant(a_constant_ptr con, an_expr_node_ptr node) {
  a_type_ptr at = skip_typerefs(con->type);
  size_t elemSize = 1;
  if (at->kind == tk_array) {
    a_type_ptr et = skip_typerefs(at->variant.array.element_type);
    if (et->size > 0) elemSize = (size_t)et->size;
  }
  std::string value;
  std::string text = literalSpelling(this, con, node);
  bool fromSource = false;
  if (!text.empty() && text.find('R') == std::string::npos) {
    // Use the source spelling (keeps escapes and concatenation as written) when
    // the literal's characters are on one line.
    fromSource = stringLiteralBody(text, value);
  }
  if (!fromSource) {
    size_t length = (size_t)con->variant.string.length;
    const unsigned char* bytes = (const unsigned char*)con->variant.string.value;
    size_t n = length / elemSize;
    // Drop the terminating null character (if any)
    if (n > 0) {
      unsigned long last = 0;
      for (size_t b = 0; b < elemSize; b++) last |= (unsigned long)bytes[(n - 1) * elemSize + b] << (8 * b);
      if (last == 0) n--;
    }
    for (size_t i = 0; i < n; i++) {
      unsigned long ch = 0;
      for (size_t b = 0; b < elemSize; b++) ch |= (unsigned long)bytes[i * elemSize + b] << (8 * b);
      std::string esc = escapeCharacter(ch, '"');
      // Keep "\nnn" followed by a digit unambiguous, and avoid trigraphs.
      if (!value.empty() && value.back() == '?' && ch == '?') esc = "\\?";
      value += esc;
    }
  }
  SgStringVal* sv = SageBuilder::buildStringVal_nfi(value);
  switch ((a_character_kind)con->character_kind) {
    case chk_wchar_t:
      sv->set_wcharString(true);
      break;
    case chk_char16_t:
      sv->set_is16bitString(true);
      break;
    case chk_char32_t:
      sv->set_is32bitString(true);
      break;
    default:
      break;
  }
  return sv;
}

SgExpression* Translator::convertAddressConstant(a_constant_ptr con, an_expr_node_ptr node) {
  SgExpression* base = nullptr;
  bool needAddressOf = true;
  switch (con->variant.address.kind) {
    case abk_routine:
      base = convertRoutineReference(con->variant.address.variant.routine, nullptr);
      needAddressOf = false;  // function designators decay to pointers
      break;
    case abk_variable: {
      a_variable_ptr v = con->variant.address.variant.variable;
      base = convertVariableReference(v, nullptr);
      if (skip_typerefs(v->type)->kind == tk_array) needAddressOf = false;
      break;
    }
    case abk_constant: {
      a_constant_ptr c = con->variant.address.variant.constant;
      base = convertConstant(c, nullptr);
      if (c->kind == ck_string) needAddressOf = false;
      break;
    }
    case abk_label: {
      SgLabelStatement* ls = labelStatementFor(con->variant.address.variant.label);
      SgLabelSymbol* sym = labelSymbols.count(con->variant.address.variant.label)
                               ? labelSymbols[con->variant.address.variant.label]
                               : nullptr;
      if (sym == nullptr) {
        sym = new SgLabelSymbol(ls);
        labelSymbols[con->variant.address.variant.label] = sym;
      }
      base = new SgLabelRefExp(sym);
      needAddressOf = false;
      break;
    }
    default:
      throw Unsupported("address constant kind");
  }
  if (base->get_startOfConstruct() == nullptr) setCompilerGenerated(base);
  // The value of a reference (e.g. a template argument for a reference
  // parameter) is the address of the object referred to, written as the object.
  a_type_ptr ct = skip_typerefs(con->type);
  if (ct != nullptr && ct->kind == tk_pointer && (ct->variant.pointer.is_reference || ct->variant.pointer.is_rvalue_reference) &&
      con->variant.address.offset == 0) {
    needAddressOf = false;
  }
  SgExpression* r = base;
  if (needAddressOf) {
    r = unaryOp<SgAddressOfOp>(base);
    setCompilerGenerated(r);
  }
  if (con->variant.address.offset != 0) {
    // address + byte offset
    SgType* charPtr = SageBuilder::buildPointerType(SgTypeChar::createType());
    SgCastExp* c = SageBuilder::buildCastExp_nfi(r, charPtr, SgCastExp::e_C_style_cast);
    setCompilerGenerated(c);
    SgExpression* off = SageBuilder::buildLongIntVal_nfi((long)con->variant.address.offset, "");
    setCompilerGenerated(off);
    SgExpression* sum = binaryOp<SgAddOp>(c, off);
    setCompilerGenerated(sum);
    r = SageBuilder::buildCastExp_nfi(sum, convertType(con->type), SgCastExp::e_C_style_cast);
  }
  return r;
}

// ---------------------------------------------------------------------------------
// Initializers
// ---------------------------------------------------------------------------------

namespace {
SgAssignInitializer* assignInitializer(Translator* t, SgExpression* e, SgType* type) {
  SgAssignInitializer* ai = SageBuilder::buildAssignInitializer_nfi(e, type ? type : e->get_type());
  e->set_parent(ai);
  // The initializer is written where its expression is (implicit conversions
  // of the expression are compiler generated, the initializer is not).
  if (!t->copyPosition(ai, e)) t->setCompilerGenerated(ai, true);
  return ai;
}
}  // namespace

// The expression an initializer stands for: the operand of an assignment
// initializer (detached from it, so that the abandoned initializer does not
// refer to a node of the AST), or the initializer itself.
SgExpression* Translator::initializerExpression(SgInitializer* init) {
  if (SgAssignInitializer* ai = isSgAssignInitializer(init)) {
    SgExpression* e = ai->get_operand();
    ai->set_operand(nullptr);
    if (e != nullptr) e->set_parent(nullptr);
    return e;
  }
  return init;
}

// Gives `node` the source position of `e`, looking through implicit
// (compiler-generated) conversions.  Returns false if no position is known.
bool Translator::copyPosition(SgLocatedNode* node, SgExpression* e) {
  while (e != nullptr && e->get_startOfConstruct() != nullptr && e->get_startOfConstruct()->isCompilerGenerated()) {
    SgCastExp* c = isSgCastExp(e);
    if (c == nullptr) return false;
    e = c->get_operand();
  }
  if (e == nullptr || e->get_startOfConstruct() == nullptr) return false;
  Sg_File_Info* s = new Sg_File_Info(*e->get_startOfConstruct());
  Sg_File_Info* en = new Sg_File_Info(*(e->get_endOfConstruct() ? e->get_endOfConstruct() : e->get_startOfConstruct()));
  delete node->get_startOfConstruct();
  delete node->get_endOfConstruct();
  node->set_startOfConstruct(s);
  node->set_endOfConstruct(en);
  s->set_parent(node);
  en->set_parent(node);
  if (SgExpression* ne = isSgExpression(node)) {
    Sg_File_Info* o = new Sg_File_Info(*e->get_startOfConstruct());
    delete ne->get_operatorPosition();
    ne->set_operatorPosition(o);
    o->set_parent(ne);
  }
  return true;
}

static a_local_static_variable_init_ptr findLocalStaticInit(a_scope_ptr scope, a_variable_ptr var) {
  if (scope == nullptr) return nullptr;
  for (a_local_static_variable_init_ptr li = scope->local_static_variable_inits; li != nullptr; li = li->next) {
    if (li->variable == var) return li;
  }
  for (a_scope_ptr s = scope->scopes; s != nullptr; s = s->next) {
    if (a_local_static_variable_init_ptr li = findLocalStaticInit(s, var)) return li;
  }
  return nullptr;
}

SgInitializer* Translator::convertVariableInitializer(a_variable_ptr var) {
  SgType* type = convertType(var->type);
  switch (var->init_kind) {
    case initk_static:
      return convertInitializerConstant(var->initializer.constant, type);
    case initk_dynamic:
      return convertDynamicInit(var->initializer.dynamic, type);
    case initk_function_local: {
      // Local static variable: the initialization is on a list of the function
      // or block scope (in the function's memory region).
      a_local_static_variable_init_ptr li = findLocalStaticInit(functionScopeOf(currentRoutine), var);
      if (li == nullptr) return nullptr;
      if (li->init_kind == initk_static) return convertInitializerConstant(li->initializer.constant, type);
      if (li->init_kind == initk_dynamic) return convertDynamicInit(li->initializer.dynamic, type);
      return nullptr;
    }
    default:
      return nullptr;
  }
}

SgInitializer* Translator::convertDynamicInit(a_dynamic_init_ptr dip, SgType* type) {
  if (dip == nullptr) return nullptr;
  switch (dip->kind) {
    case dik_none:
    case dik_zero:
      if (dip->is_explicit_cast && type != nullptr) {
        // "T()": a value-initialized temporary (e.g. "return T();")
        SgConstructorInitializer* ci = valueInitializedTemporary(type);
        setCompilerGenerated(ci->get_args());
        return ci;
      }
      return nullptr;
    case dik_constant:
    case dik_nonconstant_aggregate:
      if (dip->variant.constant.lambda != nullptr) {
        return assignInitializer(this, convertLambda(dip->variant.constant.lambda), type);
      }
      if (dip->is_explicit_cast && type != nullptr && dip->variant.constant.ptr != nullptr &&
          dip->variant.constant.ptr->kind == ck_aggregate &&
          dip->variant.constant.ptr->variant.aggregate.first_constant == nullptr) {
        // "T()" for a class without a user-provided constructor
        SgConstructorInitializer* ci = valueInitializedTemporary(type);
        setCompilerGenerated(ci->get_args());
        return ci;
      }
      if (a_constant_ptr c = dip->variant.constant.ptr) {
        // The closure object of a lambda (with init-captures) is an aggregate
        // constant whose backing expression is the lambda.
        an_expr_node_ptr be = c->expr;
        if (be != nullptr && be->kind == enk_initializer && be->variant.initializer.dyn_init != nullptr &&
            be->variant.initializer.dyn_init->kind == dik_lambda) {
          return convertDynamicInit(be->variant.initializer.dyn_init, type);
        }
      }
      return convertInitializerConstant(dip->variant.constant.ptr, type);
    case dik_lambda: {
      // "auto f = [...](...) {...};"
      SgExpression* e = convertLambda(dip->variant.constant.lambda);
      return assignInitializer(this, e, type);
    }
    case dik_expression:
    case dik_class_result_via_ctor: {
      SgExpression* e = convertExpression(dip->variant.expression);
      if (SgInitializer* i = isSgInitializer(e)) {
        if (!isSgAggregateInitializer(i) || dip->is_braced_initializer) return i;
      }
      return assignInitializer(this, e, type);
    }
    case dik_bitwise_copy: {
      SgExpression* e = convertExpression(dip->variant.bitwise_copy.source);
      return assignInitializer(this, e, type);
    }
    case dik_constructor: {
      a_routine_ptr ctor = dip->variant.constructor.ptr;
      SgExprListExp* args = convertArgumentList(dip->variant.constructor.args);
      SgMemberFunctionDeclaration* decl =
          ctor != nullptr ? isSgMemberFunctionDeclaration(functionDeclarationFor(ctor)) : nullptr;
      if (dip->variant.constructor.is_implicit_copy_for_copy_initialization &&
          args->get_expressions().size() == 1) {
        // "T x = expr;" with an implicit copy: just the expression
        SgExpression* e = args->get_expressions()[0];
        args->get_expressions().clear();
        e->set_parent(nullptr);
        return assignInitializer(this, e, type);
      }
      SgType* ctype = type;
      if (ctype == nullptr && dip->variable != nullptr) ctype = convertType(dip->variable->type);
      a_type_ptr cls = nullptr;
      if (ctor != nullptr) {
        a_type_ptr rt = skip_typerefs(ctor->type);
        if (rt->kind == tk_routine && rt->variant.routine.extra_info != nullptr) {
          cls = rt->variant.routine.extra_info->this_class;
        }
      }
      if (ctype == nullptr && cls != nullptr) ctype = convertType(cls);
      if (ctype == nullptr) throw Unsupported("constructor call of unknown type");
      a_type_qualifier_set quals = 0;
      if (type == nullptr && dip->variable == nullptr && ctor != nullptr && cls != nullptr &&
          args->get_expressions().size() == 1 &&
          is_copy_constructor(ctor, cls, &quals, /*include_move_ctors=*/TRUE, /*is_declarative_context=*/FALSE)) {
        // An (elided) copy of a returned or thrown object: just the object
        SgExpression* e = args->get_expressions()[0];
        args->get_expressions().clear();
        e->set_parent(nullptr);
        return assignInitializer(this, e, ctype);
      }
      // In a variable declaration "T x(args)" prints only "(args)"; elsewhere
      // (a returned or thrown object, a temporary) the class name is needed.
      bool needName = (type == nullptr && dip->variable == nullptr);
      SgConstructorInitializer* ci = SageBuilder::buildConstructorInitializer_nfi(
          decl, args, ctype, needName, false, needName || !args->get_expressions().empty(), decl == nullptr);
      args->set_parent(ci);
      setCompilerGenerated(ci);
      return ci;
    }
    default:
      throw Unsupported("dynamic initialization kind " + std::to_string((int)dip->kind));
  }
}

// Whether the default constructor of a class is explicit (then "T x = {};"
// does not compile, "T x = T();" does).
static bool hasExplicitDefaultConstructor(a_type_ptr type) {
  type = skip_typerefs(type);
  if (type == nullptr || (type->kind != tk_class && type->kind != tk_struct)) return false;
  a_class_type_supplement_ptr ctsp = type->variant.class_struct_union.extra_info;
  a_scope_ptr scope = ctsp != nullptr ? ctsp->assoc_scope : nullptr;
  for (a_routine_ptr r = scope ? scope->routines : nullptr; r != nullptr; r = r->next) {
    if (r->special_kind != sfk_constructor || !r->is_explicit_constructor) continue;
    a_type_ptr rt = skip_typerefs(r->type);
    a_routine_type_supplement_ptr rtsp = rt != nullptr && rt->kind == tk_routine ? rt->variant.routine.extra_info : nullptr;
    if (rtsp != nullptr && (rtsp->param_type_list == nullptr || rtsp->param_type_list->has_default_arg)) return true;
  }
  return false;
}

SgInitializer* Translator::convertInitializerConstant(a_constant_ptr con, SgType* type) {
  if (con == nullptr) return nullptr;
  switch (con->kind) {
    case ck_aggregate: {
      if (con->variant.aggregate.first_constant == nullptr && isCxx && hasExplicitDefaultConstructor(con->type)) {
        // "T x = T();" with an explicit default constructor
        SgConstructorInitializer* ci = valueInitializedTemporary(type != nullptr ? type : convertType(con->type));
        ci->set_is_explicit_cast(true);
        setCompilerGenerated(ci->get_args());
        return ci;
      }
      // The closure object of a lambda: the backing expression is the lambda.
      an_expr_node_ptr be = con->expr;
      if (be != nullptr && (be->kind == enk_lambda ||
                            (be->kind == enk_initializer && be->variant.initializer.dyn_init != nullptr &&
                             be->variant.initializer.dyn_init->kind == dik_lambda))) {
        SgExpression* e = be->kind == enk_lambda ? convertLambda(be->variant.init.source.lambda)
                                                 : convertLambda(be->variant.initializer.dyn_init->variant.constant.lambda);
        return assignInitializer(this, e, type);
      }
      if (con->is_result_of_constexpr_call && be != nullptr) {
        // The value of a constexpr constructor call: the call as written.
        SgExpression* e = convertExpression(be);
        if (SgInitializer* i = isSgInitializer(e)) return i;
        return assignInitializer(this, e, type);
      }
      SgAggregateInitializer* ai = convertAggregate(con, type);
      ai->set_need_explicit_braces(true);
      return ai;
    }
    case ck_dynamic_init:
      return convertDynamicInit(con->variant.dynamic_init.ptr, type);
    default: {
      SgExpression* e = convertConstant(con, nullptr);
      if (SgInitializer* i = isSgInitializer(e)) return i;
      return assignInitializer(this, e, type);
    }
  }
}

SgAggregateInitializer* Translator::convertAggregate(a_constant_ptr con, SgType* type) {
  if (type == nullptr) type = convertType(con->type);
  SgExprListExp* list = SageBuilder::buildExprListExp_nfi();
  appendAggregateElements(list, con);
  setCompilerGenerated(list);
  SgAggregateInitializer* ai = SageBuilder::buildAggregateInitializer_nfi(list, type);
  list->set_parent(ai);
  ai->set_need_explicit_braces(con->explicit_braces_on_aggregate);
  if (con->source_corresp.decl_position.seq != 0) {
    setPosition(ai, con->source_corresp.decl_position,
                con->end_position.seq != 0 ? con->end_position : con->source_corresp.decl_position);
  } else {
    setCompilerGenerated(ai);
  }
  return ai;
}

// A constructor call written as an element of an aggregate initializer
// ("{T(1), {2, 3}}") is printed as written, with the class name or braces, and
// is not compiler generated (ROSE stops printing the elements at the first
// compiler-generated constructor call: the initialization of the elements
// without initializer).
void Translator::markWrittenConstructorElement(SgInitializer* init, a_dynamic_init_ptr dip) {
  SgConstructorInitializer* ci = isSgConstructorInitializer(init);
  if (ci == nullptr || dip == nullptr || dip->kind != dik_constructor) return;
  bool hasArgs = !ci->get_args()->get_expressions().empty();
  if (!dip->is_braced_initializer && !dip->is_explicit_cast && !hasArgs) return;
  if (dip->is_braced_initializer && hasArgs) {
    ci->set_is_braced_initialized(true);
  } else if (dip->is_braced_initializer || dip->is_explicit_cast) {
    ci->set_need_name(true);
    ci->set_is_explicit_cast(true);
  }  // (else an implicit conversion of the argument, printed "(arg)")
  if (ci->isCompilerGenerated() && !(hasArgs && copyPosition(ci, ci->get_args()->get_expressions()[0]))) {
    for (Sg_File_Info* fi : {ci->get_startOfConstruct(), ci->get_endOfConstruct(), ci->get_operatorPosition()}) {
      if (fi != nullptr) {
        fi->unsetCompilerGenerated();
        fi->setTransformation();
        fi->setOutputInCodeGeneration();
      }
    }
  }
}

void Translator::appendAggregateElements(SgExprListExp* list, a_constant_ptr con) {
  auto element = [&](a_constant_ptr c) -> SgInitializer* {
    SgType* et = c->type != nullptr ? convertType(c->type) : nullptr;
    if (c->kind == ck_aggregate && !(c->is_result_of_constexpr_call && c->expr != nullptr)) {
      SgAggregateInitializer* sub = convertAggregate(c, et);
      return sub;
    }
    SgInitializer* init = convertInitializerConstant(c, et);
    if (c->kind == ck_dynamic_init) {
      markWrittenConstructorElement(init, c->variant.dynamic_init.ptr);
    } else if (c->kind == ck_aggregate && c->expr != nullptr && c->expr->kind == enk_temp_init) {
      // the value of a constexpr constructor call: the call
      markWrittenConstructorElement(init, c->expr->variant.init.dynamic_init);
    }
    return init;
  };

  for (a_constant_ptr c = con->variant.aggregate.first_constant; c != nullptr; c = c->next) {
    if (c->kind == ck_designator) {
      a_constant_ptr value = c->next;
      if (value == nullptr) break;
      SgExpression* designator = nullptr;
      if (c->variant.designator.is_field_designator && !c->variant.designator.is_generic) {
        a_field_ptr f = c->variant.designator.variant.field;
        if (f == nullptr || f->source_corresp.name == nullptr) {
          // Designator of an anonymous struct/union member (implicit in the
          // source): the members' designators follow in its initializer.
          if (value->kind == ck_aggregate) {
            appendAggregateElements(list, value);
            c = value;
            continue;
          }
          throw Unsupported("designator of an anonymous member");
        }
        designator = SageBuilder::buildVarRefExp_nfi(fieldSymbolFor(f));
      } else if (c->variant.designator.is_generic) {
        if (c->variant.designator.is_field_designator) throw Unsupported("unresolved field designator");
        designator = convertConstant(c->variant.designator.variant.subscript, nullptr);
      } else {
        unsigned long index = (unsigned long)c->variant.designator.variant.array_element;
        designator = SageBuilder::buildUnsignedLongVal_nfi(index, std::to_string(index));
      }
      if (designator->get_startOfConstruct() == nullptr) {
        if (c->source_corresp.decl_position.seq != 0) {
          setPosition(designator, c->source_corresp.decl_position);
        } else {
          setCompilerGenerated(designator);
        }
      }
      SgInitializer* vi = nullptr;
      if (value->kind == ck_init_repeat) {
        // GNU range designator "[a ... b] = v": keep the first element
        vi = element(value->variant.init_repeat.constant);
      } else {
        vi = element(value);
      }
      SgExprListExp* dl = SageBuilder::buildExprListExp_nfi();
      dl->append_expression(designator);
      designator->set_parent(dl);
      setCompilerGenerated(dl);
      SgDesignatedInitializer* di = new SgDesignatedInitializer(dl, vi);
      dl->set_parent(di);
      if (vi) vi->set_parent(di);
      if (c->source_corresp.decl_position.seq != 0) {
        setPosition(di, c->source_corresp.decl_position,
                    value->end_position.seq != 0 ? value->end_position : c->source_corresp.decl_position);
      } else {
        setCompilerGenerated(di);
      }
      list->append_expression(di);
      di->set_parent(list);
      c = value;
      continue;
    }
    if (c->kind == ck_init_repeat) {
      for (a_targ_size_t i = 0; i < c->variant.init_repeat.count; i++) {
        SgInitializer* e = element(c->variant.init_repeat.constant);
        list->append_expression(e);
        e->set_parent(list);
      }
      continue;
    }
    SgInitializer* e = element(c);
    if (e == nullptr) continue;
    list->append_expression(e);
    e->set_parent(list);
  }
}

}  // namespace edg2sage
