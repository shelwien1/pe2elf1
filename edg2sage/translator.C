// Translator core: source positions, file names, scopes, and the top-level driver.
#include "edg2sage.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <unistd.h>

using namespace edg;

namespace edg2sage {

#ifdef _WIN32
// Windows: _fullpath handles drive letters and both kinds of separators, and resolves "."
// and ".." (the result has backslashes).
std::string absolutePath(const std::string& path) {
  char buf[4096];
  if (path.empty() || _fullpath(buf, path.c_str(), sizeof buf) == nullptr) return path;
  return buf;
}

// File names are not case sensitive on Windows
static bool samePath(const std::string& a, const std::string& b) {
  return a.size() == b.size() && _stricmp(a.c_str(), b.c_str()) == 0;
}
#else
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

static bool samePath(const std::string& a, const std::string& b) { return a == b; }
#endif

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
      if (samePath(base, name)) name = primaryFileName;
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

bool Translator::inSystemHeader(const a_source_position& pos) {
  return pos.seq != 0 && seq_is_in_system_header(pos.seq);
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

void Translator::fillMissingPositions(SgNode* root) {
  MissingPositions mp(this);
  if (root != nullptr) {
    // Only the subtree (e.g. an expression that is unparsed during the
    // translation to name a template instance)
    std::vector<SgNode*> work(1, root);
    while (!work.empty()) {
      SgNode* n = work.back();
      work.pop_back();
      mp.visit(n);
      for (SgNode* c : n->get_traversalSuccessorContainer()) {
        if (c != nullptr) work.push_back(c);
      }
    }
    return;
  }
  mp.traverseMemoryPool();
}

std::string Translator::sourceText(const a_source_position& start, const a_source_position& end) {
  if (start.seq == 0 || end.seq == 0 || start.seq != end.seq) return "";
  if (start.column == SP_COL_UNKNOWN || end.column == SP_COL_UNKNOWN || end.column < start.column) return "";
  a_line_number line = 0;
  std::string name = fileNameOf(start.seq, &line);
  if (name.empty() || line == 0) return "";
  const std::vector<std::string>& lines = linesOf(name);
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

const std::vector<std::string>& Translator::linesOf(const std::string& fileName) {
  auto it = sourceLines.find(fileName);
  if (it == sourceLines.end()) {
    std::vector<std::string> lines;
    std::ifstream in(fileName);
    std::string l;
    while (std::getline(in, l)) {
      if (!l.empty() && l.back() == '\r') l.pop_back();
      lines.push_back(l);
    }
    it = sourceLines.emplace(fileName, std::move(lines)).first;
  }
  return it->second;
}

namespace {
// Whether the "'" at position i of a line separates the digits of a number (C++14), rather than
// starting a character literal
bool isDigitSeparator(const std::string& l, size_t i) {
  size_t b = i;
  while (b > 0 && (std::isalnum((unsigned char)l[b - 1]) || l[b - 1] == '_' || l[b - 1] == '\'' || l[b - 1] == '.')) --b;
  return b < i && std::isdigit((unsigned char)l[b]);
}
}  // namespace

std::string Translator::lambdaText(const a_source_position& start) {
  if (start.seq == 0 || start.column == SP_COL_UNKNOWN) return "";
  a_line_number line = 0;
  std::string name = fileNameOf(start.seq, &line);
  if (name.empty() || line == 0) return "";
  const std::vector<std::string>& lines = linesOf(name);
  if (line > lines.size()) return "";
  size_t row = line - 1;
  // EDG counts characters (a multibyte UTF-8 character is one column)
  size_t col = 0;
  for (a_column_number c = 1; c < start.column && col < lines[row].size(); ++c) {
    ++col;
    while (col < lines[row].size() && ((unsigned char)lines[row][col] & 0xC0) == 0x80) ++col;
  }
  if (col >= lines[row].size() || lines[row][col] != '[') return "";
  // The body is the first "{" outside the brackets of the introducer, parameters, etc.
  enum { CODE, LINE_COMMENT, BLOCK_COMMENT, STRING, CHARACTER, RAW_STRING } state = CODE;
  std::string rawEnd;
  int depth = 0;
  bool inBody = false;
  std::string text;
  for (; row < lines.size(); ++row, col = 0) {
    const std::string& l = lines[row];
    size_t begin = col;
    for (; col < l.size(); ++col) {
      char c = l[col];
      char next = col + 1 < l.size() ? l[col + 1] : '\0';
      switch (state) {
        case LINE_COMMENT:
          break;
        case BLOCK_COMMENT:
          if (c == '*' && next == '/') state = CODE, begin = col + 2, ++col;
          break;
        case STRING:
        case CHARACTER:
          if (c == '\\') ++col;
          else if (c == (state == STRING ? '"' : '\'')) state = CODE;
          break;
        case RAW_STRING:
          if (l.compare(col, rawEnd.size(), rawEnd) == 0) state = CODE, col += rawEnd.size() - 1;
          break;
        case CODE:
          if (c == '/' && next == '/') {
            // Comments are left out: ROSE attaches them to the statement around the lambda.
            text += l.substr(begin, col - begin);
            state = LINE_COMMENT;
          } else if (c == '/' && next == '*') {
            text += l.substr(begin, col - begin) + " ";
            state = BLOCK_COMMENT, ++col;
          } else if (c == '"' && col > 0 && l[col - 1] == 'R') {
            size_t open = l.find('(', col);
            if (open == std::string::npos) return "";
            rawEnd = ")" + l.substr(col + 1, open - col - 1) + "\"";
            state = RAW_STRING, col = open;
          } else if (c == '"') {
            state = STRING;
          } else if (c == '\'' && !isDigitSeparator(l, col)) {
            state = CHARACTER;
          } else if (c == '(' || c == '[' || c == '{') {
            if (c == '{' && depth == 0) inBody = true;
            ++depth;
          } else if (c == ')' || c == ']' || c == '}') {
            if (--depth < 0) return "";
            if (depth == 0 && c == '}' && inBody) return text + l.substr(begin, col + 1 - begin);
          }
          break;
      }
    }
    if (state == LINE_COMMENT || state == BLOCK_COMMENT) {
      if (state == LINE_COMMENT) state = CODE;
      text += "\n";
    } else {
      text += l.substr(begin) + "\n";
    }
  }
  return "";
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

  fillMissingPositions(nullptr);
  if (std::getenv("EDG2SAGE_DEBUG") != nullptr) checkTree(globalScope, 0);
  if (warnings > 0 && SgProject::get_verbose() > 0) {
    mlog[Sawyer::Message::WARN] << warnings << " IL constructs could not be translated\n";
  }
}

}  // namespace edg2sage
