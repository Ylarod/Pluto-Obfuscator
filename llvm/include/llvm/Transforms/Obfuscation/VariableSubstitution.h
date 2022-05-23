#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

namespace llvm {

class VariableSubstitution : public FunctionPass {
public:
  static char ID;
  bool enable;

  explicit VariableSubstitution(bool enable) : FunctionPass(ID) {
    this->enable = enable;
  }

  bool runOnFunction(Function &F) override;

  // 对单个指令 BI 进行替换
  static void substitute(Instruction *I);

  // 线性替换：val -> ax + by + c
  // 其中 val 为原常量 a, b 为随机常量 x, y 为随机全局变量 c = val - (ax + by)
  static void linearSubstitute(Instruction *I, unsigned int i);

  // 按位运算替换：val -> (x << left | y >> right) ^ c
  // 其中 val 为原常量x, y 为随机全局变量 c = val ^ (x << left | y >> right)
  static void bitwiseSubstitute(Instruction *I, unsigned int i);
};

FunctionPass *createVariableSubstitutionPass(bool enable);
} // namespace llvm