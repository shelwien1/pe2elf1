#ifndef ROSE_FlowAnalysisInterface_H
#define ROSE_FlowAnalysisInterface_H

#include <RoseFirst.h>
#include <Rose/FlowGraphInterface.h>

class SgNode;
class SgFunctionDeclaration;
class SgExpression;
class SgProject;

namespace Rose{
namespace FlowGraphInterface {

// Internally automatically select the desired analysis based on the options.
enum class AnalysisSupportOption { CodeThorn, AstSideEffect };

template <class NodeIterator, class EdgeIterator, class CallData, AnalysisSupportOption>
class CallGraphAnalysis {
 public:
  // Call Graph Analysis Interface below.
  typedef SgFunctionDeclaration* NodeInfo;
  typedef SgExpression* EdgeInfo;
  typedef FlowGraphAccessInterface<NodeIterator,EdgeIterator> GraphAccess;

  GraphAccess& getResultGraph();
  CallData& getSavedCallData();

  bool performAnalysis(SgProject* ast);
};

}; // namespace FlowGraphInterface

}; // namespace Rose

#endif
