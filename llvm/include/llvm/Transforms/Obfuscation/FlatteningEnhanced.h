#ifndef LLVM_FLATTENING_ENHANCED_H
#define LLVM_FLATTENING_ENHANCED_H

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include <list>
#include <random>
#include <vector>

namespace llvm {
struct TreeNode {
  unsigned int val = 0, limit = 0, l = 0, r = 0;
  TreeNode *left = nullptr, *right = nullptr;
};
class MyFlatten : public FunctionPass {
public:
  static char ID;
  bool enable;
  static std::random_device rd;

  explicit MyFlatten(bool enable) : FunctionPass(ID) { this->enable = enable; }

  StringRef getPassName() const override{
    return "MyFlatten";
  }

  static std::list<TreeNode *>::iterator
  randomElement(std::list<TreeNode *> *x);
  static void expandNode(TreeNode *node);
  static unsigned long genRandomTree(TreeNode *node, unsigned long node_limit);
  void walkTree(TreeNode *node);
  bool allocNode(TreeNode *node, unsigned int l, unsigned int r);
  BasicBlock *
  createRandomBasicBlock(TreeNode *node, Function *f, Value *var,
                         std::vector<BasicBlock *>::iterator &iter,
                         std::map<BasicBlock *, TreeNode *> *bb_info);
  bool spawnRandomIf(BasicBlock *from, std::vector<BasicBlock *> *son,
                     Value *var, std::map<BasicBlock *, TreeNode *> *bb_info);
  static std::vector<BasicBlock *> *getBlocks(Function *function,
                                              std::vector<BasicBlock *> *lists);

  static unsigned int getUniqueNumber(std::vector<unsigned int> *rand_list);
  static bool valueEscapes(Instruction *Inst);
  void DoFlatten(Function *f, unsigned long seed, unsigned int enderNum);
  bool runOnFunction(Function &function) override;
};
FunctionPass *createFlatteningEnhancedPass(bool enable);
} // namespace llvm

#endif // LLVM_FLATTENING_ENHANCED_H