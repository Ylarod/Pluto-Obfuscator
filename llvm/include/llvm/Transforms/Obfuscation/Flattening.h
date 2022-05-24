#ifndef LLVM_FLATTENING_H
#define LLVM_FLATTENING_H

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

namespace llvm {
class Flattening : public FunctionPass {
public:
  static char ID;
  bool enable;

  explicit Flattening(bool enable) : FunctionPass(ID) { this->enable = enable; }

  StringRef getPassName() const override{
    return "Flattening";
  }

  static void flatten(Function &F);

  bool runOnFunction(Function &F) override;
};

FunctionPass *createFlatteningPass(bool enable);
} // namespace llvm

#endif // LLVM_FLATTENING_H