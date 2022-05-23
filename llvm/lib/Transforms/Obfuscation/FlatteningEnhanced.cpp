#include "llvm/Transforms/Obfuscation/FlatteningEnhanced.h"
#include "llvm/IR/Instructions.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Obfuscation/CryptoUtils.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/Local.h"
#include <cstdlib>
#include <ctime>
#include <list>
#include <map>
#include <utility>
#include <vector>
#include <random>

using namespace llvm;

std::list<TreeNode *>::iterator
MyFlatten::randomElement(std::list<TreeNode *> *x) {
  auto iter = x->begin();
  unsigned long val = x->size();
  for (unsigned long i = 0; i < rd() % val; i++)
    iter++;
  return iter;
}

void MyFlatten::expandNode(TreeNode *node) {
  auto *newNode = (TreeNode *)malloc(sizeof(TreeNode));
  newNode->left = newNode->right = nullptr;
  node->left = newNode;
  newNode = (TreeNode *)malloc(sizeof(TreeNode));
  newNode->left = newNode->right = nullptr;
  node->right = newNode;
}

unsigned long MyFlatten::genRandomTree(TreeNode *tree_node,
                                       unsigned long node_limit) {
  std::list<TreeNode *> q;
  q.push_back(tree_node);
  unsigned long node_num = 1;
  while (!q.empty() && node_num < node_limit) {
    auto tmp = randomElement(&q);
    TreeNode *node = *tmp;
    int val = (node->left == nullptr) + (node->right == nullptr);
    if (val == 2) {
      expandNode(node);
      q.push_back(node->left);
      q.push_back(node->right);
      q.erase(tmp);
    }
    node_num++;
  }
  q.clear();
  return node_num;
}

void MyFlatten::walkTree(TreeNode *node) {
  if (node->left != nullptr) // Traverse all branches
    walkTree(node->left);
  if (node->right != nullptr)
    walkTree(node->right);
  node->limit = 0;
  if (node->left == nullptr &&
      node->right == nullptr) // Start to calculate node info
    node->limit = 1;
  else
    node->limit = node->left->limit + node->right->limit;
}

bool MyFlatten::allocNode(TreeNode *node, unsigned int l, unsigned int r) {
  if (r - l < node->limit)
    return false;
  node->l = l;
  node->r = r;
  if (node->left == nullptr && node->right == nullptr)
    return true;
  unsigned int var;
  if (r - l - node->limit == 0)
    var = 0;
  else
    var = rd() % (r - l - node->limit);
  unsigned int mid = l + node->left->limit + var;
  if (!allocNode(node->left, l, mid) || !allocNode(node->right, mid, r))
    return false;
  return true;
}

BasicBlock *
MyFlatten::createRandomBasicBlock(TreeNode *node, Function *f, Value *var,
                                  std::vector<BasicBlock *>::iterator &iter,
                                  std::map<BasicBlock *, TreeNode *> *bb_info) {
  if (node->left == nullptr && node->right == nullptr) {
    BasicBlock *bb = *iter;
    bb_info->insert(std::pair<BasicBlock *, TreeNode *>(bb, node));
    iter++;
    return bb;
  }
  BasicBlock *left_bb =
      createRandomBasicBlock(node->left, f, var, iter, bb_info);
  BasicBlock *right_bb =
      createRandomBasicBlock(node->right, f, var, iter, bb_info);
  BasicBlock *node_bb = BasicBlock::Create(f->getContext(), "knot", f);
  if (node->left->r != node->right->l)
    errs() << "Error!\n";
  auto *load =
      new LoadInst(Type::getInt32Ty(f->getContext()), var, "", node_bb);
  auto *condition = new ICmpInst(
      *node_bb, ICmpInst::ICMP_ULT, load,
      ConstantInt::get(Type::getInt32Ty(f->getContext()), node->left->r));
  BranchInst::Create(left_bb, right_bb, (Value *)condition, node_bb);
  return node_bb;
}

bool MyFlatten::spawnRandomIf(BasicBlock *from, std::vector<BasicBlock *> *son,
                              Value *var,
                              std::map<BasicBlock *, TreeNode *> *bb_info) {
  TreeNode tree;
  genRandomTree(&tree, son->size());
  walkTree(&tree);
  if (!allocNode(&tree, 0, 0x7fffffff))
    return false;
  auto iter = son->begin();
  BasicBlock *head =
      createRandomBasicBlock(&tree, from->getParent(), var, iter, bb_info);
  BranchInst::Create(head, from);
  return true;
}

std::vector<BasicBlock *> *
MyFlatten::getBlocks(Function *function, std::vector<BasicBlock *> *lists) {
  lists->clear();
  for (BasicBlock &basicBlock : *function)
    lists->push_back(&basicBlock);
  return lists;
}

unsigned int MyFlatten::getUniqueNumber(std::vector<unsigned int> *rand_list) {
  unsigned int num = rd();
  while (true) {
    bool state = true;
    for (unsigned int &n : *rand_list)
      if (n == num) {
        state = false;
        break;
      }
    if (state)
      break;
    num = rd();
  }
  return num;
}

bool MyFlatten::valueEscapes(Instruction *Inst) {
  const BasicBlock *BB = Inst->getParent();
  return std::any_of( Inst->users().begin(), Inst->users().end(), [&BB](const User *U){
    const auto *UI = cast<Instruction>(U);
    return UI->getParent() != BB || isa<PHINode>(UI);
  });
}

void MyFlatten::DoFlatten(Function *f, unsigned long seed, unsigned int enderNum) {
  srand(seed);
  std::vector<BasicBlock *> origBB;
  getBlocks(f, &origBB);
  if (origBB.size() <= 1)
    return;
  unsigned long rand_val = seed;
  std::random_device rd2(std::to_string(seed));
  BasicBlock *oldEntry = &*f->begin();
  origBB.erase(origBB.begin());
  BranchInst *firstBr = nullptr;
  if (isa<BranchInst>(oldEntry->getTerminator()))
    firstBr = cast<BranchInst>(oldEntry->getTerminator());
  BasicBlock *firstBB = oldEntry->getTerminator()->getSuccessor(0);
  if ((firstBr != nullptr && firstBr->isConditional()) ||
      oldEntry->getTerminator()->getNumSuccessors() >
          2) // Split the first basic block
  {
    BasicBlock::iterator iter = oldEntry->end();
    iter--;
    if (oldEntry->size() > 1)
      iter--;
    BasicBlock *splitted = oldEntry->splitBasicBlock(iter, Twine("FirstBB"));
    firstBB = splitted;
    origBB.insert(origBB.begin(), splitted);
  }
  unsigned int retBlockNum = 0;
  for (auto bb : origBB) {
    if (bb->getTerminator()->getNumSuccessors() == 0)
      retBlockNum++;
  }
  unsigned int loopEndNum =
      (enderNum >= (origBB.size() - retBlockNum) ? (origBB.size() - retBlockNum)
                                                 : enderNum);
  BasicBlock *newEntry = oldEntry; // Prepare basic block
  BasicBlock *loopBegin =
      BasicBlock::Create(f->getContext(), "LoopBegin", f, newEntry);
  std::vector<BasicBlock *> loopEndBlocks;
  for (unsigned int i = 0; i < loopEndNum; i++) {
    BasicBlock *tmp =
        BasicBlock::Create(f->getContext(), "LoopEnd", f, newEntry);
    loopEndBlocks.push_back(tmp);
    BranchInst::Create(loopBegin, tmp);
  }

  newEntry->moveBefore(loopBegin);
  newEntry->getTerminator()->eraseFromParent();
  BranchInst::Create(loopBegin, newEntry);

  auto *switchVar =
      new AllocaInst(Type::getInt32Ty(f->getContext()), 0, Twine("switchVar"),
                     newEntry->getTerminator()); // Create switch variable
  std::map<BasicBlock *, TreeNode *> bb_map;
  std::map<BasicBlock *, unsigned int> nums_map;
  spawnRandomIf(loopBegin, &origBB, switchVar, &bb_map);
  unsigned int startNum = 0;
  for (auto bb : origBB) {
    unsigned int l = bb_map[bb]->l, r = bb_map[bb]->r;
    unsigned int val = rd2() % (r - l) + l;
    nums_map[bb] = val;
    if (bb == firstBB)
      startNum = val;
  }
  int every = (int)((double)(origBB.size() - retBlockNum) / (double)loopEndNum);

  int counter = 0;
  auto end_iter = loopEndBlocks.begin();
  for (auto block : origBB) // Handle successors
  {
    BasicBlock *loopEnd = *end_iter;
    if (block->getTerminator()->getNumSuccessors() == 1) {
      // errs()<<"This block has 1 successor\n";
      BasicBlock *succ = block->getTerminator()->getSuccessor(0);
      auto *caseNum = cast<ConstantInt>(
          ConstantInt::get(Type::getInt32Ty(f->getContext()), nums_map[succ]));
      block->getTerminator()->eraseFromParent();
      new StoreInst(caseNum, switchVar, block);
      BranchInst::Create(loopEnd, block);
      counter++;
    } else if (block->getTerminator()->getNumSuccessors() == 2) {
      // errs()<<"This block has 2 successors\n";
      BasicBlock *succTrue = block->getTerminator()->getSuccessor(0);
      BasicBlock *succFalse = block->getTerminator()->getSuccessor(1);
      auto *numTrue = cast<ConstantInt>(ConstantInt::get(
          Type::getInt32Ty(f->getContext()), nums_map[succTrue]));
      auto *numFalse = cast<ConstantInt>(ConstantInt::get(
          Type::getInt32Ty(f->getContext()), nums_map[succFalse]));
      auto *oldBr = cast<BranchInst>(block->getTerminator());
      SelectInst *select =
          SelectInst::Create(oldBr->getCondition(), numTrue, numFalse,
                             Twine("choice"), block->getTerminator());
      block->getTerminator()->eraseFromParent();
      new StoreInst(select, switchVar, block);
      BranchInst::Create(loopEnd, block);
      counter++;
    }
    if (counter == every) {
      counter = 0;
      end_iter++;
      if (end_iter == loopEndBlocks.end())
        end_iter--;
    }
  }
  auto *startVal = cast<ConstantInt>(ConstantInt::get(
      Type::getInt32Ty(f->getContext()), startNum)); // Set the entry value
  new StoreInst(startVal, switchVar, newEntry->getTerminator());
  std::vector<PHINode *> tmpPhi;
  std::vector<Instruction *> tmpReg;
  BasicBlock *bbEntry = &*f->begin();
  do {
    tmpPhi.clear();
    tmpReg.clear();
    for (auto & i : *f) {
      for (BasicBlock::iterator j = i.begin(); j != i.end(); j++) {
        if (isa<PHINode>(j)) {
          auto *phi = cast<PHINode>(j);
          tmpPhi.push_back(phi);
          continue;
        }
        if (!(isa<AllocaInst>(j) && j->getParent() == bbEntry) &&
            (valueEscapes(&*j) || j->isUsedOutsideOfBlock(&i))) {
          tmpReg.push_back(&*j);
          continue;
        }
      }
    }
    for (auto & i : tmpReg)
      DemoteRegToStack(*i, f->begin()->getTerminator());
    for (auto & i : tmpPhi)
      DemotePHIToStack(i, f->begin()->getTerminator());
  } while (!tmpReg.empty() || !tmpPhi.empty());
}
bool MyFlatten::runOnFunction(Function &function) {
  if (enable && readAnnotate(&function).find("fla2")) {
    DoFlatten(&function, time(nullptr), 10);
    return true;
  }
  return false;
}

char MyFlatten::ID = 0;
std::random_device MyFlatten::rd;

FunctionPass *llvm::createFlatteningEnhancedPass(bool enable) {
  return new MyFlatten(enable);
}
