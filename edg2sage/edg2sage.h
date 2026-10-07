// edg2sage: translation of the EDG C/C++ front end's intermediate language (IL)
// into ROSE's Sage III abstract syntax tree.
//
// This is a new implementation of the "EDG/Sage connection" for the open-source
// EDG front end (https://github.com/edgcpp/compiler).  The EDG front end is built
// as a library (see edgconfig/edg_config.h); after a translation unit has been
// parsed, EDG calls back_end() (edg_main.C), which runs the Translator below over
// the complete, unlowered IL.
#ifndef EDG2SAGE_H
#define EDG2SAGE_H

#include "sage3basic.h"
#include "sageBuilder.h"
#include "sageInterface.h"

// EDG headers (all EDG declarations live in namespace edg)
#include "basic_hdrs.h"
#include "fe_common.h"
#include "src_seq.h"
#include "types.h"

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace edg2sage {

// Diagnostic facility ("EDG_ROSE_Translation" in ROSE's diagnostics)
extern Sawyer::Message::Facility mlog;

// Thrown for IL constructs the translator cannot handle; the enclosing
// declaration is then skipped (with a warning) instead of aborting.
struct Unsupported {
  std::string what;
  explicit Unsupported(const std::string& w) : what(w) {}
};

// A position on a source sequence list.  Function-scope lists contain "sublist
// parent" entries pointing to sublists of file-scope entries; the cursor steps
// into and out of those transparently.
struct SeqCursor {
  edg::a_source_sequence_entry_ptr cur = nullptr;
  edg::a_source_sequence_entry_ptr sublistParent = nullptr;
  SeqCursor() = default;
  explicit SeqCursor(edg::a_source_sequence_entry_ptr e) : cur(e) { normalize(); }
  void normalize();
  void advance() {
    if (cur != nullptr) cur = cur->next;
    normalize();
  }
  bool atEnd() const { return cur == nullptr; }
  edg::an_il_entry_kind kind() const { return (edg::an_il_entry_kind)cur->entity.kind; }
  char* ptr() const { return cur->entity.ptr; }
};

class Translator {
 public:
  explicit Translator(SgSourceFile* file);
  // Translate the whole translation unit (the IL rooted at edg::il_header).
  void translate();

  // ---------------------------------------------------------------- positions
  Sg_File_Info* fileInfo(const edg::a_source_position& pos);
  Sg_File_Info* compilerGeneratedFileInfo(bool output = false);
  void setPosition(SgLocatedNode* node, const edg::a_source_position& start,
                   const edg::a_source_position& end);
  void setPosition(SgLocatedNode* node, const edg::a_source_position& pos) { setPosition(node, pos, pos); }
  void setCompilerGenerated(SgLocatedNode* node, bool output = false);
  bool copyPosition(SgLocatedNode* node, SgExpression* e);
  // Gives every node of a subtree that still lacks a source position a
  // compiler-generated one.
  void fillMissingPositions(SgNode* root);
  void checkTree(SgNode* node, int depth);  // EDG2SAGE_DEBUG: reports broken AST links
  std::string fileNameOf(edg::a_seq_number seq, edg::a_line_number* line = nullptr);
  bool isFromSourceFile(const edg::a_source_position& pos);
  // Text of the source between two positions on the same line (empty if unavailable)
  std::string sourceText(const edg::a_source_position& start, const edg::a_source_position& end);

  // ---------------------------------------------------------------- scopes
  SgScopeStatement* scopeFor(edg::a_scope_ptr scope);
  SgScopeStatement* parentScopeOf(edg::a_source_correspondence* scp, SgScopeStatement* fallback = nullptr);
  SgScopeStatement* currentScope() const { return scopeStack.empty() ? globalScope : scopeStack.back(); }

  // ---------------------------------------------------------------- types
  SgType* convertType(edg::a_type_ptr type);
  SgFunctionType* convertFunctionType(edg::a_type_ptr type, SgClassDefinition* memberOf = nullptr,
                                      SgType* memberClassType = nullptr);
  SgClassDeclaration* classDeclarationFor(edg::a_type_ptr classType);   // first nondefining declaration
  SgEnumDeclaration* enumDeclarationFor(edg::a_type_ptr enumType);      // first nondefining declaration
  SgTypedefDeclaration* typedefDeclarationFor(edg::a_type_ptr typedefType);
  void setTypedefBaseDeclaration(SgTypedefDeclaration* decl);
  SgClassDefinition* classDefinitionFor(edg::a_type_ptr classType);

  // ---------------------------------------------------------------- declarations
  void translateDeclarationList(SeqCursor& cursor, SgScopeStatement* scope, void* endOfConstructEntity);
  SgFunctionDeclaration* functionDeclarationFor(edg::a_routine_ptr routine);  // first nondefining
  SgFunctionSymbol* functionSymbolFor(edg::a_routine_ptr routine);
  SgInitializedName* variableFor(edg::a_variable_ptr var);
  SgVariableSymbol* variableSymbolFor(edg::a_variable_ptr var);
  SgInitializedName* fieldFor(edg::a_field_ptr field);
  SgVariableSymbol* fieldSymbolFor(edg::a_field_ptr field);
  SgEnumFieldSymbol* enumeratorSymbolFor(edg::a_constant_ptr enumerator);
  SgLabelStatement* labelStatementFor(edg::a_label_ptr label);  // created on first use

  // ---------------------------------------------------------------- statements
  SgStatement* convertStatement(edg::a_statement_ptr stmt);
  SgBasicBlock* convertBlock(edg::a_statement_ptr stmt, SgBasicBlock* block = nullptr);
  void appendStatementTo(SgScopeStatement* scope, SgStatement* stmt);

  // ---------------------------------------------------------------- expressions
  SgExpression* convertExpression(edg::an_expr_node_ptr expr);
  SgExpression* convertConstant(edg::a_constant_ptr con, edg::an_expr_node_ptr node = nullptr);
  SgInitializer* convertVariableInitializer(edg::a_variable_ptr var);
  SgInitializer* convertDynamicInit(edg::a_dynamic_init_ptr dip, SgType* type);
  SgInitializer* convertInitializerConstant(edg::a_constant_ptr con, SgType* type);
  SgExprListExp* convertArgumentList(edg::an_expr_node_ptr first);
  SgExpression* initializerExpression(SgInitializer* init);

 private:
  // --- declarations (declarations.C)
  void translateDeclarationEntry(SeqCursor& cursor, SgScopeStatement* scope);
  SgDeclarationStatement* translateVariable(edg::a_variable_ptr var, edg::a_src_seq_secondary_decl_ptr sec,
                                            SgScopeStatement* scope);
  SgDeclarationStatement* translateField(edg::a_field_ptr field, SgClassDefinition* cdef);
  SgFunctionDeclaration* translateRoutine(edg::a_routine_ptr routine, edg::a_src_seq_secondary_decl_ptr sec,
                                          SgScopeStatement* scope, bool declarationOnly = false);
  void translateTypeDeclaration(SeqCursor& cursor, edg::a_type_ptr type, edg::a_src_seq_secondary_decl_ptr sec,
                                SgScopeStatement* scope);
  SgClassDeclaration* translateClassDefinition(SeqCursor& cursor, edg::a_type_ptr type, SgScopeStatement* scope);
  SgEnumDeclaration* translateEnumDefinition(SeqCursor& cursor, edg::a_type_ptr type, SgScopeStatement* scope);
  SgTypedefDeclaration* translateTypedef(edg::a_type_ptr type, edg::a_src_seq_secondary_decl_ptr sec,
                                         SgScopeStatement* scope);
  SgFunctionParameterList* buildParameterList(edg::a_routine_ptr routine, bool defining,
                                              edg::a_type_ptr declaredType = nullptr);
  void setSpecialFunctionKind(SgFunctionDeclaration* decl, edg::a_routine_ptr routine);
  void translateFunctionBody(edg::a_routine_ptr routine, SgFunctionDeclaration* defining);
  void setDeclarationModifiers(SgDeclarationStatement* decl, edg::a_source_correspondence* scp,
                               edg::a_storage_class sc);
  void setAccess(SgDeclarationStatement* decl, edg::an_access_specifier access);
  void attachPendingBaseTypeDeclaration(SgDeclarationStatement* decl);
  SgVariableDeclaration* declaratorGroupFor(SgScopeStatement* scope, SgType* type,
                                            const edg::a_source_position& specifiers);
 public:
  SgDeclarationStatement* typeDefinitionInExpression(edg::a_type_ptr type);
 private:
  SgName nameOf(edg::a_source_correspondence* scp);
  void translateDeclarationStatement(edg::a_statement_ptr stmt, SgScopeStatement* scope);
  void skipToEndOfConstruct(SeqCursor& cursor, void* entity);
  SgPragmaDeclaration* translatePragma(edg::a_pragma_ptr pragma);
  SgDeclarationStatement* translateStaticAssertion(edg::a_static_assertion_ptr sa);
  void finishDeferredFunctionBodies();

  // --- C++ declarations (cxx.C)
 public:
  SgNamespaceDeclarationStatement* namespaceDeclarationFor(edg::a_namespace_ptr ns);  // first declaration
 private:
  void translateNamespace(SeqCursor& cursor, edg::a_namespace_ptr ns, edg::a_src_seq_secondary_decl_ptr sec,
                          SgScopeStatement* scope);
  SgDeclarationStatement* translateUsingDeclaration(edg::a_using_decl_ptr ud, SgScopeStatement* scope);
  void translateBaseClasses(edg::a_class_type_supplement_ptr ctsp, SgClassDefinition* cdef);
  void translateConstructorInitializers(edg::a_scope_ptr fscope, SgMemberFunctionDeclaration* decl);
 public:
  SgExpression* convertLambda(edg::a_lambda_ptr lambda);
 private:

  // --- templates (templates.C)
 public:
  // The first (hidden) declaration of a template
  SgDeclarationStatement* templateDeclarationFor(edg::a_template_ptr tmpl);
  SgTemplateArgumentPtrList convertTemplateArguments(edg::a_template_arg_ptr args);
  // Class template instances, members of class template instances
  bool isTemplateInstance(edg::a_type_ptr classType);
  SgClassDeclaration* instanceDeclarationFor(edg::a_type_ptr classType);
  SgClassDeclaration* newDefiningClassDeclaration(SgClassDeclaration* first, SgClassDefinition*& def);
  SgClassDeclaration* newNondefiningClassDeclaration(SgClassDeclaration* first);
  SgClassDefinition* hiddenDefinitionFor(edg::a_type_ptr classType);
  SgFunctionDeclaration* newFunctionDeclaration(edg::a_routine_ptr routine, const SgName& name, SgFunctionType* type,
                                                bool member);
  SgInitializedName* hiddenFieldFor(edg::a_field_ptr field);
 private:
  void translateTemplate(SeqCursor& cursor, edg::a_template_ptr tmpl, edg::a_src_seq_secondary_decl_ptr sec,
                         SgScopeStatement* scope);
  void skipTemplateMembers(SeqCursor& cursor, edg::a_template_ptr tmpl);
  SgDeclarationStatement* translateInstantiationDirective(edg::an_instantiation_directive_ptr id,
                                                          SgScopeStatement* scope);
  std::string templateText(edg::a_template_ptr tmpl);

  // --- attributes (attributes.C)
  void applyClassAttributes(SgClassDeclaration* decl, edg::a_type_ptr type);
  void applyVariableAttributes(SgInitializedName* in, edg::an_attribute_ptr attrs, bool packed, bool primaryOnly);
  void applyFunctionAttributes(SgFunctionDeclaration* decl, edg::an_attribute_ptr attrs, bool primaryOnly);

  // --- statements (statements.C)
  SgStatement* convertStatementKind(edg::a_statement_ptr stmt);
  void convertStatementListInto(edg::a_statement_ptr first, SgScopeStatement* scope, bool single = false);
  void attachDeferredInitializer(edg::a_statement_ptr initStmt);
  SgStatement* convertForStatement(edg::a_statement_ptr stmt);
  SgStatement* convertRangeBasedForStatement(edg::a_statement_ptr stmt);
  SgStatement* convertSwitchStatement(edg::a_statement_ptr stmt);
  SgStatement* convertIfStatement(edg::a_statement_ptr stmt);
  SgStatement* convertCondition(edg::an_expr_node_ptr expr, SgScopeStatement* scope);
  SgStatement* convertTryStatement(edg::a_statement_ptr stmt);
  SgStatement* convertAsmStatement(edg::an_asm_entry_ptr asm_entry, const edg::a_source_position& pos);
  void restructureSwitchBody(SgBasicBlock* body);

  // --- expressions (expressions.C)
  SgExpression* convertOperation(edg::an_expr_node_ptr expr);
  SgExpression* convertCall(edg::an_expr_node_ptr expr);
  SgExpression* convertVariableReference(edg::a_variable_ptr var, edg::an_expr_node_ptr expr);
  SgExpression* convertRoutineReference(edg::a_routine_ptr routine, edg::an_expr_node_ptr expr);
  SgVariableSymbol* functionNameSymbol(const std::string& name, SgType* type);
  SgExpression* convertIntegerConstant(edg::a_constant_ptr con, edg::an_expr_node_ptr node);
  SgExpression* convertFloatConstant(edg::a_constant_ptr con, edg::an_expr_node_ptr node);
  SgExpression* convertStringConstant(edg::a_constant_ptr con, edg::an_expr_node_ptr node);
  SgExpression* convertAddressConstant(edg::a_constant_ptr con, edg::an_expr_node_ptr node);
  SgExpression* convertCast(edg::an_expr_node_ptr expr, SgExpression* operand, bool implicit);
  SgExpression* convertNewDelete(edg::an_expr_node_ptr expr);
  SgExpression* convertTempInit(edg::an_expr_node_ptr expr);
  SgExpression* convertSizeof(edg::an_expr_node_ptr expr);
  SgExpression* convertFieldSelection(edg::an_expr_node_ptr expr, bool arrow);
  SgExpression* convertStatementExpression(edg::an_expr_node_ptr expr);
  SgAggregateInitializer* convertAggregate(edg::a_constant_ptr con, SgType* type);
  void appendAggregateElements(SgExprListExp* list, edg::a_constant_ptr aggregate);
  void setExpressionPosition(SgExpression* e, edg::an_expr_node_ptr expr);
  bool isImplicitNode(edg::an_expr_node_ptr expr);
  edg::an_expr_node_ptr skipImplicitSteps(edg::an_expr_node_ptr expr);

 public:
  SgSourceFile* sourceFile;
  SgGlobal* globalScope;
  std::vector<SgScopeStatement*> scopeStack;

  // EDG IL entity -> Sage node maps
  std::unordered_map<edg::a_routine_ptr, SgFunctionDeclaration*> firstRoutineDecl;
  std::unordered_map<edg::a_routine_ptr, SgFunctionDeclaration*> definingRoutineDecl;
  std::unordered_map<edg::a_variable_ptr, SgInitializedName*> variables;
  std::unordered_map<edg::a_field_ptr, SgInitializedName*> fields;
  std::unordered_map<edg::a_type_ptr, SgClassDeclaration*> firstClassDecl;
  std::unordered_map<edg::a_type_ptr, SgClassDeclaration*> definingClassDecl;
  std::unordered_map<edg::a_type_ptr, SgEnumDeclaration*> firstEnumDecl;
  std::unordered_map<edg::a_type_ptr, SgEnumDeclaration*> definingEnumDecl;
  std::unordered_map<edg::a_type_ptr, SgTypedefDeclaration*> typedefDecls;
  std::unordered_map<edg::a_namespace_ptr, SgNamespaceDeclarationStatement*> firstNamespaceDecl;
  std::unordered_map<edg::a_template_ptr, SgDeclarationStatement*> firstTemplateDecl;  // canonical template ->
  std::unordered_map<edg::a_template_ptr, SgDeclarationStatement*> definingTemplateDecl;
  std::unordered_map<edg::a_type_ptr, SgType*> typeCache;
  std::unordered_map<edg::a_constant_ptr, SgInitializedName*> enumerators;
  std::unordered_map<edg::a_scope_ptr, SgScopeStatement*> scopes;
  std::unordered_map<edg::a_label_ptr, SgLabelStatement*> labels;
  std::unordered_map<edg::a_label_ptr, SgLabelSymbol*> labelSymbols;
  std::set<edg::a_constant_ptr> foldedConstants;
  std::map<SgInitializedName*, SgVariableSymbol*> pendingParameterSymbols;
  // Start of the declaration specifiers of each variable declaration
  std::map<SgVariableDeclaration*, edg::a_source_position> declarationSpecifiers;
  std::map<std::pair<SgScopeStatement*, std::string>, SgVariableSymbol*> functionNameSymbols;  // constants whose backing expression is being translated
  int compoundLiterals = 0;
  bool suppressInitializers = false;  // the iteration variable of a range-based for
  std::set<SgClassDeclaration*> firstUsedAsStatement;      // hidden first decl reused as forward declaration
  std::set<SgClassDeclaration*> hiddenDefinitions;         // definitions of template instances (see templates.C)
  std::set<SgTypedefDeclaration*> typedefInStatementList;

  // A non-autonomous tag definition ("struct S {...} x;") waiting for the
  // declaration it is part of.
  SgDeclarationStatement* pendingBaseTypeDecl = nullptr;
  edg::a_type_ptr pendingBaseType = nullptr;

  // In-class member function definitions are translated after the class body.
  std::vector<std::pair<edg::a_routine_ptr, SgFunctionDeclaration*>> deferredBodies;
  int classNesting = 0;

  // The routine whose body is being translated
  edg::a_routine_ptr currentRoutine = nullptr;
  SgFunctionDefinition* currentFunctionDefinition = nullptr;

  // file name handling
  std::unordered_map<edg::a_source_file_ptr, std::string> fileNames;
  std::map<std::string, std::vector<std::string>> sourceLines;
  std::string primaryFileName;
  bool isCxx = false;
  int warnings = 0;
};

// The function scope of a defined routine (NULL for routines without a definition).
inline edg::a_scope_ptr functionScopeOf(edg::a_routine_ptr routine) {
  if (routine == nullptr || routine->function_def_number == 0) return nullptr;  // NULL_function_def_number
  return edg::scope_for_routine_or_null(routine);
}

// The active translator (valid during back_end())
extern Translator* translator;

// True if `text` is the spelling of a numeric literal (used to keep literals as written)
bool isNumericLiteral(const std::string& text);

// Helper: normalizes a path to an absolute path without "." and ".." components
std::string absolutePath(const std::string& path);

}  // namespace edg2sage

#endif
