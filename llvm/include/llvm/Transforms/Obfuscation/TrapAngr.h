#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

namespace llvm {

class TrapAngr : public FunctionPass {
public:
  static char ID;
  bool enable;

  explicit TrapAngr(bool enable) : FunctionPass(ID) { this->enable = enable; }

  StringRef getPassName() const override{
    return "TrapAngr";
  }

  bool runOnFunction(Function &F) override;

  static void substitute(Instruction *I, unsigned int i);
};

FunctionPass *createTrapAngrPass(bool enable);

} // namespace llvm