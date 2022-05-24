#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"

namespace llvm {

class MBAObfuscation : public FunctionPass {
public:
  static char ID;
  bool enable;

  explicit MBAObfuscation(bool enable) : FunctionPass(ID) {
    this->enable = enable;
  }

  StringRef getPassName() const override{
    return "MBAObfuscation";
  }

  bool runOnFunction(Function &F) override;

  static void substituteConstant(Instruction *I, unsigned int i);

  static void substitute(BinaryOperator *BI);

  // 替换 Add 指令
  static Value *substituteAdd(BinaryOperator *BI);

  // 替换 Sub 指令
  static Value *substituteSub(BinaryOperator *BI);

  // 替换 And 指令
  static Value *substituteAnd(BinaryOperator *BI);

  // 替换 Or 指令
  static Value *substituteOr(BinaryOperator *BI);

  // 替换 Xor 指令
  static Value *substituteXor(BinaryOperator *BI);
};

FunctionPass *createMBAObfuscationPass(bool enable);
} // namespace llvm