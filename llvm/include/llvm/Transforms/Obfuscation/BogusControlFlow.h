#ifndef LLVM_BOGUS_CONTROL_FLOW_H
#define LLVM_BOGUS_CONTROL_FLOW_H

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

namespace llvm {
class BogusControlFlow : public FunctionPass {
public:
  static char ID;
  bool enable;

  explicit BogusControlFlow(bool enable) : FunctionPass(ID) {
    this->enable = enable;
  }

  bool runOnFunction(Function &F) override;

  // 对基本块 BB 进行混淆
  static void bogus(BasicBlock *BB);

  // 创建条件恒为真的 ICmpInst*
  // 该比较指令的条件为：y < 10 || x * (x + 1) % 2 == 0
  // 其中 x, y 为恒为0的全局变量
  static Value *createBogusCmp(BasicBlock *insertAfter);
};

FunctionPass *createBogusControlFlow(bool enable);

} // namespace llvm

#endif // LLVM_BOGUS_CONTROL_FLOW_H