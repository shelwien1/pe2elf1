// Internal interface between RoseRefactor (refactor/RoseRefactor.C) and the EDG front end
// (edg2sage/xref.C).  Not part of the public API.
#ifndef ROSE_REFACTOR_IMPL_H
#define ROSE_REFACTOR_IMPL_H

#include "RoseRefactor.h"

namespace RoseRefactor {
namespace impl {

bool recording();                                 // recordCrossReferences() was called
const std::vector<std::string>& frontEndOptions();  // setFrontEndOptions()
CrossReferences& current();                       // what crossReferences() returns
std::string temporaryFile(const char* prefix);    // a new, empty temporary file
void removeAtExit(const std::string& file);       // removes the file when the program exits

}  // namespace impl
}  // namespace RoseRefactor

#endif
