// GNU/C11 attributes: those that ROSE represents in the AST (and unparses) are
// copied from the EDG attribute lists and entity flags.
#include "edg2sage.h"

#include "const_ints.h"

using namespace edg;

namespace edg2sage {

namespace {

typedef std::vector<an_attribute_ptr> Attributes;

// The attributes written on one declaration.  The attribute list of an entity
// accumulates the attributes of all its declarations; those that appeared on
// the primary declaration are marked (a secondary declaration has its own list).
Attributes select(an_attribute_ptr attrs, bool primaryOnly) {
  Attributes result;
  for (an_attribute_ptr a = attrs; a != nullptr; a = a->next) {
    if (!primaryOnly || a->on_primary_declaration) result.push_back(a);
  }
  return result;
}

an_attribute_ptr findAttribute(const Attributes& attrs, an_attribute_kind kind) {
  for (an_attribute_ptr a : attrs) {
    if (a->kind == kind) return a;
  }
  return nullptr;
}

// The first argument of an attribute as an integer (or `dflt`).
long integerArgument(an_attribute_ptr a, long dflt) {
  an_attribute_arg_ptr arg = a->arguments;
  if (arg == nullptr || arg->kind != aak_constant || arg->variant.constant == nullptr) return dflt;
  a_constant_ptr c = arg->variant.constant;
  if (c->kind != ck_integer) return dflt;
  a_boolean ovf = FALSE;
  return (long)value_of_integer_constant(c, &ovf);
}

// The first argument of an attribute as a string (e.g. section("name")).
std::string stringArgument(an_attribute_ptr a) {
  an_attribute_arg_ptr arg = a->arguments;
  if (arg == nullptr) return "";
  if (arg->kind == aak_constant && arg->variant.constant != nullptr && arg->variant.constant->kind == ck_string) {
    a_constant_ptr c = arg->variant.constant;
    size_t len = (size_t)c->variant.string.length;
    if (len > 0 && c->variant.string.value[len - 1] == '\0') len--;
    return std::string(c->variant.string.value, len);
  }
  if ((arg->kind == aak_token || arg->kind == aak_raw_token) && arg->variant.token != nullptr) {
    std::string t = arg->variant.token;
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') t = t.substr(1, t.size() - 2);
    return t;
  }
  return "";
}

// Alignment requested with __attribute__((aligned(N))) or _Alignas/alignas:
// -1 if none.
long requestedAlignment(const Attributes& attrs) {
  long result = -1;
  for (an_attribute_ptr a : attrs) {
    if (a->kind != ak_align) continue;
    // "aligned" without an argument: the largest alignment of the target
    long n = integerArgument(a, 16);
    if (n > result) result = n;
  }
  return result;
}

}  // namespace

void Translator::applyClassAttributes(SgClassDeclaration* decl, a_type_ptr type) {
  if (decl == nullptr || type == nullptr) return;
  SgTypeModifier& tm = decl->get_declarationModifier().get_typeModifier();
#if GNU_EXTENSIONS_ALLOWED
  if (type->variant.class_struct_union.is_packed) tm.setGnuAttributePacked();
  if (type->variant.class_struct_union.is_transparent) tm.setGnuAttributeTransparentUnion();
#endif
  long alignment = requestedAlignment(select(type->source_corresp.attributes, false));
  if (alignment >= 0) tm.set_gnu_attribute_alignment((short)alignment);
}

void Translator::applyVariableAttributes(SgInitializedName* in, an_attribute_ptr list, bool packed,
                                         bool primaryOnly) {
  if (in == nullptr) return;
  Attributes attrs = select(list, primaryOnly);
  if (packed) in->setGnuAttributePacked();
  long alignment = requestedAlignment(attrs);
  if (alignment >= 0) {
    an_attribute_ptr a = findAttribute(attrs, ak_align);
    if (!isCxx && a != nullptr && a->family == af_alignas) {
      // C11 _Alignas(constant-expression or type)
      SgNode* operand = nullptr;
      an_attribute_arg_ptr arg = a->arguments;
      if (arg != nullptr && arg->kind == aak_type && arg->variant.type != nullptr) {
        operand = convertType(arg->variant.type);
      } else {
        SgExpression* e = SageBuilder::buildIntVal_nfi((int)alignment, std::to_string(alignment));
        if (arg != nullptr && arg->position.seq != 0) {
          setPosition(e, arg->position);
        } else {
          setCompilerGenerated(e, true);
        }
        e->set_parent(in);
        operand = e;
      }
      in->set_using_C11_Alignas_keyword(true);
      in->set_constant_or_type_argument_for_Alignas_keyword(operand);
    } else {
      in->set_gnu_attribute_alignment((short)alignment);
    }
  }
  for (an_attribute_ptr a : attrs) {
    if (a->family != af_gnu) continue;
    switch (a->kind) {
      case ak_unused:
        in->setGnuAttributeUnused();
        break;
      case ak_used:
        in->setGnuAttributeUsed();
        break;
      case ak_weak:
        in->setGnuAttributeWeak();
        break;
      case ak_deprecated:
        in->setGnuAttributeDeprecated();
        break;
      case ak_nocommon:
        in->setGnuAttributeNoCommon();
        break;
      case ak_section:
        in->set_gnu_attribute_section_name(stringArgument(a));
        break;
      case ak_alias:
        in->set_gnu_attribute_named_alias(stringArgument(a));
        break;
      default:
        break;
    }
  }
}

void Translator::applyFunctionAttributes(SgFunctionDeclaration* decl, an_attribute_ptr list, bool primaryOnly) {
  if (decl == nullptr) return;
  SgFunctionModifier& fm = decl->get_functionModifier();
  SgTypeModifier& tm = decl->get_declarationModifier().get_typeModifier();
  for (an_attribute_ptr a : select(list, primaryOnly)) {
    if (a->kind == ak_noreturn) {
      if (a->family == af_gnu) {
        tm.setGnuAttributeNoReturn();
      } else if (!isCxx) {
        decl->set_using_C11_Noreturn_keyword(true);
      }
      continue;
    }
    if (a->family != af_gnu) continue;
    switch (a->kind) {
      case ak_always_inline:
        fm.setGnuAttributeAlwaysInline();
        break;
      case ak_noinline:
        fm.setGnuAttributeNoInline();
        break;
      case ak_pure:
        fm.setGnuAttributePure();
        break;
      case ak_malloc:
        fm.setGnuAttributeMalloc();
        break;
      case ak_naked:
        fm.setGnuAttributeNaked();
        break;
      case ak_nothrow:
        fm.setGnuAttributeNoThrow();
        break;
      case ak_no_instrument_function:
        fm.setGnuAttributeNoInstrumentFunction();
        break;
      case ak_used:
        fm.setGnuAttributeUsed();
        break;
      case ak_unused:
        fm.setGnuAttributeUnused();
        break;
      case ak_weak:
        fm.setGnuAttributeWeak();
        break;
      case ak_deprecated:
        fm.setGnuAttributeDeprecated();
        break;
      case ak_constructor:
        fm.setGnuAttributeConstructor();
        if (a->arguments != nullptr) fm.set_gnu_attribute_constructor_destructor_priority((int)integerArgument(a, 0));
        break;
      case ak_destructor:
        fm.setGnuAttributeDestructor();
        if (a->arguments != nullptr) fm.set_gnu_attribute_constructor_destructor_priority((int)integerArgument(a, 0));
        break;
      case ak_alias:
        fm.set_gnu_attribute_named_alias(stringArgument(a));
        break;
      case ak_section:
        decl->get_declarationModifier().set_gnu_attribute_section_name(stringArgument(a));
        break;
      case ak_const:
        tm.setGnuAttributeConst();
        break;
      case ak_warn_unused_result:
        tm.setGnuAttributeWarnUnusedResult();
        break;
      default:
        break;
    }
  }
}

}  // namespace edg2sage
