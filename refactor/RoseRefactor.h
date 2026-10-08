// RoseRefactor: support for source-to-source refactoring tools built on ROSE and EDG.
//
// The tools parse a translation unit with ROSE's frontend() and then change the original
// source text (rather than unparsing the AST), so that formatting, comments and macros are kept:
//
// * Cross-references: every reference to every named entity (variables, functions, classes,
//   members, namespaces, typedefs, enumerators, parameters, template parameters, ...), with the
//   exact position of the name, as recorded by the EDG front end; and, for each entity, its
//   kind, qualified name, type, enclosing class, access, base classes, overridden functions.
// * Source files and edits: line/column positions (as EDG counts them) to text, scanning of
//   C++ tokens, and a set of replacements applied to the files.
//
//   RoseRefactor::recordCrossReferences();
//   SgProject* project = frontend(argc, argv);
//   const RoseRefactor::CrossReferences& xref = RoseRefactor::crossReferences();
//   for (const RoseRefactor::Entity* e : xref.named("count")) ...
#ifndef ROSE_REFACTOR_H
#define ROSE_REFACTOR_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace RoseRefactor {

// ---------------------------------------------------------------------------------------------
// Positions
// ---------------------------------------------------------------------------------------------

// A position in a source file.  Lines and columns count from 1; a column counts characters (a
// UTF-8 multibyte character is one column), as EDG does.
struct Position {
  std::string file;  // as EDG names the file (usually absolute)
  int line = 0;
  int column = 0;

  Position() = default;
  Position(const std::string& f, int l, int c) : file(f), line(l), column(c) {}
  bool valid() const { return line > 0; }
  bool operator==(const Position& o) const { return line == o.line && column == o.column && file == o.file; }
  bool operator!=(const Position& o) const { return !(*this == o); }
  bool operator<(const Position& o) const;
  std::string str() const;  // "file:line:column"
};

// ---------------------------------------------------------------------------------------------
// Cross-references
// ---------------------------------------------------------------------------------------------

enum class Kind {
  Namespace, Class, Struct, Union, Enum, Enumerator, Typedef, Variable, Parameter, Field,
  StaticDataMember, Function, MemberFunction, Constructor, Destructor, ClassTemplate,
  FunctionTemplate, VariableTemplate, Concept, TemplateParameter, Label, Macro, Other
};
const char* kindName(Kind k);  // "variable", "member function", ...

enum class Access { None, Public, Protected, Private };
const char* accessName(Access a);  // "public", ... ("" for None)

// A scope in which names are declared and looked up.  Two scopes are the same scope if their
// kinds and ids are equal.
struct Scope {
  enum class Kind : unsigned char {
    None,       // unknown (macros, for example)
    Global,     // the global namespace (id 0)
    Namespace,  // a namespace (id: the namespace, an entity)
    Class,      // a class, struct, union or scoped enum (id: that entity)
    Function,   // the outermost block of a function, where its parameters are declared
    Local       // a block, a condition, a template parameter list or a function prototype (in
                // C++, the outermost block of a statement controlled by a condition is the scope
                // of the condition; the condition and the outermost block of the body of a for
                // statement are the scope of its init-statement or range declaration)
  };
  Kind kind = Kind::None;
  std::uint64_t id = 0;   // for Function and Local, a number unique in the translation unit
  bool operator==(const Scope& o) const { return kind == o.kind && id == o.id; }
  bool operator!=(const Scope& o) const { return !(*this == o); }
  bool operator<(const Scope& o) const { return kind != o.kind ? kind < o.kind : id < o.id; }
};

// A reference to an entity at a position: what EDG's cross-reference listing records.
struct Reference {
  Position pos;            // the position of the entity's name
  char code = 'R';         // 'd' declaration, 'D' definition, 't'/'T' declaration/definition by
                           // template instantiation, 'U' use, 'M' modification, 'C' use and
                           // modification, 'A' address taken, 'R' other reference, 'E' error
  bool inSystemHeader = false;
  unsigned order = 0;      // the position of the reference in the translation unit (in the order
                           // in which the front end processes the text)
  int scopes = -1;         // the scopes that enclose the reference, in which an unqualified name
                           // is looked up: CrossReferences::scopes(scopes) (-1 if unknown)
  bool isDeclaration() const { return code == 'd' || code == 'D' || code == 't' || code == 'T'; }
  bool isDefinition() const { return code == 'D' || code == 'T'; }
};

struct BaseClass {
  std::uint64_t entity = 0;   // the base class (an entity of kind Class, Struct or Union)
  Access access = Access::None;
  bool isVirtual = false;
  Position start, end;        // the base specifier in the source ("public Base<T>"), if known
};

typedef std::uint64_t EntityId;

struct Entity {
  EntityId id = 0;                  // unique within one translation unit
  std::string name;                 // the name as declared (unqualified)
  std::string qualifiedName;        // with the enclosing namespaces and classes ("ns::A::f")
  Kind kind = Kind::Other;
  std::string type;                 // the type as C++ text, for variables, functions, typedefs, ...
  EntityId parent = 0;              // the class (or namespace) the entity is a member of
  Scope scope;                      // the scope the entity is declared in (for the members of an
                                    // anonymous union, the scope that encloses it)
  Access access = Access::None;     // for class members
  bool isStatic = false;            // static member
  bool isVirtual = false;
  bool isPureVirtual = false;
  bool isConst = false;             // const member function
  bool isImplicit = false;          // declared by the compiler
  bool isTemplateInstance = false;  // instance of a template, or member of a class template instance
  bool isInTemplate = false;        // member of a class template (not of an instance)
  bool isLocal = false;             // declared in a function
  std::vector<BaseClass> bases;     // classes: the direct base classes, in declaration order
  std::vector<EntityId> overrides;  // virtual member functions: the functions they override
  std::vector<Reference> references;  // in source order

  bool isMember() const;            // of a class
  bool isType() const;              // class, struct, union, enum, typedef, class template, type template parameter
  // The first declaration (or definition) outside system headers, in the order of the
  // translation unit; an invalid position if none
  Position declaration() const;
  // Whether the front end recorded a declaration of the entity (entities without one are names
  // that depend on template parameters, in templates, or implicit declarations)
  bool isDeclared() const;
  bool declaredInSystemHeader() const;  // all its declarations are in system headers
};

class CrossReferences {
public:
  CrossReferences();
  ~CrossReferences();
  CrossReferences(const CrossReferences&) = delete;
  CrossReferences& operator=(const CrossReferences&) = delete;

  bool empty() const { return entities_.empty(); }
  const std::map<EntityId, Entity>& entities() const { return entities_; }
  const Entity* entity(EntityId id) const;
  // The entities with this (unqualified) name
  std::vector<const Entity*> named(const std::string& name) const;
  // The entities referenced at exactly this position
  std::vector<const Entity*> at(const Position& pos) const;
  // The entities declared at the same positions as e (a template and its instances, a member of
  // a class template and the corresponding members of the instances), e included
  std::vector<const Entity*> sameDeclaration(const Entity& e) const;
  // Whether class b is a base (direct or indirect) of class d
  bool isBaseOf(EntityId b, EntityId d) const;
  // The direct base of class d through which b is a base of d (or 0)
  const BaseClass* directBaseLeadingTo(EntityId d, EntityId b) const;
  // The scopes that enclose a reference (Reference::scopes), innermost first: those in which an
  // unqualified name is looked up there (empty if unknown).  A class scope stands for the class
  // and its base classes; a namespace's using-directives are not included.
  const std::vector<Scope>& scopes(int index) const;
  // Whether the translation unit is C++ (rather than C)
  bool cplusplus() const { return cplusplus_; }

  // Used by the front end
  Entity& add(EntityId id);
  int addScopes(const std::vector<Scope>& scopes);  // the index of a list of scopes
  void setCplusplus(bool on) { cplusplus_ = on; }
  void finish();  // sorts the references and builds the indexes
  void clear();

private:
  std::map<EntityId, Entity> entities_;
  std::multimap<std::string, EntityId> byName_;
  std::multimap<Position, EntityId> byPosition_;
  std::multimap<Position, EntityId> byDeclaration_;
  std::vector<std::vector<Scope>> scopes_;
  std::map<std::vector<Scope>, int> scopeIndex_;
  bool cplusplus_ = true;
};

// Makes the next frontend() calls record cross-references (in the EDG front end)
void recordCrossReferences(bool on = true);
// More options for the EDG front end in the next frontend() calls, e.g. "--no_dep_name"
void setFrontEndOptions(const std::vector<std::string>& options);
// Other options to try when the front end rejects a source file with those of
// setFrontEndOptions(): it runs again with each set of options in turn, until it accepts the
// file.  The diagnostics of the runs that fail are not shown (except those of the last).
void setAlternativeFrontEndOptions(const std::vector<std::vector<std::string>>& alternatives);
// The options the front end accepted the last C or C++ file with: 0 for those of
// setFrontEndOptions(), 1 for the first alternative, and so on
int frontEndOptionsUsed();
// Parsing as Visual C++ parses, with its headers, rather than as GCC does: the front end runs in
// its Microsoft mode (the language, extensions and predefined macros of Visual C++, and its data
// model and class layout for x64), with includeDirs as the folders of the system headers instead
// of GCC's (the folders of the INCLUDE environment variable of Visual C++).  version is the
// _MSC_VER of the Visual C++ to emulate, such as 1920 for Visual Studio 2019 16.0; 0 takes the
// version and the build number from the headers (crtversion.h).
void setMicrosoftMode(const std::vector<std::string>& includeDirs, int version = 0);
bool microsoftMode();  // setMicrosoftMode() was called
// The folders of the headers of Visual C++ and of the Windows SDK: those of the INCLUDE environment
// variable if dir is empty, otherwise those in dir that exist: include, atlmfc\include,
// ucrt\include and sdk\include (with its ucrt, um, shared and winrt subfolders if it has them),
// the layout of a portable Visual C++
std::vector<std::string> microsoftIncludeDirs(const std::string& dir);
// Whether the next frontend() calls translate the C/C++ code into ROSE's AST (they do by
// default); tools that only use the cross-references can skip it (the files of the project then
// have empty global scopes)
void buildAst(bool on);
// The cross-references of the last C or C++ file parsed by frontend() (empty if not recorded)
const CrossReferences& crossReferences();

// ---------------------------------------------------------------------------------------------
// Source text
// ---------------------------------------------------------------------------------------------

// The text of a source file, with conversions between positions and offsets
class SourceText {
public:
  explicit SourceText(const std::string& file);  // reads the file (empty if it cannot be read)
  bool ok() const { return ok_; }
  const std::string& file() const { return file_; }
  const std::string& text() const { return text_; }
  // Byte offset of a position (std::string::npos if outside the file)
  std::size_t offset(int line, int column) const;
  std::size_t offset(const Position& p) const { return offset(p.line, p.column); }
  Position position(std::size_t offset) const;
  std::string line(int line) const;  // without the line terminator
  int lineCount() const { return (int)lineStart_.size(); }
  // The identifier (or keyword) that starts at an offset ("" if none)
  std::string identifierAt(std::size_t offset) const;
  // The offsets where an identifier is written in the code (not in comments or literals, and not
  // as a part of a longer identifier or of a number)
  std::vector<std::size_t> occurrences(const std::string& identifier) const;
  // The offset of the end of the comment or white space starting at offset (offset if none)
  std::size_t skipSpace(std::size_t offset) const;
  // The offset just after the last token before offset, skipping white space and comments
  std::size_t skipSpaceBackward(std::size_t offset) const;
  // The offset of the bracket matching the one at offset ("(", "[", "{" or "<" forward; ")",
  // "]", "}" or ">" backward), skipping comments, string and character literals (npos if none)
  std::size_t matching(std::size_t offset) const;
  // The offset of the next occurrence of the character c at offset or later, outside comments,
  // literals and nested brackets (npos if none)
  std::size_t find(char c, std::size_t offset) const;
  // The line terminator used in the file ("\n" or "\r\n")
  const char* newline() const { return crlf_ ? "\r\n" : "\n"; }
  // The white space at the start of a line
  std::string indentation(int line) const;

private:
  std::string file_;
  std::string text_;
  std::vector<std::size_t> lineStart_;
  bool ok_ = false;
  bool crlf_ = false;
};

// Replacements of text in source files, applied all at once
class Edits {
public:
  // Replaces the text between two offsets of a file (end == start: inserts)
  void replace(const std::string& file, std::size_t start, std::size_t end, const std::string& text);
  void insert(const std::string& file, std::size_t at, const std::string& text) { replace(file, at, at, text); }
  bool empty() const { return edits_.empty(); }
  // Overlapping replacements (other than insertions at the same offset), as "file:offset" strings
  std::vector<std::string> conflicts() const;
  // The new text of a file
  std::string apply(const std::string& file, const std::string& text) const;
  // The files that are changed
  std::vector<std::string> files() const;
  // Writes the changed files; returns false (and describes the problem in *error) if one fails
  bool write(std::string* error = nullptr) const;

private:
  struct Edit {
    std::size_t start, end;
    std::string text;
    unsigned order;
  };
  std::map<std::string, std::vector<Edit>> edits_;
  unsigned count_ = 0;
};

// The source text of each file, read once
SourceText& sourceText(const std::string& file);

// Where a name is written for a reference at a position: the byte offset of the name in the
// file, if the name is written at the position or, when the position is the invocation of a
// function-like macro, written once in the macro's arguments.  Otherwise std::string::npos:
// either the reference is not written with the name (an implicit constructor call, for example),
// or the name is in the definition of a macro, whose name is then stored in *macro.
std::size_t nameOffset(const CrossReferences& xr, const Position& pos, const std::string& name,
                       std::string* macro = nullptr);

// Where a name is written in the code of the files of the cross-references (outside system
// headers), but the front end recorded no reference to a declared entity of that name: in code
// that the preprocessor skipped, in the definition of a macro, as the name of a member of a
// template parameter, or in the body of a template that the front end did not parse (when
// template bodies are only parsed where they are instantiated)
std::vector<Position> unresolvedOccurrences(const CrossReferences& xr, const std::string& name);

// ---------------------------------------------------------------------------------------------
// Command lines of the tools
// ---------------------------------------------------------------------------------------------

// Splits a tool's command line ("tool [options] source.cpp args...") into the command line for
// frontend() (the program name, the options and the source files) and the other arguments.
// Source files are recognized by their extension (.c .cc .cpp .cxx .c++ .C .h .hh .hpp .hxx).
// The options --msvc (the headers of the INCLUDE environment variable), --msvc=<folder> and
// --msvc-version=<_MSC_VER> are not passed on: they call setMicrosoftMode().
void splitCommandLine(int argc, char* argv[], std::vector<std::string>& frontEndArgs,
                      std::vector<std::string>& toolArgs);

}  // namespace RoseRefactor

#endif
