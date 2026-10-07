// Templates.
//
// ROSE prints template declarations from their text (get_string()), which EDG
// records in the IL (RECORD_TEMPLATE_STRINGS, see edgconfig/edg_config.h): the
// members of class templates and the bodies of function templates are not
// translated.  Template instances -- the classes and functions EDG generates
// from templates -- are represented by ROSE's SgTemplateInstantiation*
// declarations.  They are created when they are referenced and are not part of
// any statement list (the back-end compiler instantiates the templates
// again), except explicit specializations, which are written in the source.
#include "edg2sage.h"

using namespace edg;
using namespace Sawyer::Message;

namespace edg2sage {

namespace {

a_template_ptr canonicalOf(a_template_ptr t) {
  return t != nullptr && t->canonical_template != nullptr ? t->canonical_template : t;
}

bool isClassKind(a_type_kind k) { return k == tk_class || k == tk_struct || k == tk_union; }

SgClassDeclaration::class_types classKindOf(a_type_ptr t) {
  if (t != nullptr && t->kind == tk_class) return SgClassDeclaration::e_class;
  if (t != nullptr && t->kind == tk_union) return SgClassDeclaration::e_union;
  return SgClassDeclaration::e_struct;
}

// The class (or enum) type a class-like template declares, NULL for alias
// templates.
a_type_ptr prototypeTagType(a_template_ptr t) {
  if (t->kind != templk_class && t->kind != templk_member_class && t->kind != templk_member_enum) return nullptr;
  a_type_ptr type = t->prototype_instantiation.type;
  if (type == nullptr) return nullptr;
  if (isClassKind(type->kind)) return type;
  if (type->kind == tk_integer && type->variant.integer.enum_type) return type;
  return nullptr;
}

}  // namespace

std::string Translator::templateText(a_template_ptr tmpl) {
#if RECORD_TEMPLATE_STRINGS
  if (tmpl != nullptr && tmpl->text != nullptr) return std::string(tmpl->text);
#endif
  return "";
}

// ---------------------------------------------------------------------------------
// Template declarations
// ---------------------------------------------------------------------------------

SgDeclarationStatement* Translator::templateDeclarationFor(a_template_ptr tmpl) {
  a_template_ptr t = canonicalOf(tmpl);
  if (t == nullptr) return nullptr;
  auto it = firstTemplateDecl.find(t);
  if (it != firstTemplateDecl.end()) return it->second;

  SgScopeStatement* scope = parentScopeOf(&t->source_corresp, globalScope);
  SgName name = nameOf(&t->source_corresp);
  SgDeclarationStatement* decl = nullptr;
  switch (t->kind) {
    case templk_function:
    case templk_member_function: {
      SgClassDefinition* cdef = isSgClassDefinition(scope);
      a_routine_ptr proto = t->prototype_instantiation.routine;
      SgFunctionType* ftype = nullptr;
      if (proto != nullptr) {
        try {
          ftype = convertFunctionType(proto->type, cdef);
        } catch (const Unsupported&) {
        }
      }
      if (ftype == nullptr) {
        ftype = SageBuilder::buildFunctionType(SgTypeUnknown::createType(), SageBuilder::buildFunctionParameterTypeList());
      }
      auto again = firstTemplateDecl.find(t);
      if (again != firstTemplateDecl.end()) return again->second;
      SgFunctionDeclaration* fd = nullptr;
      if (cdef != nullptr) {
        SgTemplateMemberFunctionDeclaration* m = new SgTemplateMemberFunctionDeclaration(name, ftype, nullptr);
        fd = m;
      } else {
        SgTemplateFunctionDeclaration* f = new SgTemplateFunctionDeclaration(name, ftype, nullptr);
        fd = f;
      }
      fd->get_parameterList()->set_parent(fd);
      setPosition(fd->get_parameterList(), t->source_corresp.decl_position);
      fd->setForward();
      if (proto != nullptr) setSpecialFunctionKind(fd, proto);
      decl = fd;
      SgSymbol* sym = cdef != nullptr ? (SgSymbol*)new SgTemplateMemberFunctionSymbol(fd) : new SgTemplateFunctionSymbol(fd);
      scope->insert_symbol(name, sym);
      break;
    }
    case templk_variable:
    case templk_static_data_member: {
      a_variable_ptr proto = t->prototype_instantiation.variable;
      SgType* type = SgTypeUnknown::createType();
      if (proto != nullptr) {
        try {
          type = convertType(proto->type);
        } catch (const Unsupported&) {
        }
      }
      auto again = firstTemplateDecl.find(t);
      if (again != firstTemplateDecl.end()) return again->second;
      SgTemplateVariableDeclaration* vd = new SgTemplateVariableDeclaration(name, type, nullptr);
      SgInitializedName* in = vd->get_variables().empty() ? nullptr : vd->get_variables()[0];
      if (in != nullptr) {
        in->set_scope(scope);
        in->set_parent(vd);
        setPosition(in, t->source_corresp.decl_position);
        scope->insert_symbol(name, new SgTemplateVariableSymbol(in));
      }
      decl = vd;
      break;
    }
    default: {
      // Class templates (also alias templates and other templates, which are
      // only printed from their text).
      a_type_ptr proto = prototypeTagType(t);
      SgClassDeclaration::class_types kind = classKindOf(proto);
      SgTemplateClassDeclaration* cd = new SgTemplateClassDeclaration(name, kind, nullptr, nullptr);
      cd->set_templateName(name);
      cd->setForward();
      cd->set_firstNondefiningDeclaration(cd);
      cd->set_scope(scope);
      cd->set_parent(scope);
      cd->set_type(SgClassType::createType(cd));
      decl = cd;
      // A partial specialization has the name of its primary template, which
      // is the one found by name lookup.
      if (scope->lookup_template_class_symbol(name, nullptr, nullptr) == nullptr) {
        scope->insert_symbol(name, new SgTemplateClassSymbol(cd));
      }
      break;
    }
  }
  decl->set_firstNondefiningDeclaration(decl);
  decl->set_definingDeclaration(nullptr);
  if (decl->hasExplicitScope()) decl->set_scope(scope);
  decl->set_parent(scope);
  setPosition(decl, t->source_corresp.decl_position);
  if (t->source_corresp.is_class_member) setAccess(decl, (an_access_specifier)t->source_corresp.access);
  firstTemplateDecl[t] = decl;
  return decl;
}

// The prototype members of a class template definition follow its entry, up
// to the end-of-construct entry of the template.
void Translator::skipTemplateMembers(SeqCursor& cursor, a_template_ptr tmpl) {
  skipToEndOfConstruct(cursor, tmpl);
}

void Translator::translateTemplate(SeqCursor& cursor, a_template_ptr tmpl, a_src_seq_secondary_decl_ptr sec,
                                   SgScopeStatement* scope) {
  cursor.advance();
  // A primary entry for a class template is its definition (a secondary entry
  // is a declaration), as in EDG's own C++-generating back end.
  a_type_ptr proto = prototypeTagType(tmpl);
  bool isDefinition = (sec == nullptr);
  if (proto != nullptr && isDefinition) skipTemplateMembers(cursor, tmpl);
  if (tmpl->kind == templk_template_template_param) return;

  SgDeclarationStatement* first = templateDeclarationFor(tmpl);
  if (first == nullptr) return;
  std::string text = templateText(tmpl);
  SgDeclarationStatement* decl = nullptr;
  if (SgTemplateClassDeclaration* fc = isSgTemplateClassDeclaration(first)) {
    SgTemplateClassDeclaration* cd = new SgTemplateClassDeclaration(fc->get_name(), fc->get_class_type(), fc->get_type(), nullptr);
    cd->set_templateName(fc->get_templateName());
    if (isDefinition && proto != nullptr && isClassKind(proto->kind)) {
      SgTemplateClassDefinition* def = new SgTemplateClassDefinition(cd);
      cd->set_definition(def);
      def->set_parent(cd);
      cd->set_definingDeclaration(cd);
      fc->set_definingDeclaration(cd);
      cd->unsetForward();
      // Out-of-class member definitions refer to the scope of the prototype
      // instantiation.
      definingClassDecl[proto] = cd;
      firstClassDecl[proto] = fc;
      a_class_type_supplement_ptr ctsp = proto->variant.class_struct_union.extra_info;
      if (ctsp != nullptr && ctsp->assoc_scope != nullptr) scopes[ctsp->assoc_scope] = def;
      setCompilerGenerated(def);
    } else {
      cd->setForward();
      cd->set_definingDeclaration(fc->get_definingDeclaration());
    }
    decl = cd;
  } else if (SgTemplateMemberFunctionDeclaration* fm = isSgTemplateMemberFunctionDeclaration(first)) {
    SgTemplateMemberFunctionDeclaration* m = new SgTemplateMemberFunctionDeclaration(fm->get_name(), fm->get_type(), nullptr);
    m->get_parameterList()->set_parent(m);
    m->get_specialFunctionModifier() = fm->get_specialFunctionModifier();
    decl = m;
  } else if (SgTemplateFunctionDeclaration* ff = isSgTemplateFunctionDeclaration(first)) {
    SgTemplateFunctionDeclaration* f = new SgTemplateFunctionDeclaration(ff->get_name(), ff->get_type(), nullptr);
    f->get_parameterList()->set_parent(f);
    decl = f;
  } else if (SgTemplateVariableDeclaration* fv = isSgTemplateVariableDeclaration(first)) {
    SgInitializedName* fin = fv->get_variables().empty() ? nullptr : fv->get_variables()[0];
    SgTemplateVariableDeclaration* v = new SgTemplateVariableDeclaration(fv->get_variables()[0]->get_name(),
                                                                         fin ? fin->get_type() : SgTypeUnknown::createType(), nullptr);
    for (SgInitializedName* in : v->get_variables()) {
      in->set_scope(first->get_scope());
      in->set_parent(v);
      if (fin != nullptr) in->set_prev_decl_item(fin);
      setPosition(in, tmpl->source_corresp.decl_position);
    }
    decl = v;
  }
  if (decl == nullptr) return;
  if (SgFunctionDeclaration* fd = isSgFunctionDeclaration(decl)) {
    bool hasBody = false;
#if EXTRA_SOURCE_POSITIONS_IN_IL
    hasBody = tmpl->definition_range.start.seq != 0;
#endif
    if (hasBody) {
      fd->set_definingDeclaration(fd);
      first->set_definingDeclaration(fd);
      definingTemplateDecl[canonicalOf(tmpl)] = fd;
    } else {
      fd->setForward();
      fd->set_definingDeclaration(first->get_definingDeclaration());
    }
    setPosition(fd->get_parameterList(), tmpl->source_corresp.decl_position);
  }
  decl->set_firstNondefiningDeclaration(first);
  if (decl->hasExplicitScope()) decl->set_scope(first->get_scope());
  decl->set_parent(scope);
  if (SgTemplateClassDeclaration* cd = isSgTemplateClassDeclaration(decl)) cd->set_string(text);
  if (SgTemplateFunctionDeclaration* fd = isSgTemplateFunctionDeclaration(decl)) fd->set_string(text);
  if (SgTemplateMemberFunctionDeclaration* md = isSgTemplateMemberFunctionDeclaration(decl)) md->set_string(text);
  if (SgTemplateVariableDeclaration* vd = isSgTemplateVariableDeclaration(decl)) vd->set_string(text);
  if (tmpl->source_corresp.is_class_member) setAccess(decl, (an_access_specifier)tmpl->source_corresp.access);

  // Positions: from the "template" keyword to the end of the definition.
  a_source_position start = tmpl->source_corresp.decl_position;
  if (tmpl->template_decl != nullptr && tmpl->template_decl->template_pos.seq != 0) {
    start = tmpl->template_decl->template_pos;
  }
  a_source_position end = tmpl->source_corresp.decl_position;
#if EXTRA_SOURCE_POSITIONS_IN_IL
  if (tmpl->definition_range.end.seq != 0) end = tmpl->definition_range.end;
#endif
  setPosition(decl, start, end);
  if (first->get_startOfConstruct() == nullptr || first->get_startOfConstruct()->isCompilerGenerated()) {
    setPosition(first, start);
  }
  appendStatementTo(scope, decl);
}

// "template class X<int>;", "extern template void f<int>(int);"
SgDeclarationStatement* Translator::translateInstantiationDirective(an_instantiation_directive_ptr id,
                                                                    SgScopeStatement* scope) {
  SgDeclarationStatement* decl = nullptr;
  switch ((an_il_entry_kind)id->entity.kind) {
    case iek_type: {
      a_type_ptr type = skip_typerefs((a_type_ptr)id->entity.ptr);
      if (!isTemplateInstance(type)) break;
      SgClassDeclaration* first = classDeclarationFor(type);
      SgClassDeclaration* d = newNondefiningClassDeclaration(first);
      if (SgTemplateInstantiationDecl* ti = isSgTemplateInstantiationDecl(d)) {
        for (SgTemplateArgument* a : convertTemplateArguments(type->variant.class_struct_union.extra_info->template_arg_list)) {
          a->set_parent(ti);
          ti->get_templateArguments().push_back(a);
        }
      }
      decl = d;
      break;
    }
    case iek_routine: {
      a_routine_ptr routine = (a_routine_ptr)id->entity.ptr;
      if (routine->template_arg_list == nullptr) break;  // a member of a class template instance
      decl = translateRoutine(routine, nullptr, scope, true);
      break;
    }
    default:
      break;
  }
  if (decl == nullptr) throw Unsupported("explicit instantiation of this kind of entity");
  SgTemplateInstantiationDirectiveStatement* s = new SgTemplateInstantiationDirectiveStatement(decl);
  decl->set_parent(s);
  s->set_firstNondefiningDeclaration(s);
  s->set_do_not_instantiate(id->do_not_instantiate);
  setPosition(s, id->position);
  setPosition(decl, id->position);
  return s;
}

// ---------------------------------------------------------------------------------
// Template arguments
// ---------------------------------------------------------------------------------

SgTemplateArgumentPtrList Translator::convertTemplateArguments(a_template_arg_ptr args) {
  SgTemplateArgumentPtrList result;
  for (a_template_arg_ptr a = args; a != nullptr; a = a->next) {
    SgTemplateArgument* arg = nullptr;
    switch (a->kind) {
      case tak_type:
        arg = new SgTemplateArgument(SgTemplateArgument::type_argument, false, convertType(a->variant.type), nullptr,
                                     nullptr, a->explicitly_specified);
        break;
      case tak_nontype: {
        SgExpression* e = nullptr;
        if (!a->is_array_bound_of_unknown_type && a->variant.constant != nullptr) {
          e = convertConstant(a->variant.constant);
        }
        if (e == nullptr) {
          e = SageBuilder::buildIntVal_nfi(0, "0");
          setCompilerGenerated(e);
        }
        arg = new SgTemplateArgument(SgTemplateArgument::nontype_argument, false, nullptr, e, nullptr,
                                     a->explicitly_specified);
        e->set_parent(arg);
        fillMissingPositions(e);
        break;
      }
      case tak_template: {
        SgDeclarationStatement* td = templateDeclarationFor(a->variant.templ.ptr);
        arg = new SgTemplateArgument(SgTemplateArgument::template_template_argument, false, nullptr, nullptr, td,
                                     a->explicitly_specified);
        break;
      }
      case tak_start_of_pack_expansion:
        arg = new SgTemplateArgument(SgTemplateArgument::start_of_pack_expansion_argument, false, nullptr, nullptr,
                                     nullptr, a->explicitly_specified);
        break;
      default:
        continue;
    }
    arg->set_is_pack_element(a->is_pack_element);
    result.push_back(arg);
  }
  return result;
}

// ---------------------------------------------------------------------------------
// Class template instances
// ---------------------------------------------------------------------------------

bool Translator::isTemplateInstance(a_type_ptr type) {
  if (type == nullptr || !isClassKind(type->kind)) return false;
  a_class_type_supplement_ptr ctsp = type->variant.class_struct_union.extra_info;
  return ctsp != nullptr && ctsp->assoc_template != nullptr && ctsp->template_arg_list != nullptr;
}

SgClassDeclaration* Translator::instanceDeclarationFor(a_type_ptr type) {
  a_class_type_supplement_ptr ctsp = type->variant.class_struct_union.extra_info;
  SgTemplateClassDeclaration* tdecl = isSgTemplateClassDeclaration(templateDeclarationFor(ctsp->assoc_template));
  // A "nonreal" class (an instance whose arguments depend on template
  // parameters) only appears in templates: it is represented by the template.
  if (type->variant.class_struct_union.is_nonreal_class && tdecl != nullptr) {
    firstClassDecl[type] = tdecl;
    return tdecl;
  }
  SgScopeStatement* scope = parentScopeOf(&type->source_corresp);
  SgName templateName = nameOf(&type->source_corresp);
  SgTemplateArgumentPtrList args = convertTemplateArguments(ctsp->template_arg_list);
  auto again = firstClassDecl.find(type);  // created while converting the arguments
  if (again != firstClassDecl.end()) return again->second;
  SgName name = SageBuilder::appendTemplateArgumentsToName(templateName, args);
  SgTemplateInstantiationDecl* decl =
      new SgTemplateInstantiationDecl(name, classKindOf(type), nullptr, nullptr, tdecl, SgTemplateArgumentPtrList());
  decl->set_templateName(templateName);
  decl->set_nameResetFromMangledForm(true);
  for (SgTemplateArgument* a : args) {
    a->set_parent(decl);
    decl->get_templateArguments().push_back(a);
  }
  decl->set_firstNondefiningDeclaration(decl);
  decl->set_definingDeclaration(nullptr);
  decl->setForward();
  decl->set_scope(scope);
  decl->set_parent(scope);
  decl->set_type(SgClassType::createType(decl));
  if (type->variant.class_struct_union.is_specialized) {
    decl->set_specialization(SgDeclarationStatement::e_specialization);
    setPosition(decl, type->source_corresp.decl_position);
  } else {
    setCompilerGenerated(decl);
  }
  scope->insert_symbol(name, new SgClassSymbol(decl));
  firstClassDecl[type] = decl;
  return decl;
}

// A declaration of the same kind (class, class template, class template
// instance) as the first declaration of a class.
SgClassDeclaration* Translator::newNondefiningClassDeclaration(SgClassDeclaration* first) {
  SgClassDeclaration* d = nullptr;
  if (SgTemplateInstantiationDecl* ti = isSgTemplateInstantiationDecl(first)) {
    SgTemplateInstantiationDecl* n = new SgTemplateInstantiationDecl(ti->get_name(), ti->get_class_type(), ti->get_type(),
                                                                     nullptr, ti->get_templateDeclaration(),
                                                                     SgTemplateArgumentPtrList());
    n->set_templateName(ti->get_templateName());
    n->set_nameResetFromMangledForm(true);
    n->set_specialization(ti->get_specialization());
    d = n;
  } else if (SgTemplateClassDeclaration* tc = isSgTemplateClassDeclaration(first)) {
    SgTemplateClassDeclaration* n = new SgTemplateClassDeclaration(tc->get_name(), tc->get_class_type(), tc->get_type(), nullptr);
    n->set_templateName(tc->get_templateName());
    d = n;
  } else {
    d = new SgClassDeclaration(first->get_name(), first->get_class_type(), first->get_type(), nullptr);
  }
  d->set_firstNondefiningDeclaration(first);
  d->set_definingDeclaration(first->get_definingDeclaration());
  d->setForward();
  d->set_scope(first->get_scope());
  d->set_isUnNamed(first->get_isUnNamed());
  return d;
}

SgClassDeclaration* Translator::newDefiningClassDeclaration(SgClassDeclaration* first, SgClassDefinition*& cdef) {
  SgClassDeclaration* def = nullptr;
  if (SgTemplateInstantiationDecl* ti = isSgTemplateInstantiationDecl(first)) {
    SgTemplateInstantiationDecl* d = new SgTemplateInstantiationDecl(ti->get_name(), ti->get_class_type(), ti->get_type(),
                                                                     nullptr, ti->get_templateDeclaration(),
                                                                     SgTemplateArgumentPtrList());
    d->set_templateName(ti->get_templateName());
    d->set_nameResetFromMangledForm(true);
    d->set_specialization(ti->get_specialization());
    cdef = new SgTemplateInstantiationDefn(d);
    def = d;
  } else if (SgTemplateClassDeclaration* tc = isSgTemplateClassDeclaration(first)) {
    SgTemplateClassDeclaration* d = new SgTemplateClassDeclaration(tc->get_name(), tc->get_class_type(), tc->get_type(), nullptr);
    d->set_templateName(tc->get_templateName());
    cdef = new SgTemplateClassDefinition(d);
    def = d;
  } else {
    def = new SgClassDeclaration(first->get_name(), first->get_class_type(), first->get_type(), nullptr);
    cdef = new SgClassDefinition(def);
  }
  def->set_definition(cdef);
  cdef->set_parent(def);
  def->set_firstNondefiningDeclaration(first);
  def->set_definingDeclaration(def);
  first->set_definingDeclaration(def);
  def->set_scope(first->get_scope());
  def->set_isUnNamed(first->get_isUnNamed());
  def->unsetForward();
  return def;
}

// The definition of a class generated from a template (or nested in one),
// which is not part of the translated source: created for the scope of its
// members, when they are referenced.
SgClassDefinition* Translator::hiddenDefinitionFor(a_type_ptr type) {
  type = skip_typerefs(type);
  if (type == nullptr || !isClassKind(type->kind)) return nullptr;
  auto it = definingClassDecl.find(type);
  if (it != definingClassDecl.end()) return it->second->get_definition();
  a_class_type_supplement_ptr ctsp = type->variant.class_struct_union.extra_info;
  if (ctsp == nullptr || ctsp->assoc_scope == nullptr) return nullptr;  // not defined
  if (!type->variant.class_struct_union.is_template_class || type->variant.class_struct_union.is_nonreal_class) {
    return nullptr;
  }
  SgClassDeclaration* first = classDeclarationFor(type);
  auto again = definingClassDecl.find(type);
  if (again != definingClassDecl.end()) return again->second->get_definition();
  SgClassDefinition* cdef = nullptr;
  SgClassDeclaration* def = newDefiningClassDeclaration(first, cdef);
  def->set_parent(first->get_parent());
  if (SgTemplateInstantiationDecl* ti = isSgTemplateInstantiationDecl(def)) {
    for (SgTemplateArgument* a : convertTemplateArguments(ctsp->template_arg_list)) {
      a->set_parent(ti);
      ti->get_templateArguments().push_back(a);
    }
  }
  setCompilerGenerated(def);
  setCompilerGenerated(cdef);
  definingClassDecl[type] = def;
  hiddenDefinitions.insert(def);
  scopes[ctsp->assoc_scope] = cdef;
  translateBaseClasses(ctsp, cdef);
  return cdef;
}

// A field of a class that was not translated from the source (a class template
// instance): created on first reference.
SgInitializedName* Translator::hiddenFieldFor(a_field_ptr field) {
  SgClassDefinition* cdef = isSgClassDefinition(scopeFor(field->source_corresp.parent_scope));
  if (cdef == nullptr) return nullptr;
  auto it = fields.find(field);
  if (it != fields.end()) return it->second;
  SgType* type = convertType(field->type);
  SgName name = nameOf(&field->source_corresp);
  SgInitializedName* iname = SageBuilder::buildInitializedName_nfi(name, type, nullptr);
  SgVariableDeclaration* decl = new SgVariableDeclaration(iname);
  decl->set_firstNondefiningDeclaration(decl);
  decl->set_definingDeclaration(decl);
  iname->set_scope(cdef);
  setAccess(decl, (an_access_specifier)field->source_corresp.access);
  if (field->is_mutable) decl->get_declarationModifier().get_storageModifier().setMutable();
  setCompilerGenerated(decl);
  setCompilerGenerated(iname);
  if (SgDeclarationStatement* vdef = iname->get_declptr()) {
    if (vdef != decl) setCompilerGenerated(vdef);
  }
  cdef->append_member(decl);
  decl->set_parent(cdef);
  fields[field] = iname;
  if (!name.is_null()) cdef->insert_symbol(name, new SgVariableSymbol(iname));
  return iname;
}

// ---------------------------------------------------------------------------------
// Function template instances
// ---------------------------------------------------------------------------------

SgFunctionDeclaration* Translator::newFunctionDeclaration(a_routine_ptr routine, const SgName& name,
                                                          SgFunctionType* ftype, bool member) {
  if (routine->template_arg_list != nullptr && routine->assoc_template != nullptr) {
    // An instance (or explicit specialization) of a function template
    SgTemplateArgumentPtrList args = convertTemplateArguments(routine->template_arg_list);
    // ROSE names instances of operator templates without their arguments (so
    // that calls can be printed with operator syntax).
    bool isOperator = routine->special_kind == sfk_operator || routine->special_kind == sfk_conversion ||
                      routine->special_kind == sfk_udl_operator;
    SgName fullName = isOperator ? name : SageBuilder::appendTemplateArgumentsToName(name, args);
    SgDeclarationStatement* tdecl = templateDeclarationFor(routine->assoc_template);
    SgFunctionDeclaration* result = nullptr;
    if (member) {
      SgTemplateInstantiationMemberFunctionDecl* d = new SgTemplateInstantiationMemberFunctionDecl(fullName, ftype, nullptr);
      d->set_templateName(name);
      d->set_templateDeclaration(isSgTemplateMemberFunctionDeclaration(tdecl));
      d->set_nameResetFromMangledForm(true);
      d->set_template_argument_list_is_explicit(routine->expl_template_arg_list_used);
      for (SgTemplateArgument* a : args) {
        a->set_parent(d);
        d->get_templateArguments().push_back(a);
      }
      result = d;
    } else {
      SgTemplateInstantiationFunctionDecl* d = new SgTemplateInstantiationFunctionDecl(fullName, ftype, nullptr);
      d->set_templateName(name);
      d->set_templateDeclaration(isSgTemplateFunctionDeclaration(tdecl));
      d->set_nameResetFromMangledForm(true);
      d->set_template_argument_list_is_explicit(routine->expl_template_arg_list_used);
      for (SgTemplateArgument* a : args) {
        a->set_parent(d);
        d->get_templateArguments().push_back(a);
      }
      result = d;
    }
    if (routine->is_specialized) result->set_specialization(SgDeclarationStatement::e_specialization);
    return result;
  }
  if (member && routine->is_specialized && routine->is_template_function) {
    // An explicit specialization of a member of a class template
    // ("template<> void X<int>::f() {...}"): ROSE prints "template<>" for it.
    SgTemplateInstantiationMemberFunctionDecl* d = new SgTemplateInstantiationMemberFunctionDecl(name, ftype, nullptr);
    d->set_templateName(name);
    d->set_nameResetFromMangledForm(true);
    d->set_specialization(SgDeclarationStatement::e_specialization);
    return d;
  }
  if (member) return new SgMemberFunctionDeclaration(name, ftype, nullptr);
  return new SgFunctionDeclaration(name, ftype, nullptr);
}

}  // namespace edg2sage
