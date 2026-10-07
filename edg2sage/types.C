// Translation of EDG types to Sage III types, and on-demand creation of the
// (first nondefining) declarations that named types refer to.
#include "edg2sage.h"

#include "il_to_str.h"

#include <cstdio>

using namespace edg;

namespace edg2sage {

namespace {

SgType* qualify(SgType* base, a_type_qualifier_set quals) {
  if ((quals & (TQ_CONST | TQ_VOLATILE | TQ_RESTRICT)) == 0) return base;
  SgModifierType* mt = new SgModifierType(base);
  if (quals & TQ_CONST) mt->get_typeModifier().get_constVolatileModifier().setConst();
  if (quals & TQ_VOLATILE) mt->get_typeModifier().get_constVolatileModifier().setVolatile();
  if (quals & TQ_RESTRICT) mt->get_typeModifier().setRestrict();
  SgModifierType* shared = SgModifierType::insertModifierTypeIntoTypeTable(mt);
  if (shared != mt) delete mt;
  return shared;
}

SgType* integerType(a_type_ptr type) {
  if (type->variant.integer.bool_type) return SgTypeBool::createType();
  if (type->variant.integer.wchar_t_type) return SgTypeWchar::createType();
  if (type->variant.integer.char16_t_type) return SgTypeChar16::createType();
  if (type->variant.integer.char32_t_type) return SgTypeChar32::createType();
  if (type->variant.integer.char8_t_type) return SgTypeUnsignedChar::createType();
  bool explicitlySigned = type->variant.integer.explicitly_signed;
  switch (type->variant.integer.int_kind) {
    case ik_char:
      return SgTypeChar::createType();
    case ik_signed_char:
      return SgTypeSignedChar::createType();
    case ik_unsigned_char:
      return SgTypeUnsignedChar::createType();
    case ik_short:
      return explicitlySigned ? (SgType*)SgTypeSignedShort::createType() : SgTypeShort::createType();
    case ik_unsigned_short:
      return SgTypeUnsignedShort::createType();
    case ik_int:
      return explicitlySigned ? (SgType*)SgTypeSignedInt::createType() : SgTypeInt::createType();
    case ik_unsigned_int:
      return SgTypeUnsignedInt::createType();
    case ik_long:
      return explicitlySigned ? (SgType*)SgTypeSignedLong::createType() : SgTypeLong::createType();
    case ik_unsigned_long:
      return SgTypeUnsignedLong::createType();
    case ik_long_long:
      return explicitlySigned ? (SgType*)SgTypeSignedLongLong::createType() : SgTypeLongLong::createType();
    case ik_unsigned_long_long:
      return SgTypeUnsignedLongLong::createType();
    case ik_int128:
      return SgTypeSigned128bitInteger::createType();
    case ik_unsigned_int128:
      return SgTypeUnsigned128bitInteger::createType();
    default:
      return SgTypeInt::createType();
  }
}

SgType* floatType(a_float_kind kind) {
  switch (kind) {
    case fk_float16:
      return SgTypeFloat16::createType();
    case fk_fp16:
      return SgTypeFp16::createType();
    case fk_float:
      return SgTypeFloat::createType();
    case fk_float32x:
      return SgTypeFloat32x::createType();
    case fk_double:
      return SgTypeDouble::createType();
    case fk_float64x:
      return SgTypeFloat64x::createType();
    case fk_long_double:
      return SgTypeLongDouble::createType();
    case fk_float80:
      return SgTypeFloat80::createType();
    case fk_float128:
      return SgTypeFloat128::createType();
    case fk_std_bfloat16:
      return SgTypeBFloat16::createType();
    case fk_std_float16:
      return SgTypeFloat16::createType();
    case fk_std_float32:
      return SgTypeFloat32::createType();
    case fk_std_float64:
      return SgTypeFloat64::createType();
    case fk_std_float128:
      return SgTypeFloat128::createType();
    default:
      return SgTypeDouble::createType();
  }
}

std::string anonymousName(const void* entity) {
  char buf[64];
  std::snprintf(buf, sizeof buf, "__anonymous_%p", entity);
  return buf;
}

}  // namespace

// The operand of "decltype(e)" or "typeof(e)": in the type entry, or (for a type
// written in a function) on the function's list of local expressions.
static an_expr_node_ptr typeOperand(a_type_ptr type) {
  a_typeref_type_supplement_ptr ext = type->variant.typeref.extra_info;
  if (ext == nullptr) return nullptr;
  if (ext->expr != nullptr) return ext->expr;
  a_routine_ptr r = type->source_corresp.enclosing_routine;
  if (r == nullptr || r->function_def_number == NULL_function_def_number) return nullptr;
  a_scope_ptr scope = scope_for_routine_or_null(r);
  return scope != nullptr ? find_local_expr_node_in_scope((char*)type, lerk_decltype, scope) : nullptr;
}

SgFunctionType* Translator::convertFunctionType(a_type_ptr type, SgClassDefinition* memberOf, SgType* memberClassType) {
  a_type_ptr rt = skip_typerefs(type);
  ROSE_ASSERT(rt->kind == tk_routine);
  a_routine_type_supplement_ptr rtsp = rt->variant.routine.extra_info;
  SgType* returnType = convertType(rt->variant.routine.return_type);
  SgFunctionParameterTypeList* params = new SgFunctionParameterTypeList();
  for (a_param_type_ptr p = rtsp->param_type_list; p != nullptr; p = p->next) {
    params->append_argument(convertType(p->type));
  }
  if (rtsp->has_ellipsis) params->append_argument(SgTypeEllipse::createType());
  if (memberOf == nullptr && rtsp->this_class != nullptr) {
    memberOf = classDefinitionFor(rtsp->this_class);
  }
  SgFunctionType* ft = nullptr;
  if (memberOf != nullptr || memberClassType != nullptr) {
    unsigned int cv = 0;
    // The cv-qualifiers of a member function ("int f() const")
    unsigned int q = rtsp->qualifiers | rtsp->this_qualifiers;
    if (q & TQ_CONST) cv |= SgMemberFunctionType::e_const;
    if (q & TQ_VOLATILE) cv |= SgMemberFunctionType::e_volatile;
    if (q & TQ_RESTRICT) cv |= SgMemberFunctionType::e_restrict;
    if (rtsp->ref_qualifiers == rqk_lvalue) cv |= SgMemberFunctionType::e_ref_qualifier_lvalue;
    if (rtsp->ref_qualifiers == rqk_rvalue) cv |= SgMemberFunctionType::e_ref_qualifier_rvalue;
    ft = memberOf != nullptr ? SageBuilder::buildMemberFunctionType(returnType, params, memberOf, cv)
                             : SageBuilder::buildMemberFunctionType(returnType, params, memberClassType, cv);
  } else {
    ft = SageBuilder::buildFunctionType(returnType, params);
  }
  if (!rtsp->prototyped) ft->set_has_ellipses(false);
  return ft;
}

SgType* Translator::convertType(a_type_ptr type) {
  if (type == nullptr) return SgTypeUnknown::createType();
  auto cached = typeCache.find(type);
  if (cached != typeCache.end()) return cached->second;

  SgType* result = nullptr;
  switch (type->kind) {
    case tk_error:
    case tk_unknown:
      result = SgTypeUnknown::createType();
      break;
    case tk_void:
      result = SgTypeVoid::createType();
      break;
    case tk_integer:  // includes tk_enum
      if (type->variant.integer.enum_type) {
        SgEnumDeclaration* decl = enumDeclarationFor(type);
        result = decl->get_type();
      } else {
        result = integerType(type);
      }
      break;
    case tk_float:
      result = floatType(type->variant.float_kind);
      break;
#if C99_IL_EXTENSIONS_SUPPORTED
    case tk_complex:
      result = SgTypeComplex::createType(floatType(type->variant.float_kind));
      break;
    case tk_imaginary:
      result = SgTypeImaginary::createType(floatType(type->variant.float_kind));
      break;
#endif
    case tk_pointer: {
      SgType* base = convertType(type->variant.pointer.type);
      if (type->variant.pointer.is_rvalue_reference) {
        result = SageBuilder::buildRvalueReferenceType(base);
      } else if (type->variant.pointer.is_reference) {
        result = SageBuilder::buildReferenceType(base);
      } else {
        result = SageBuilder::buildPointerType(base);
      }
      break;
    }
    case tk_routine:
      result = convertFunctionType(type);
      break;
    case tk_array: {
      SgType* base = convertType(type->variant.array.element_type);
      SgExpression* index = nullptr;
      bool vla = type->variant.array.is_vla || type->variant.array.is_variable_size_array;
      if (vla) {
        if (type->variant.array.has_assoc_vla_dimension && currentRoutine != nullptr) {
          // The dimension expression is on a list of the function scope.
          a_scope_ptr fs = functionScopeOf(currentRoutine);
          for (a_vla_dimension_ptr vd = fs != nullptr ? fs->vla_dimensions : nullptr; vd != nullptr; vd = vd->next) {
            if (vd->type != type) continue;
            a_vla_dimension_ptr orig = vd->dimension_expr != nullptr ? vd : vd->original_dimension;
            if (orig != nullptr && orig->dimension_expr != nullptr) index = convertExpression(orig->dimension_expr);
            break;
          }
        }
        if (index == nullptr) {
          // "[*]" (e.g. in a prototype): the unparser prints the VLA flag as "*"
          index = SageBuilder::buildNullExpression_nfi();
          setCompilerGenerated(index);
        }
      } else if (type->variant.array.bound_constant != nullptr) {
        index = convertConstant(type->variant.array.bound_constant);
      } else if (!type->incomplete) {
        index = SageBuilder::buildUnsignedLongVal_nfi(type->variant.array.variant.number_of_elements, "");
        setCompilerGenerated(index);
      }
      SgArrayType* at = SageBuilder::buildArrayType(base, index);
      if (index != nullptr) index->set_parent(at);
      if (vla) at->set_is_variable_length_array(true);
      if (type->variant.array.qualifiers & (TQ_CONST | TQ_VOLATILE | TQ_RESTRICT)) {
        result = qualify(at, type->variant.array.qualifiers);
      } else {
        result = at;
      }
      // Array types own their index expression, so they are not cached.
      return result;
    }
    case tk_class:
    case tk_struct:
    case tk_union: {
      SgClassDeclaration* decl = classDeclarationFor(type);
      result = decl->get_type();
      break;
    }
    case tk_typeref: {
      a_typeref_kind trk = type->variant.typeref.kind;
      if (trk == trk_is_template_alias) {
        // An instance of an alias template ("Table<int>"): the aliased type
        // (ROSE has no representation of alias template instances).
        result = convertType(type->variant.typeref.type);
        if (type->variant.typeref.qualifiers != 0) result = qualify(result, type->variant.typeref.qualifiers);
      } else if (typeref_is_typedef(type)) {
        SgTypedefDeclaration* decl = typedefDeclarationFor(type);
        result = decl ? decl->get_type() : convertType(type->variant.typeref.type);
      } else if (typeref_is_qualified(type)) {
        result = qualify(convertType(type->variant.typeref.type), type->variant.typeref.qualifiers);
      } else if (trk == trk_is_typeof_with_expression && typeOperand(type) != nullptr) {
        SgExpression* e = convertExpression(typeOperand(type));
        SgTypeOfType* tt = new SgTypeOfType(e, nullptr);
        e->set_parent(tt);
        result = tt;
        return result;  // owns its expression
      } else if (trk == trk_is_typeof_with_type_operand) {
        SgType* operand = convertType(type->variant.typeref.type);
        result = new SgTypeOfType(nullptr, operand);
      } else if (trk == trk_is_decltype && typeOperand(type) != nullptr) {
        SgExpression* e = convertExpression(typeOperand(type));
        SgDeclType* dt = new SgDeclType(e, convertType(type->variant.typeref.type));
        e->set_parent(dt);
        return dt;
      } else {
        // Other typerefs (attributes, deduced types, type traits, ...): use the target type.
        result = convertType(type->variant.typeref.type);
        if (type->variant.typeref.qualifiers != 0) result = qualify(result, type->variant.typeref.qualifiers);
      }
      break;
    }
    case tk_ptr_to_member: {
      // A pointer to member function points to a member function type.
      SgType* cls = convertType(type->variant.ptr_to_member.class_of_which_a_member);
      a_type_ptr mt = skip_typerefs(type->variant.ptr_to_member.type);
      SgType* base = mt != nullptr && mt->kind == tk_routine && mt == type->variant.ptr_to_member.type
                         ? convertFunctionType(mt, nullptr, cls)
                         : convertType(type->variant.ptr_to_member.type);
      result = SgPointerMemberType::createType(base, cls);
      break;
    }
    case tk_nullptr:
      result = SgTypeNullptr::createType();
      break;
    case tk_template_param:
      // "auto" (and "decltype(auto)") in a declaration is a special template
      // parameter type; other template parameter types only occur in templates.
      if (is_decltype_auto_type(type)) {
        result = decltypeAutoType();
      } else {
        result = is_auto_type(type) ? (SgType*)SageBuilder::buildAutoType() : SgTypeUnknown::createType();
      }
      break;
#if GNU_VECTOR_TYPES_ALLOWED
    case tk_vector:
      result = vectorType(type);
      break;
#endif
    default:
      result = SgTypeUnknown::createType();
      break;
  }
  typeCache[type] = result;
  return result;
}

// "enum E : unsigned char": the explicitly specified underlying type
void Translator::setEnumBase(SgEnumDeclaration* decl, a_type_ptr type) {
  if (type->variant.integer.has_explicit_enum_base && type->variant.integer.extra_info != nullptr &&
      type->variant.integer.extra_info->base_type != nullptr) {
    decl->set_field_type(convertType(type->variant.integer.extra_info->base_type));
  }
}

// A type ROSE has no representation of, printed as written: the type of a
// hidden typedef (in global scope, never unparsed itself) with that name.
SgType* Translator::pseudoType(const std::string& name, SgType* base) {
  SgTypedefDeclaration*& decl = pseudoTypes[name];
  if (decl == nullptr) {
    decl = new SgTypedefDeclaration(SgName(name), base, nullptr, nullptr, nullptr);
    decl->set_firstNondefiningDeclaration(decl);
    decl->set_scope(globalScope);
    decl->set_parent(globalScope);
    // (ROSE's mangled names, e.g. of template arguments, must not contain '<')
    SgNode::get_globalMangledNameMap()[decl] = "typedef_pseudo_" + std::to_string(pseudoTypes.size());
    decl->set_type(SgTypedefType::createType(decl));
    setCompilerGenerated(decl);
    globalScope->insert_symbol(SgName(name), new SgTypedefSymbol(decl));
  }
  return decl->get_type();
}

// The text of a type as EDG writes it (with qualified names and without
// typedefs), e.g. "int (const ns::S &)".
static void appendTypeText(a_const_char* str, an_il_to_str_output_control_block_ptr octl) {
  static_cast<std::string*>(octl->text_buffer)->append(str);
}

std::string Translator::typeText(a_type_ptr type) {
  std::string text;
  an_il_to_str_output_control_block octl;
  clear_il_to_str_output_control_block(&octl);
  octl.output_str = appendTypeText;
  octl.text_buffer = &text;
  octl.suppress_typedefs = TRUE;
  form_type(type, &octl);
  return text;
}

// The text of a template argument as EDG writes it.
std::string Translator::templateArgumentText(a_template_arg_ptr arg) {
  std::string text;
  an_il_to_str_output_control_block octl;
  clear_il_to_str_output_control_block(&octl);
  octl.output_str = appendTypeText;
  octl.text_buffer = &text;
  octl.suppress_typedefs = TRUE;
  form_a_template_arg(arg, &octl);
  return text;
}

// The type of a template type argument.  ROSE prints function and array types
// in template arguments wrongly when the template instance is used in a
// reference or pointer to a cv-qualified type ("const std::function<int (int)>&"
// becomes "const std::function<int ()(int)>&"): those are printed as written by
// EDG, as pseudo types.
SgType* Translator::templateArgumentType(a_type_ptr type) {
  SgType* t = convertType(type);
  a_type_ptr k = skip_typerefs(type);
  if (k != nullptr && (k->kind == tk_routine || k->kind == tk_array) && isCxx) {
    std::string text = typeText(type);
    if (!text.empty()) return pseudoType(text, t);
  }
  return t;
}

// An expression ROSE has no representation of, printed as written: a reference
// to a hidden variable (in global scope, never unparsed itself) with that name.
SgExpression* Translator::pseudoExpression(const std::string& text, SgType* type) {
  SgVariableSymbol*& sym = pseudoVariables[text];
  if (sym == nullptr) {
    SgInitializedName* in = SageBuilder::buildInitializedName_nfi(SgName(text), type, nullptr);
    SgVariableDeclaration* decl = new SgVariableDeclaration(in);
    decl->set_firstNondefiningDeclaration(decl);
    decl->set_parent(globalScope);
    in->set_scope(globalScope);
    in->set_parent(decl);
    setCompilerGenerated(decl);
    setCompilerGenerated(in);
    sym = new SgVariableSymbol(in);
    globalScope->insert_symbol(SgName(text), sym);
  }
  SgVarRefExp* r = SageBuilder::buildVarRefExp_nfi(sym);
  setCompilerGenerated(r);
  return r;
}

// "decltype(auto)"
SgType* Translator::decltypeAutoType() {
  return pseudoType("decltype(auto)", SageBuilder::buildAutoType());
}

// GNU vector types: "float __attribute__((__vector_size__(16)))"
SgType* Translator::vectorType(a_type_ptr type) {
  SgType* element = convertType(skip_typerefs(type->variant.vector.element_type));
  std::string name = element->unparseToString() + " __attribute__((__vector_size__(" +
                     std::to_string((unsigned long)type->size) + ")))";
  return pseudoType(name, element);
}

// ---------------------------------------------------------------------------------
// Declarations created on demand for named types
// ---------------------------------------------------------------------------------

SgClassDeclaration* Translator::classDeclarationFor(a_type_ptr type) {
  type = skip_typerefs(type);
  auto it = firstClassDecl.find(type);
  if (it != firstClassDecl.end()) return it->second;
  if (isTemplateInstance(type)) return instanceDeclarationFor(type);

  SgClassDeclaration::class_types kind = SgClassDeclaration::e_struct;
  if (type->kind == tk_class) kind = SgClassDeclaration::e_class;
  if (type->kind == tk_union) kind = SgClassDeclaration::e_union;
  // An unnamed class named by a typedef ("typedef struct {...} S;") has the
  // typedef name for linkage purposes only.
  bool unnamed = (type->source_corresp.name == nullptr) || type->variant.class_struct_union.originally_unnamed;
  SgName name = unnamed ? SgName(anonymousName(type)) : SgName(type->source_corresp.name);
  SgScopeStatement* scope = parentScopeOf(&type->source_corresp);

  SgClassDeclaration* decl = new SgClassDeclaration(name, kind, nullptr, nullptr);
  decl->set_firstNondefiningDeclaration(decl);
  decl->set_definingDeclaration(nullptr);
  decl->setForward();
  decl->set_scope(scope);
  decl->set_parent(scope);
  decl->set_isUnNamed(unnamed);
  decl->set_type(SgClassType::createType(decl));
  setPosition(decl, type->source_corresp.decl_position);
  SgClassSymbol* sym = new SgClassSymbol(decl);
  scope->insert_symbol(name, sym);
  firstClassDecl[type] = decl;
  return decl;
}

SgClassDefinition* Translator::classDefinitionFor(a_type_ptr type) {
  type = skip_typerefs(type);
  auto it = definingClassDecl.find(type);
  if (it != definingClassDecl.end()) return it->second->get_definition();
  return hiddenDefinitionFor(type);
}

SgEnumDeclaration* Translator::enumDeclarationFor(a_type_ptr type) {
  auto it = firstEnumDecl.find(type);
  if (it != firstEnumDecl.end()) return it->second;
  bool unnamed = (type->source_corresp.name == nullptr);
  SgName name = unnamed ? SgName(anonymousName(type)) : SgName(type->source_corresp.name);
  SgScopeStatement* scope = parentScopeOf(&type->source_corresp);

  SgEnumDeclaration* decl = new SgEnumDeclaration(name, nullptr);
  decl->set_firstNondefiningDeclaration(decl);
  decl->set_definingDeclaration(nullptr);
  decl->setForward();
  decl->set_scope(scope);
  decl->set_parent(scope);
  decl->set_isUnNamed(unnamed);
  decl->set_type(SgEnumType::createType(decl));
  if (type->variant.integer.is_scoped_enum) decl->set_isScopedEnum(true);
  setEnumBase(decl, type);
  setPosition(decl, type->source_corresp.decl_position);
  SgEnumSymbol* sym = new SgEnumSymbol(decl);
  scope->insert_symbol(name, sym);
  firstEnumDecl[type] = decl;
  return decl;
}

// The enumerator of an enumeration constant.  The enumerators of enumerations
// whose definition is not translated (members of template instances) are
// created on demand, with the first declaration of the enumeration.
SgInitializedName* Translator::enumeratorFor(a_constant_ptr e, a_type_ptr enumType) {
  auto it = enumerators.find(e);
  if (it != enumerators.end()) return it->second;
  SgName name = nameOf(&e->source_corresp);
  if (name.is_null()) return nullptr;
  SgEnumDeclaration* ed = enumDeclarationFor(enumType);
  SgInitializedName* in = SageBuilder::buildInitializedName_nfi(name, ed->get_type(), nullptr);
  in->set_parent(ed);
  in->set_scope(ed->get_scope());
  setCompilerGenerated(in);
  ed->get_scope()->insert_symbol(name, new SgEnumFieldSymbol(in));
  enumerators[e] = in;
  return in;
}

// A typedef of a class or enum type refers to the declaration of that type
// (the defining one if the typedef contains the definition, see
// attachPendingBaseTypeDeclaration()).
void Translator::setTypedefBaseDeclaration(SgTypedefDeclaration* decl) {
  if (decl->get_declaration() != nullptr) return;
  SgType* base = decl->get_base_type();
  SgDeclarationStatement* d = nullptr;
  if (SgClassType* ct = isSgClassType(base)) d = ct->get_declaration();
  if (SgEnumType* et = isSgEnumType(base)) d = et->get_declaration();
  if (d != nullptr) {
    if (d->get_firstNondefiningDeclaration() != nullptr) d = d->get_firstNondefiningDeclaration();
    decl->set_declaration(d);
  }
}

// The symbol of the class or namespace a typedef is declared in (as SageBuilder
// sets it): SgTypedefType::createType returns an existing typedef type with the
// same name, base type and parent scope symbol, so member typedefs of different
// classes would otherwise share one type ("A::size_type" for "B::size_type").
SgSymbol* Translator::typedefParentScope(SgScopeStatement* scope) {
  SgDeclarationStatement* decl = nullptr;
  if (SgClassDefinition* cd = isSgClassDefinition(scope)) decl = cd->get_declaration();
  else if (SgNamespaceDefinitionStatement* nd = isSgNamespaceDefinitionStatement(scope)) decl = nd->get_namespaceDeclaration();
  if (decl == nullptr) return nullptr;
  SgDeclarationStatement* withSymbol = decl->get_declaration_associated_with_symbol();
  return withSymbol != nullptr ? withSymbol->get_symbol_from_symbol_table() : nullptr;
}

SgTypedefDeclaration* Translator::typedefDeclarationFor(a_type_ptr type) {
  auto it = typedefDecls.find(type);
  if (it != typedefDecls.end()) return it->second;
  // A typedef that was not (yet) encountered on a source sequence list: create
  // a declaration that is not part of any statement list.
  SgScopeStatement* scope = parentScopeOf(&type->source_corresp);
  SgType* base = convertType(type->variant.typeref.type);
  auto again = typedefDecls.find(type);  // converting the base type may have created it
  if (again != typedefDecls.end()) return again->second;
  SgName name = nameOf(&type->source_corresp);
  SgTypedefDeclaration* decl = new SgTypedefDeclaration(name, base, nullptr, nullptr, typedefParentScope(scope));
  decl->set_firstNondefiningDeclaration(decl);  // ROSE: typedefs have no defining declaration
  decl->set_scope(scope);
  decl->set_parent(scope);
  decl->set_type(SgTypedefType::createType(decl));
  setTypedefBaseDeclaration(decl);
  setPosition(decl, type->source_corresp.decl_position);
  SgTypedefSymbol* sym = new SgTypedefSymbol(decl);
  scope->insert_symbol(name, sym);
  typedefDecls[type] = decl;
  return decl;
}

}  // namespace edg2sage
