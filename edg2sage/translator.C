// Translator core: source positions, file names, scopes, and the top-level driver.
#include "edg2sage.h"

#include <cstdlib>
#include <fstream>
#include <unistd.h>

using namespace edg;

namespace edg2sage {

std::string absolutePath(const std::string& path) {
  std::string p = path;
  if (p.empty()) return p;
  if (p[0] != '/') {
    char buf[4096];
    if (getcwd(buf, sizeof buf) != nullptr) p = std::string(buf) + "/" + p;
  }
  // Lexically remove "." and ".." components (symbolic links are not resolved).
  std::vector<std::string> parts;
  size_t i = 0;
  while (i < p.size()) {
    size_t j = p.find('/', i);
    if (j == std::string::npos) j = p.size();
    std::string c = p.substr(i, j - i);
    if (c.empty() || c == ".") {
    } else if (c == "..") {
      if (!parts.empty()) parts.pop_back();
    } else {
      parts.push_back(c);
    }
    i = j + 1;
  }
  std::string r;
  for (const std::string& c : parts) r += "/" + c;
  return r.empty() ? "/" : r;
}

Translator::Translator(SgSourceFile* file) : sourceFile(file) {
  globalScope = file->get_globalScope();
  ROSE_ASSERT(globalScope != nullptr);
  primaryFileName = file->get_sourceFileNameWithPath();
  isCxx = il_header.source_language == sl_Cplusplus;
  scopes[il_header.primary_scope] = globalScope;
}

// ---------------------------------------------------------------------------------
// File names and source positions
// ---------------------------------------------------------------------------------

std::string Translator::fileNameOf(a_seq_number seq, a_line_number* line) {
  a_const_char *file_name = nullptr, *full_name = nullptr;
  a_line_number ln = 0;
  a_boolean at_end = FALSE;
  a_source_file_ptr sf = conv_seq_to_file_and_line(seq, &file_name, &full_name, &ln, &at_end);
  if (line) *line = ln;
  if (sf == nullptr) return "";
  auto it = fileNames.find(sf);
  if (it != fileNames.end()) return it->second;
  std::string name;
  if (full_name != nullptr && full_name[0] != '\0') {
    name = full_name;
  } else if (file_name != nullptr) {
    name = file_name;
  }
  if (!name.empty()) {
    name = absolutePath(name);
    if (!sf->is_include_file && sf->top_level_file) {
      // The primary source file: use exactly the name ROSE knows it by.
      std::string base = absolutePath(primaryFileName);
      if (base == name) name = primaryFileName;
    }
  }
  fileNames[sf] = name;
  return name;
}

Sg_File_Info* Translator::compilerGeneratedFileInfo(bool output) {
  Sg_File_Info* fi = Sg_File_Info::generateDefaultFileInfoForCompilerGeneratedNode();
  fi->setCompilerGenerated();
  if (output) fi->setOutputInCodeGeneration();
  return fi;
}

Sg_File_Info* Translator::fileInfo(const a_source_position& pos) {
  if (pos.seq == 0) return compilerGeneratedFileInfo();
  a_line_number line = 0;
  std::string name = fileNameOf(pos.seq, &line);
  if (name.empty() || line == 0) return compilerGeneratedFileInfo();
  int column = (pos.column == SP_COL_UNKNOWN) ? 0 : (int)pos.column;
  return new Sg_File_Info(name, (int)line, column);
}

bool Translator::isFromSourceFile(const a_source_position& pos) {
  if (pos.seq == 0) return false;
  return fileNameOf(pos.seq) == primaryFileName;
}

void Translator::setPosition(SgLocatedNode* node, const a_source_position& start, const a_source_position& end) {
  if (node == nullptr) return;
  Sg_File_Info* s = fileInfo(start);
  Sg_File_Info* e = fileInfo(end.seq != 0 ? end : start);
  delete node->get_startOfConstruct();
  delete node->get_endOfConstruct();
  node->set_startOfConstruct(s);
  node->set_endOfConstruct(e);
  s->set_parent(node);
  e->set_parent(node);
  if (SgExpression* expr = isSgExpression(node)) {
    Sg_File_Info* o = fileInfo(start);
    delete expr->get_operatorPosition();
    expr->set_operatorPosition(o);
    o->set_parent(node);
  }
}

void Translator::setCompilerGenerated(SgLocatedNode* node, bool output) {
  if (node == nullptr) return;
  Sg_File_Info* s = compilerGeneratedFileInfo(output);
  Sg_File_Info* e = compilerGeneratedFileInfo(output);
  delete node->get_startOfConstruct();
  delete node->get_endOfConstruct();
  node->set_startOfConstruct(s);
  node->set_endOfConstruct(e);
  s->set_parent(node);
  e->set_parent(node);
  if (SgExpression* expr = isSgExpression(node)) {
    Sg_File_Info* o = compilerGeneratedFileInfo(output);
    delete expr->get_operatorPosition();
    expr->set_operatorPosition(o);
    o->set_parent(node);
  }
}

namespace {
// Visits every located node in the memory pools: nodes that are only reachable
// through symbols (e.g. hidden first nondefining declarations) need positions too.
class MissingPositions : public ROSE_VisitTraversal {
 public:
  explicit MissingPositions(Translator* t) : t(t) {}
  void visit(SgNode* n) override {
    SgLocatedNode* ln = isSgLocatedNode(n);
    if (ln == nullptr) return;
    if (ln->get_startOfConstruct() == nullptr || ln->get_endOfConstruct() == nullptr) {
      Sg_File_Info* s = ln->get_startOfConstruct();
      Sg_File_Info* e = ln->get_endOfConstruct();
      if (s == nullptr && e != nullptr) {
        s = new Sg_File_Info(*e);
        ln->set_startOfConstruct(s);
        s->set_parent(ln);
      } else if (s != nullptr && e == nullptr) {
        e = new Sg_File_Info(*s);
        ln->set_endOfConstruct(e);
        e->set_parent(ln);
      } else {
        t->setCompilerGenerated(ln);
      }
    }
    if (SgExpression* ex = isSgExpression(ln)) {
      if (ex->get_operatorPosition() == nullptr) {
        Sg_File_Info* o = new Sg_File_Info(*ex->get_startOfConstruct());
        ex->set_operatorPosition(o);
        o->set_parent(ex);
      }
    }
  }
  Translator* t;
};
}  // namespace

void Translator::fillMissingPositions(SgNode*) {
  MissingPositions mp(this);
  mp.traverseMemoryPool();
}

std::string Translator::sourceText(const a_source_position& start, const a_source_position& end) {
  if (start.seq == 0 || end.seq == 0 || start.seq != end.seq) return "";
  if (start.column == SP_COL_UNKNOWN || end.column == SP_COL_UNKNOWN || end.column < start.column) return "";
  a_line_number line = 0;
  std::string name = fileNameOf(start.seq, &line);
  if (name.empty() || line == 0) return "";
  auto it = sourceLines.find(name);
  if (it == sourceLines.end()) {
    std::vector<std::string> lines;
    std::ifstream in(name);
    std::string l;
    while (std::getline(in, l)) lines.push_back(l);
    it = sourceLines.emplace(name, std::move(lines)).first;
  }
  const std::vector<std::string>& lines = it->second;
  if (line > lines.size()) return "";
  const std::string& text = lines[line - 1];
  // EDG columns count characters (a multibyte character counts as one column);
  // only use the text when the line is plain ASCII.
  for (unsigned char c : text) {
    if (c >= 0x80) return "";
  }
  size_t b = start.column - 1, e = end.column;  // end position is the last character
  if (e > text.size() || b >= e) return "";
  return text.substr(b, e - b);
}

// ---------------------------------------------------------------------------------
// Scopes
// ---------------------------------------------------------------------------------

SgScopeStatement* Translator::scopeFor(a_scope_ptr scope) {
  if (scope == nullptr) return nullptr;
  auto it = scopes.find(scope);
  if (it != scopes.end()) return it->second;
  switch (scope->kind) {
    case sck_file:
      return globalScope;
    case sck_class_struct_union:
      if (scope->variant.assoc_type != nullptr) {
        SgClassDefinition* def = classDefinitionFor(scope->variant.assoc_type);
        if (def != nullptr) {
          scopes[scope] = def;
          return def;
        }
      }
      break;
    case sck_namespace:
      if (scope->variant.assoc_namespace != nullptr) {
        SgNamespaceDeclarationStatement* nd = namespaceDeclarationFor(scope->variant.assoc_namespace);
        if (nd != nullptr && nd->get_definition() != nullptr) return nd->get_definition();
      }
      break;
    case sck_function:
      if (scope->variant.routine.ptr != nullptr) {
        auto d = definingRoutineDecl.find(scope->variant.routine.ptr);
        if (d != definingRoutineDecl.end() && d->second->get_definition() != nullptr) {
          return d->second->get_definition();
        }
      }
      break;
    default:
      break;
  }
  return nullptr;
}

SgScopeStatement* Translator::parentScopeOf(a_source_correspondence* scp, SgScopeStatement* fallback) {
  if (scp != nullptr && scp->parent_scope != nullptr) {
    SgScopeStatement* s = scopeFor(scp->parent_scope);
    if (s != nullptr) return s;
  }
  if (fallback != nullptr) return fallback;
  return currentScope();
}

SgName Translator::nameOf(a_source_correspondence* scp) {
  if (scp == nullptr || scp->name == nullptr) return SgName("");
  return SgName(scp->name);
}

// ---------------------------------------------------------------------------------
// Debugging: reports deleted nodes and wrong parent pointers in the AST.
// ---------------------------------------------------------------------------------

namespace {
// Named types must refer to the first nondefining declaration of their entity.
class TypeDeclarationCheck : public ROSE_VisitTraversal {
 public:
  void visit(SgNode* n) override {
    SgNamedType* t = isSgNamedType(n);
    if (t == nullptr || t->get_declaration() == nullptr) return;
    SgDeclarationStatement* d = t->get_declaration();
    if (d != d->get_firstNondefiningDeclaration() && !isSgEnumDeclaration(d)) {
      mlog[Sawyer::Message::ERROR] << t->class_name() << " " << t->get_name().getString()
                                   << " refers to a declaration that is not the first nondefining one ("
                                   << d->class_name() << ")\n";
    }
  }
};
}  // namespace

namespace {
// Declarations must have a first nondefining declaration.
class FirstDeclarationCheck : public ROSE_VisitTraversal {
 public:
  void visit(SgNode* n) override {
    SgDeclarationStatement* d = isSgDeclarationStatement(n);
    if (d == nullptr || d->get_firstNondefiningDeclaration() != nullptr) return;
    if (isSgFunctionParameterList(d) || isSgCtorInitializerList(d) || isSgVariableDefinition(d)) return;
    Sg_File_Info* fi = d->get_startOfConstruct();
    mlog[Sawyer::Message::ERROR] << d->class_name() << " without first nondefining declaration at "
                                 << (fi ? fi->get_filenameString() + ":" + std::to_string(fi->get_line()) : "?")
                                 << " parent " << (d->get_parent() ? d->get_parent()->class_name() : "null") << "\n";
  }
};
}  // namespace

void Translator::checkTree(SgNode* node, int depth) {
  if (depth == 0) {
    TypeDeclarationCheck check;
    check.traverseMemoryPool();
    FirstDeclarationCheck first;
    first.traverseMemoryPool();
  }
  if (node == nullptr || depth > 2000) return;
  std::vector<SgNode*> children = node->get_traversalSuccessorContainer();
  for (size_t i = 0; i < children.size(); i++) {
    SgNode* c = children[i];
    if (c == nullptr) continue;
    if (c->variantT() == V_SgNode) {
      mlog[Sawyer::Message::ERROR] << "deleted node as child " << i << " of " << node->class_name() << " "
                                   << node->unparseToString() << "\n";
      continue;
    }
    if (c->get_parent() != node) {
      mlog[Sawyer::Message::WARN] << c->class_name() << " (child " << i << " of " << node->class_name()
                                  << ") has parent " << (c->get_parent() ? c->get_parent()->class_name() : "null")
                                  << "\n";
    }
    checkTree(c, depth + 1);
  }
}

// ---------------------------------------------------------------------------------
// Driver
// ---------------------------------------------------------------------------------

void Translator::translate() {
  SageBuilder::SourcePositionClassification savedMode = SageBuilder::getSourcePositionClassificationMode();
  SageBuilder::setSourcePositionClassificationMode(SageBuilder::e_sourcePositionNullPointers);
  SageBuilder::pushScopeStack(globalScope);
  scopeStack.push_back(globalScope);

  SeqCursor cursor(il_header.primary_scope->source_sequence_list);
  translateDeclarationList(cursor, globalScope, nullptr);
  finishDeferredFunctionBodies();

  scopeStack.pop_back();
  SageBuilder::popScopeStack();
  SageBuilder::setSourcePositionClassificationMode(savedMode);

  fillMissingPositions(globalScope);
  if (std::getenv("EDG2SAGE_DEBUG") != nullptr) checkTree(globalScope, 0);
  if (warnings > 0 && SgProject::get_verbose() > 0) {
    mlog[Sawyer::Message::WARN] << warnings << " IL constructs could not be translated\n";
  }
}

}  // namespace edg2sage
