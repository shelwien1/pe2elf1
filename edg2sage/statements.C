// Translation of statements.
#include "edg2sage.h"

using namespace edg;
using namespace Sawyer::Message;

namespace edg2sage {

namespace {
a_source_position endOf(a_statement_ptr stmt) {
#if EXTRA_SOURCE_POSITIONS_IN_IL
  if (stmt->end_position.seq != 0) return stmt->end_position;
#endif
  return stmt->position;
}
}  // namespace

// A block that is not written in the source (the body of a loop or branch that
// is a single statement, or an implicit C99/C++ scope) gets the position of its
// statements, so that it is still unparsed.
static void positionImplicitBlock(Translator* t, SgBasicBlock* block) {
  SgStatementPtrList& stmts = block->get_statements();
  SgStatement* first = nullptr;
  SgStatement* last = nullptr;
  for (SgStatement* s : stmts) {
    if (s->get_startOfConstruct() == nullptr || s->get_startOfConstruct()->isCompilerGenerated()) continue;
    if (first == nullptr) first = s;
    last = s;
  }
  if (first == nullptr) {
    t->setCompilerGenerated(block, true);
    return;
  }
  Sg_File_Info* st = new Sg_File_Info(*first->get_startOfConstruct());
  Sg_File_Info* en = new Sg_File_Info(*(last->get_endOfConstruct() ? last->get_endOfConstruct()
                                                                    : last->get_startOfConstruct()));
  delete block->get_startOfConstruct();
  delete block->get_endOfConstruct();
  block->set_startOfConstruct(st);
  block->set_endOfConstruct(en);
  st->set_parent(block);
  en->set_parent(block);
}

SgBasicBlock* Translator::convertBlock(a_statement_ptr stmt, SgBasicBlock* block) {
  if (block == nullptr) block = SageBuilder::buildBasicBlock_nfi();
  // A provisional parent (replaced when the block is attached): declarations
  // inside need a complete scope chain (e.g. for mangled names).
  if (block->get_parent() == nullptr) block->set_parent(currentScope());
  if (stmt == nullptr) {
    setCompilerGenerated(block, true);
    return block;
  }
  if (stmt->kind != stmk_block) {
    // A single statement where a block is required.
    scopeStack.push_back(block);
    SageBuilder::pushScopeStack(block);
    convertStatementListInto(stmt, block, /*single=*/true);
    SageBuilder::popScopeStack();
    scopeStack.pop_back();
    positionImplicitBlock(this, block);
    return block;
  }
  a_block_ptr bi = stmt->variant.block.extra_info;
  if (bi != nullptr && bi->assoc_scope != nullptr) scopes[bi->assoc_scope] = block;
  scopeStack.push_back(block);
  SageBuilder::pushScopeStack(block);
  convertStatementListInto(stmt->variant.block.statements, block);
  SageBuilder::popScopeStack();
  scopeStack.pop_back();
  if (stmt->compiler_generated) {
    positionImplicitBlock(this, block);
  } else {
    setPosition(block, stmt->position, bi ? bi->final_position : endOf(stmt));
  }
  return block;
}

void Translator::attachDeferredInitializer(a_statement_ptr initStmt) {
  a_dynamic_init_ptr dip = initStmt->variant.dynamic_init;
  if (dip == nullptr || dip->variable == nullptr) return;
  auto it = variables.find(dip->variable);
  if (it == variables.end()) return;
  SgInitializedName* in = it->second;
  if (in->get_initptr() != nullptr) return;
  SgInitializer* init = convertDynamicInit(dip, in->get_type());
  if (init == nullptr) return;
  in->set_initptr(init);
  init->set_parent(in);
}

void Translator::convertStatementListInto(a_statement_ptr first, SgScopeStatement* scope, bool single) {
  for (a_statement_ptr s = first; s != nullptr; s = single ? nullptr : s->next) {
    try {
      if (s->kind == stmk_decl) {
        translateDeclarationStatement(s, scope);
        continue;
      }
      if (s->kind == stmk_init) {
        // The initialization of a variable declared after executable statements
        // (normally already attached to the declaration).
        attachDeferredInitializer(s);
        continue;
      }
      if (s->kind == stmk_vla_decl || s->kind == stmk_set_vla_size) continue;
      if (s->kind == stmk_block && s->compiler_generated) {
        // An implicit scope (e.g. around a C99 for statement): its statements
        // belong to the enclosing statement list.
        a_block_ptr bi = s->variant.block.extra_info;
        if (bi != nullptr && bi->assoc_scope != nullptr && scopes.count(bi->assoc_scope) == 0) {
          scopes[bi->assoc_scope] = scope;
        }
        convertStatementListInto(s->variant.block.statements, scope);
        continue;
      }
      SgStatement* st = convertStatement(s);
      if (st != nullptr) appendStatementTo(scope, st);
    } catch (const Unsupported& u) {
      warnings++;
      mlog[WARN] << "skipping statement (" << u.what << ")\n";
      SgStatement* st = SageBuilder::buildNullStatement_nfi();
      setPosition(st, s->position);
      appendStatementTo(scope, st);
    }
  }
}

// The statement that is the body of a loop or the branch of an if.
static bool isCompilerGeneratedBlock(a_statement_ptr s) {
  return s != nullptr && s->kind == stmk_block && s->compiler_generated;
}

SgLabelStatement* Translator::labelStatementFor(a_label_ptr label) {
  auto it = labels.find(label);
  if (it != labels.end()) return it->second;
  SgLabelStatement* ls = new SgLabelStatement(nameOf(&label->source_corresp), nullptr);
  ls->set_scope(currentFunctionDefinition);
  labels[label] = ls;
  if (currentFunctionDefinition != nullptr) {
    SgLabelSymbol* sym = new SgLabelSymbol(ls);
    currentFunctionDefinition->insert_symbol(ls->get_label(), sym);
    labelSymbols[label] = sym;
  }
  return ls;
}

SgStatement* Translator::convertStatement(a_statement_ptr stmt) {
  if (stmt == nullptr) return nullptr;
  SgStatement* result = convertStatementKind(stmt);
  return result;
}

SgStatement* Translator::convertStatementKind(a_statement_ptr stmt) {
  switch (stmt->kind) {
    case stmk_expr: {
      SgExpression* e = convertExpression(stmt->expr);
      SgExprStatement* s = SageBuilder::buildExprStatement_nfi(e);
      setPosition(s, stmt->position, endOf(stmt));
      return s;
    }
    case stmk_empty: {
      SgStatement* s = SageBuilder::buildNullStatement_nfi();
      setPosition(s, stmt->position, endOf(stmt));
      return s;
    }
    case stmk_block:
      return convertBlock(stmt);
    case stmk_if:
    case stmk_constexpr_if:
      return convertIfStatement(stmt);
    case stmk_while: {
      SgWhileStmt* w = new SgWhileStmt((SgStatement*)nullptr, (SgStatement*)nullptr);
      w->set_parent(currentScope());
      scopeStack.push_back(w);
      SgStatement* cond = convertCondition(stmt->expr, w);
      w->set_condition(cond);
      cond->set_parent(w);
      SgStatement* body = convertBlock(stmt->variant.loop_statement);
      w->set_body(body);
      body->set_parent(w);
      scopeStack.pop_back();
      setPosition(w, stmt->position, endOf(stmt));
      return w;
    }
    case stmk_end_test_while: {
      SgStatement* body = convertBlock(stmt->variant.loop_statement);
      SgExpression* c = convertExpression(stmt->expr);
      SgExprStatement* cond = SageBuilder::buildExprStatement_nfi(c);
      if (!copyPosition(cond, c)) setPosition(cond, stmt->position);
      SgDoWhileStmt* d = new SgDoWhileStmt(body, cond);
      body->set_parent(d);
      cond->set_parent(d);
      setPosition(d, stmt->position, endOf(stmt));
      return d;
    }
    case stmk_for:
      return convertForStatement(stmt);
    case stmk_range_based_for:
      return convertRangeBasedForStatement(stmt);
    case stmk_switch:
      return convertSwitchStatement(stmt);
    case stmk_switch_case: {
      a_switch_case_entry_ptr ce = stmt->variant.switch_case.extra_info;
      SgBasicBlock* body = SageBuilder::buildBasicBlock_nfi();
      SgStatement* result = nullptr;
      if (ce == nullptr || ce->case_value == nullptr) {
        SgDefaultOptionStmt* d = new SgDefaultOptionStmt(body);
        result = d;
      } else {
        SgExpression* key = convertConstant(ce->case_value);
        SgCaseOptionStmt* c = new SgCaseOptionStmt(key, body);
        key->set_parent(c);
#if GNU_EXTENSIONS_ALLOWED
        if (ce->range_end != nullptr) {
          SgExpression* end = convertConstant(ce->range_end);
          c->set_key_range_end(end);
          end->set_parent(c);
        }
#endif
        result = c;
      }
      body->set_parent(result);
      setPosition(result, stmt->position, endOf(stmt));
      setPosition(body, stmt->position, endOf(stmt));
      return result;
    }
    case stmk_goto: {
      a_label_ptr label = stmt->variant.label.ptr;
      SgStatement* s = nullptr;
      if (label->break_label) {
        s = SageBuilder::buildBreakStmt_nfi();
      } else if (label->continue_label) {
        s = SageBuilder::buildContinueStmt_nfi();
      } else {
        s = new SgGotoStatement(labelStatementFor(label));
      }
      setPosition(s, stmt->position, endOf(stmt));
      return s;
    }
#if GNU_EXTENSIONS_ALLOWED
    case stmk_assigned_goto: {
      SgExpression* target = convertExpression(stmt->expr);
      // GNU computed goto: "goto *expr;"
      SgGotoStatement* g = new SgGotoStatement((SgLabelStatement*)nullptr);
      g->set_selector_expression(target);
      target->set_parent(g);
      setPosition(g, stmt->position, endOf(stmt));
      return g;
    }
#endif
    case stmk_label: {
      a_label_ptr label = stmt->variant.label.ptr;
      if (label->break_label || label->continue_label) return nullptr;  // implicit targets of break/continue
      SgLabelStatement* ls = labelStatementFor(label);
      setPosition(ls, stmt->position, endOf(stmt));
      return ls;
    }
    case stmk_return: {
      SgExpression* e = nullptr;
      if (stmt->expr != nullptr) {
        e = convertExpression(stmt->expr);
      } else if (stmt->variant.return_dynamic_init != nullptr) {
        e = initializerExpression(convertDynamicInit(stmt->variant.return_dynamic_init, nullptr));
      }
      if (stmt->compiler_generated && e == nullptr) return nullptr;  // implicit return at the end
      if (e == nullptr) {
        // "return;" (ROSE uses a null expression)
        e = SageBuilder::buildNullExpression_nfi();
        setPosition(e, stmt->position);
      }
      SgReturnStmt* r = SageBuilder::buildReturnStmt_nfi(e);
      if (e) e->set_parent(r);
      setPosition(r, stmt->position, endOf(stmt));
      return r;
    }
    case stmk_asm:
      return convertAsmStatement(stmt->variant.asm_entry, stmt->position);
    case stmk_stmt_expr_result: {
      SgExpression* e = nullptr;
      if (stmt->expr != nullptr) {
        e = convertExpression(stmt->expr);
      } else if (stmt->variant.stmt_expr_result.dynamic_init != nullptr) {
        e = initializerExpression(convertDynamicInit(stmt->variant.stmt_expr_result.dynamic_init, nullptr));
      } else {
        e = SageBuilder::buildNullExpression_nfi();
      }
      SgExprStatement* s = SageBuilder::buildExprStatement_nfi(e);
      setPosition(s, stmt->position, endOf(stmt));
      return s;
    }
    case stmk_try_block:
      return convertTryStatement(stmt);
    default:
      throw Unsupported("statement kind " + std::to_string((int)stmt->kind));
  }
}

SgStatement* Translator::convertCondition(an_expr_node_ptr expr, SgScopeStatement* scope) {
  if (expr != nullptr && expr->kind == enk_condition) {
    // A C++ condition declaration ("if (T x = ...)")
    a_condition_supplement_ptr cs = expr->variant.condition;
    if (cs->scope != nullptr) scopes[cs->scope] = scope;
    a_variable_ptr var = nullptr;
    if (cs->dynamic_init != nullptr) var = cs->dynamic_init->variable;
    if (var == nullptr && cs->scope != nullptr) var = cs->scope->variables;
    if (var != nullptr) {
      SgDeclarationStatement* d = translateVariable(var, nullptr, scope);
      return d;
    }
    expr = cs->expr;
  }
  SgExpression* e = convertExpression(expr);
  SgExprStatement* s = SageBuilder::buildExprStatement_nfi(e);
  // The condition is written where its expression is (implicit conversions
  // of the expression are compiler generated, the statement is not).
  if (!copyPosition(s, e)) {
    if (expr != nullptr && expr->position.seq != 0) {
      setPosition(s, expr->position);
    } else {
      setCompilerGenerated(s, true);
    }
  }
  return s;
}

SgStatement* Translator::convertIfStatement(a_statement_ptr stmt) {
  a_statement_ptr thenStmt = nullptr, elseStmt = nullptr;
  if (stmt->kind == stmk_constexpr_if) {
    thenStmt = stmt->variant.constexpr_if->then_statement;
    elseStmt = stmt->variant.constexpr_if->else_statement;
  } else {
    thenStmt = stmt->variant.if_stmt.then_statement;
    elseStmt = stmt->variant.if_stmt.else_statement;
  }
  SgIfStmt* ifs = new SgIfStmt((SgStatement*)nullptr, (SgStatement*)nullptr, (SgStatement*)nullptr);
  ifs->set_parent(currentScope());
  if (stmt->kind == stmk_constexpr_if) ifs->set_is_if_constexpr_statement(true);
  scopeStack.push_back(ifs);
  SgStatement* cond = convertCondition(stmt->expr, ifs);
  ifs->set_conditional(cond);
  cond->set_parent(ifs);
  SgStatement* t = convertBlock(thenStmt);
  ifs->set_true_body(t);
  t->set_parent(ifs);
  if (elseStmt != nullptr) {
    SgStatement* f = nullptr;
    if (elseStmt->kind == stmk_if && !isCompilerGeneratedBlock(elseStmt)) {
      // "else if": keep the nested if as the false body
      f = convertStatement(elseStmt);
    } else if (isCompilerGeneratedBlock(elseStmt) && elseStmt->variant.block.statements != nullptr &&
               elseStmt->variant.block.statements->next == nullptr &&
               elseStmt->variant.block.statements->kind == stmk_if &&
               (elseStmt->variant.block.extra_info == nullptr ||
                elseStmt->variant.block.extra_info->assoc_scope == nullptr)) {
      f = convertStatement(elseStmt->variant.block.statements);
    } else {
      f = convertBlock(elseStmt);
    }
    ifs->set_false_body(f);
    f->set_parent(ifs);
  }
  scopeStack.pop_back();
  setPosition(ifs, stmt->position, endOf(stmt));
  return ifs;
}

SgStatement* Translator::convertForStatement(a_statement_ptr stmt) {
  a_for_loop_ptr fl = stmt->variant.for_loop.extra_info;
  SgForStatement* f = new SgForStatement((SgStatement*)nullptr, (SgExpression*)nullptr, (SgStatement*)nullptr);
  f->set_parent(currentScope());
  SgForInitStatement* init = new SgForInitStatement();
  f->set_for_init_stmt(init);
  init->set_parent(f);
  if (fl != nullptr && fl->for_init_scope != nullptr) scopes[fl->for_init_scope] = f;
  scopeStack.push_back(f);
  SageBuilder::pushScopeStack(f);
  if (fl != nullptr && fl->initialization != nullptr) {
    // An expression statement, a declaration statement, or a (compiler
    // generated) block with declaration and initialization statements.
    // Declarations are scoped in the for statement; appendStatementTo()
    // puts them into its init statement list.
    a_statement_ptr is = fl->initialization;
    a_statement_ptr parts = is->kind == stmk_block ? is->variant.block.statements : is;
    for (a_statement_ptr p = parts; p != nullptr; p = is->kind == stmk_block ? p->next : nullptr) {
      if (p->kind == stmk_decl) {
        translateDeclarationStatement(p, f);
      } else if (p->kind == stmk_init) {
        attachDeferredInitializer(p);
      } else if (p->kind != stmk_vla_decl && p->kind != stmk_set_vla_size) {
        SgStatement* st = convertStatement(p);
        if (st != nullptr) {
          init->append_init_stmt(st);
          st->set_parent(init);
        }
      }
    }
  }
  SgStatement* test = nullptr;
  if (stmt->expr != nullptr) {
    test = convertCondition(stmt->expr, f);
  } else {
    // "for (init; ; incr)": the null statement prints the ';'
    test = SageBuilder::buildNullStatement_nfi();
    setPosition(test, stmt->position);
  }
  f->set_test(test);
  test->set_parent(f);
  SgExpression* incr = nullptr;
  if (fl != nullptr && fl->increment != nullptr) {
    incr = convertExpression(fl->increment);
  } else {
    incr = SageBuilder::buildNullExpression_nfi();
    setCompilerGenerated(incr);
  }
  f->set_increment(incr);
  incr->set_parent(f);
  SgStatement* body = convertBlock(stmt->variant.for_loop.statement);
  f->set_loop_body(body);
  body->set_parent(f);
  SageBuilder::popScopeStack();
  scopeStack.pop_back();
  setPosition(f, stmt->position, endOf(stmt));
  setPosition(init, stmt->position);
  return f;
}

// "for (T x : range) body": EDG represents the loop rewritten with the
// variables __range, __begin and __end; ROSE keeps the declaration of x and
// the declaration of __range, whose initializer is the range expression.
SgStatement* Translator::convertRangeBasedForStatement(a_statement_ptr stmt) {
  a_range_based_for_loop_ptr rf = stmt->variant.range_based_for_loop.extra_info;
  if (rf == nullptr || rf->iterator == nullptr || rf->range == nullptr) throw Unsupported("range-based for");
  if (rf->initialization != nullptr) throw Unsupported("range-based for with an init-statement");
  SgRangeBasedForStatement* f = new SgRangeBasedForStatement((SgVariableDeclaration*)nullptr, nullptr, nullptr, nullptr,
                                                              nullptr, nullptr, nullptr);
  f->set_parent(currentScope());
  if (rf->range_based_for_scope != nullptr) scopes[rf->range_based_for_scope] = f;
  if (rf->iterator_scope != nullptr) scopes[rf->iterator_scope] = f;
  scopeStack.push_back(f);
  SageBuilder::pushScopeStack(f);
  SgDeclarationStatement* range = translateVariable(rf->range, nullptr, f);
  // The iteration variable is initialized with "*__begin", which is not written.
  bool saved = suppressInitializers;
  suppressInitializers = true;
  SgDeclarationStatement* iter = nullptr;
  try {
    iter = translateVariable(rf->iterator, nullptr, f);
  } catch (...) {
    suppressInitializers = saved;
    SageBuilder::popScopeStack();
    scopeStack.pop_back();
    throw;
  }
  suppressInitializers = saved;
  f->set_range_declaration(isSgVariableDeclaration(range));
  f->set_iterator_declaration(isSgVariableDeclaration(iter));
  if (range != nullptr) range->set_parent(f);
  if (iter != nullptr) iter->set_parent(f);
  // The body: a compiler-generated block around the statement as written.
  a_statement_ptr bodyStmt = stmt->variant.range_based_for_loop.statement;
  if (bodyStmt != nullptr && bodyStmt->kind == stmk_block && bodyStmt->compiler_generated &&
      bodyStmt->variant.block.statements != nullptr && bodyStmt->variant.block.statements->next == nullptr &&
      bodyStmt->variant.block.statements->kind == stmk_block &&
      !bodyStmt->variant.block.statements->compiler_generated) {
    bodyStmt = bodyStmt->variant.block.statements;
  }
  SgStatement* body = convertBlock(bodyStmt);
  f->set_loop_body(body);
  body->set_parent(f);
  SageBuilder::popScopeStack();
  scopeStack.pop_back();
  if (range == nullptr || iter == nullptr) throw Unsupported("range-based for");
  setPosition(f, stmt->position, endOf(stmt));
  return f;
}

// Switch bodies: EDG has case labels as statements of the body block; ROSE
// wants each case's statements inside the case option statement.
void Translator::restructureSwitchBody(SgBasicBlock* body) {
  SgStatementPtrList& stmts = body->get_statements();
  SgStatementPtrList result;
  SgBasicBlock* current = nullptr;
  for (SgStatement* s : stmts) {
    if (SgCaseOptionStmt* c = isSgCaseOptionStmt(s)) {
      result.push_back(c);
      current = isSgBasicBlock(c->get_body());
    } else if (SgDefaultOptionStmt* d = isSgDefaultOptionStmt(s)) {
      result.push_back(d);
      current = isSgBasicBlock(d->get_body());
    } else if (current != nullptr) {
      current->append_statement(s);
      s->set_parent(current);
    } else {
      result.push_back(s);
    }
  }
  stmts = result;
}

SgStatement* Translator::convertSwitchStatement(a_statement_ptr stmt) {
  SgSwitchStatement* sw = new SgSwitchStatement((SgStatement*)nullptr, (SgStatement*)nullptr);
  sw->set_parent(currentScope());
  scopeStack.push_back(sw);
  SgStatement* sel = convertCondition(stmt->expr, sw);
  sw->set_item_selector(sel);
  sel->set_parent(sw);
  SgBasicBlock* body = convertBlock(stmt->variant.switch_stmt.body_statement);
  restructureSwitchBody(body);
  sw->set_body(body);
  body->set_parent(sw);
  scopeStack.pop_back();
  setPosition(sw, stmt->position, endOf(stmt));
  return sw;
}

SgStatement* Translator::convertTryStatement(a_statement_ptr stmt) {
  a_try_supplement_ptr ts = stmt->variant.try_block;
  SgBasicBlock* body = convertBlock(ts->statement);
  SgTryStmt* t = new SgTryStmt(body);
  body->set_parent(t);
  SgCatchStatementSeq* seq = t->get_catch_statement_seq_root();
  if (seq == nullptr) {
    seq = new SgCatchStatementSeq();
    t->set_catch_statement_seq_root(seq);
    seq->set_parent(t);
  }
  for (a_handler_ptr h = ts->handlers; h != nullptr; h = h->next) {
    SgCatchOptionStmt* c = new SgCatchOptionStmt(nullptr, nullptr, t);
    c->set_parent(seq);
    scopeStack.push_back(c);
    SgVariableDeclaration* param = nullptr;
    if (h->parameter != nullptr) {
      param = isSgVariableDeclaration(translateVariable(h->parameter, nullptr, c));
    } else {
      // catch (...)
      SgInitializedName* in = SageBuilder::buildInitializedName_nfi(SgName(""), SgTypeEllipse::createType(), nullptr);
      param = new SgVariableDeclaration(in);
      param->set_firstNondefiningDeclaration(param);
      in->set_scope(c);
      setPosition(param, h->catch_position);  // printed as "..."
      setPosition(in, h->catch_position);
    }
    c->set_condition(param);
    param->set_parent(c);
    SgBasicBlock* hb = convertBlock(h->statement);
    c->set_body(hb);
    hb->set_parent(c);
    scopeStack.pop_back();
    setPosition(c, h->catch_position, endOf(h->statement));
    seq->append_catch_statement(c);
    c->set_parent(seq);
  }
  setPosition(seq, stmt->position, endOf(stmt));
  setPosition(t, stmt->position, endOf(stmt));
  return t;
}

SgStatement* Translator::convertAsmStatement(an_asm_entry_ptr ae, const a_source_position& pos) {
  std::string text;
  if (ae->asm_string != nullptr && ae->asm_string->kind == ck_string) {
    size_t len = ae->asm_string->variant.string.length;
    if (len > 0) len--;  // trailing NUL
    text.assign(ae->asm_string->variant.string.value, len);
  }
  SgAsmStmt* a = new SgAsmStmt();
  a->set_firstNondefiningDeclaration(a);  // SgAsmStmt is a declaration statement
  a->set_assemblyCode(text);
#if GNU_EXTENSIONS_ALLOWED
  a->set_isVolatile(ae->has_volatile_keyword);
  a->set_useGnuExtendedFormat(ae->gnu_asm_form || ae->operands != nullptr || ae->clobbers != nullptr);
  for (an_asm_operand_ptr op = ae->operands; op != nullptr; op = op->next) {
    if (op->expression == nullptr) continue;
    SgExpression* e = convertExpression(op->expression);
    SgAsmOp* aop = new SgAsmOp(SgAsmOp::e_invalid, SgAsmOp::e_unknown, e);
    e->set_parent(aop);
    // The constraint strings are recorded as written (RECORD_RAW_ASM_OPERAND_DESCRIPTIONS).
    aop->set_recordRawAsmOperandDescriptions(true);
    aop->set_isOutputOperand(op->is_output_operand);
    if (op->constraints_string != nullptr) aop->set_constraintString(op->constraints_string);
    if (op->name != nullptr) aop->set_name(op->name);
    setPosition(aop, op->position.seq != 0 ? op->position : pos);
    a->get_operands().push_back(aop);
    aop->set_parent(a);
  }
  for (a_named_register_list_ptr c = ae->clobbers; c != nullptr; c = c->next) {
    // EDG's named registers and ROSE's register names are the same enumeration.
    int reg = (int)c->reg;
    if (reg <= 0 || reg >= (int)SgInitializedName::e_last_register) reg = SgInitializedName::e_unrecognized_register;
    a->get_clobberRegisterList().push_back((SgInitializedName::asm_register_name_enum)reg);
  }
#endif
  setPosition(a, pos);
  return a;
}

}  // namespace edg2sage
