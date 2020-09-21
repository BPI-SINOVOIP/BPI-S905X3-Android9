//===- BitcodeReader.cpp - Internal BitcodeReader implementation ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header defines the BitcodeReader class.
//
//===----------------------------------------------------------------------===//

#include "llvm/Bitcode/ReaderWriter.h"
#include "BitReader_3_0.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/AutoUpgrade.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/GVMaterializer.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/OperandTraits.h"
#include "llvm/IR/Operator.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm;
using namespace llvm_3_0;

#define FUNC_CODE_INST_UNWIND_2_7     14
#define eh_exception_2_7             145
#define eh_selector_2_7              149

#define TYPE_BLOCK_ID_OLD_3_0         10
#define TYPE_SYMTAB_BLOCK_ID_OLD_3_0  13
#define TYPE_CODE_STRUCT_OLD_3_0      10

namespace {
  void FindExnAndSelIntrinsics(BasicBlock *BB, CallInst *&Exn,
                                      CallInst *&Sel,
                                      SmallPtrSet<BasicBlock*, 8> &Visited) {
    if (!Visited.insert(BB).second) return;

    for (BasicBlock::iterator
           I = BB->begin(), E = BB->end(); I != E; ++I) {
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        switch (CI->getCalledFunction()->getIntrinsicID()) {
        default: break;
        case eh_exception_2_7:
          assert(!Exn && "Found more than one eh.exception call!");
          Exn = CI;
          break;
        case eh_selector_2_7:
          assert(!Sel && "Found more than one eh.selector call!");
          Sel = CI;
          break;
        }

        if (Exn && Sel) return;
      }
    }

    if (Exn && Sel) return;

    for (succ_iterator I = succ_begin(BB), E = succ_end(BB); I != E; ++I) {
      FindExnAndSelIntrinsics(*I, Exn, Sel, Visited);
      if (Exn && Sel) return;
    }
  }



  /// TransferClausesToLandingPadInst - Transfer the exception handling clauses
  /// from the eh_selector call to the new landingpad instruction.
  void TransferClausesToLandingPadInst(LandingPadInst *LPI,
                                              CallInst *EHSel) {
    LLVMContext &Context = LPI->getContext();
    unsigned N = EHSel->getNumArgOperands();

    for (unsigned i = N - 1; i > 1; --i) {
      if (const ConstantInt *CI = dyn_cast<ConstantInt>(EHSel->getArgOperand(i))){
        unsigned FilterLength = CI->getZExtValue();
        unsigned FirstCatch = i + FilterLength + !FilterLength;
        assert(FirstCatch <= N && "Invalid filter length");

        if (FirstCatch < N)
          for (unsigned j = FirstCatch; j < N; ++j) {
            Value *Val = EHSel->getArgOperand(j);
            if (!Val->hasName() || Val->getName() != "llvm.eh.catch.all.value") {
              LPI->addClause(cast<Constant>(EHSel->getArgOperand(j)));
            } else {
              GlobalVariable *GV = cast<GlobalVariable>(Val);
              LPI->addClause(GV->getInitializer());
            }
          }

        if (!FilterLength) {
          // Cleanup.
          LPI->setCleanup(true);
        } else {
          // Filter.
          SmallVector<Constant *, 4> TyInfo;
          TyInfo.reserve(FilterLength - 1);
          for (unsigned j = i + 1; j < FirstCatch; ++j)
            TyInfo.push_back(cast<Constant>(EHSel->getArgOperand(j)));
          ArrayType *AType =
            ArrayType::get(!TyInfo.empty() ? TyInfo[0]->getType() :
                           PointerType::getUnqual(Type::getInt8Ty(Context)),
                           TyInfo.size());
          LPI->addClause(ConstantArray::get(AType, TyInfo));
        }

        N = i;
      }
    }

    if (N > 2)
      for (unsigned j = 2; j < N; ++j) {
        Value *Val = EHSel->getArgOperand(j);
        if (!Val->hasName() || Val->getName() != "llvm.eh.catch.all.value") {
          LPI->addClause(cast<Constant>(EHSel->getArgOperand(j)));
        } else {
          GlobalVariable *GV = cast<GlobalVariable>(Val);
          LPI->addClause(GV->getInitializer());
        }
      }
  }


  /// This function upgrades the old pre-3.0 exception handling system to the new
  /// one. N.B. This will be removed in 3.1.
  void UpgradeExceptionHandling(Module *M) {
    Function *EHException = M->getFunction("llvm.eh.exception");
    Function *EHSelector = M->getFunction("llvm.eh.selector");
    if (!EHException || !EHSelector)
      return;

    LLVMContext &Context = M->getContext();
    Type *ExnTy = PointerType::getUnqual(Type::getInt8Ty(Context));
    Type *SelTy = Type::getInt32Ty(Context);
    Type *LPadSlotTy = StructType::get(ExnTy, SelTy, nullptr);

    // This map links the invoke instruction with the eh.exception and eh.selector
    // calls associated with it.
    DenseMap<InvokeInst*, std::pair<Value*, Value*> > InvokeToIntrinsicsMap;
    for (Module::iterator
           I = M->begin(), E = M->end(); I != E; ++I) {
      Function &F = *I;

      for (Function::iterator
             II = F.begin(), IE = F.end(); II != IE; ++II) {
        BasicBlock *BB = &*II;
        InvokeInst *Inst = dyn_cast<InvokeInst>(BB->getTerminator());
        if (!Inst) continue;
        BasicBlock *UnwindDest = Inst->getUnwindDest();
        if (UnwindDest->isLandingPad()) continue; // Already converted.

        SmallPtrSet<BasicBlock*, 8> Visited;
        CallInst *Exn = 0;
        CallInst *Sel = 0;
        FindExnAndSelIntrinsics(UnwindDest, Exn, Sel, Visited);
        assert(Exn && Sel && "Cannot find eh.exception and eh.selector calls!");
        InvokeToIntrinsicsMap[Inst] = std::make_pair(Exn, Sel);
      }
    }

    // This map stores the slots where the exception object and selector value are
    // stored within a function.
    DenseMap<Function*, std::pair<Value*, Value*> > FnToLPadSlotMap;
    SmallPtrSet<Instruction*, 32> DeadInsts;
    for (DenseMap<InvokeInst*, std::pair<Value*, Value*> >::iterator
           I = InvokeToIntrinsicsMap.begin(), E = InvokeToIntrinsicsMap.end();
         I != E; ++I) {
      InvokeInst *Invoke = I->first;
      BasicBlock *UnwindDest = Invoke->getUnwindDest();
      Function *F = UnwindDest->getParent();
      std::pair<Value*, Value*> EHIntrinsics = I->second;
      CallInst *Exn = cast<CallInst>(EHIntrinsics.first);
      CallInst *Sel = cast<CallInst>(EHIntrinsics.second);

      // Store the exception object and selector value in the entry block.
      Value *ExnSlot = 0;
      Value *SelSlot = 0;
      if (!FnToLPadSlotMap[F].first) {
        BasicBlock *Entry = &F->front();
        ExnSlot = new AllocaInst(ExnTy, "exn", Entry->getTerminator());
        SelSlot = new AllocaInst(SelTy, "sel", Entry->getTerminator());
        FnToLPadSlotMap[F] = std::make_pair(ExnSlot, SelSlot);
      } else {
        ExnSlot = FnToLPadSlotMap[F].first;
        SelSlot = FnToLPadSlotMap[F].second;
      }

      if (!UnwindDest->getSinglePredecessor()) {
        // The unwind destination doesn't have a single predecessor. Create an
        // unwind destination which has only one predecessor.
        BasicBlock *NewBB = BasicBlock::Create(Context, "new.lpad",
                                               UnwindDest->getParent());
        BranchInst::Create(UnwindDest, NewBB);
        Invoke->setUnwindDest(NewBB);

        // Fix up any PHIs in the original unwind destination block.
        for (BasicBlock::iterator
               II = UnwindDest->begin(); isa<PHINode>(II); ++II) {
          PHINode *PN = cast<PHINode>(II);
          int Idx = PN->getBasicBlockIndex(Invoke->getParent());
          if (Idx == -1) continue;
          PN->setIncomingBlock(Idx, NewBB);
        }

        UnwindDest = NewBB;
      }

      IRBuilder<> Builder(Context);
      Builder.SetInsertPoint(UnwindDest, UnwindDest->getFirstInsertionPt());

      LandingPadInst *LPI = Builder.CreateLandingPad(LPadSlotTy, 0);
      Value *LPExn = Builder.CreateExtractValue(LPI, 0);
      Value *LPSel = Builder.CreateExtractValue(LPI, 1);
      Builder.CreateStore(LPExn, ExnSlot);
      Builder.CreateStore(LPSel, SelSlot);

      TransferClausesToLandingPadInst(LPI, Sel);

      DeadInsts.insert(Exn);
      DeadInsts.insert(Sel);
    }

    // Replace the old intrinsic calls with the values from the landingpad
    // instruction(s). These values were stored in allocas for us to use here.
    for (DenseMap<InvokeInst*, std::pair<Value*, Value*> >::iterator
           I = InvokeToIntrinsicsMap.begin(), E = InvokeToIntrinsicsMap.end();
         I != E; ++I) {
      std::pair<Value*, Value*> EHIntrinsics = I->second;
      CallInst *Exn = cast<CallInst>(EHIntrinsics.first);
      CallInst *Sel = cast<CallInst>(EHIntrinsics.second);
      BasicBlock *Parent = Exn->getParent();

      std::pair<Value*,Value*> ExnSelSlots = FnToLPadSlotMap[Parent->getParent()];

      IRBuilder<> Builder(Context);
      Builder.SetInsertPoint(Parent, Exn->getIterator());
      LoadInst *LPExn = Builder.CreateLoad(ExnSelSlots.first, "exn.load");
      LoadInst *LPSel = Builder.CreateLoad(ExnSelSlots.second, "sel.load");

      Exn->replaceAllUsesWith(LPExn);
      Sel->replaceAllUsesWith(LPSel);
    }

    // Remove the dead instructions.
    for (SmallPtrSet<Instruction*, 32>::iterator
           I = DeadInsts.begin(), E = DeadInsts.end(); I != E; ++I) {
      Instruction *Inst = *I;
      Inst->eraseFromParent();
    }

    // Replace calls to "llvm.eh.resume" with the 'resume' instruction. Load the
    // exception and selector values from the stored place.
    Function *EHResume = M->getFunction("llvm.eh.resume");
    if (!EHResume) return;

    while (!EHResume->use_empty()) {
      CallInst *Resume = cast<CallInst>(*EHResume->use_begin());
      BasicBlock *BB = Resume->getParent();

      IRBuilder<> Builder(Context);
      Builder.SetInsertPoint(BB, Resume->getIterator());

      Value *LPadVal =
        Builder.CreateInsertValue(UndefValue::get(LPadSlotTy),
                                  Resume->getArgOperand(0), 0, "lpad.val");
      LPadVal = Builder.CreateInsertValue(LPadVal, Resume->getArgOperand(1),
                                          1, "lpad.val");
      Builder.CreateResume(LPadVal);

      // Remove all instructions after the 'resume.'
      BasicBlock::iterator I = Resume->getIterator();
      while (I != BB->end()) {
        Instruction *Inst = &*I++;
        Inst->eraseFromParent();
      }
    }
  }


  void StripDebugInfoOfFunction(Module* M, const char* name) {
    if (Function* FuncStart = M->getFunction(name)) {
      while (!FuncStart->use_empty()) {
        cast<CallInst>(*FuncStart->use_begin())->eraseFromParent();
      }
      FuncStart->eraseFromParent();
    }
  }

  /// This function strips all debug info intrinsics, except for llvm.dbg.declare.
  /// If an llvm.dbg.declare intrinsic is invalid, then this function simply
  /// strips that use.
  void CheckDebugInfoIntrinsics(Module *M) {
    StripDebugInfoOfFunction(M, "llvm.dbg.func.start");
    StripDebugInfoOfFunction(M, "llvm.dbg.stoppoint");
    StripDebugInfoOfFunction(M, "llvm.dbg.region.start");
    StripDebugInfoOfFunction(M, "llvm.dbg.region.end");

    if (Function *Declare = M->getFunction("llvm.dbg.declare")) {
      if (!Declare->use_empty()) {
        DbgDeclareInst *DDI = cast<DbgDeclareInst>(*Declare->use_begin());
        if (!isa<MDNode>(ValueAsMetadata::get(DDI->getArgOperand(0))) ||
            !isa<MDNode>(ValueAsMetadata::get(DDI->getArgOperand(1)))) {
          while (!Declare->use_empty()) {
            CallInst *CI = cast<CallInst>(*Declare->use_begin());
            CI->eraseFromParent();
          }
          Declare->eraseFromParent();
        }
      }
    }
  }


//===----------------------------------------------------------------------===//
//                          BitcodeReaderValueList Class
//===----------------------------------------------------------------------===//

class BitcodeReaderValueList {
  std::vector<WeakVH> ValuePtrs;

  /// ResolveConstants - As we resolve forward-referenced constants, we add
  /// information about them to this vector.  This allows us to resolve them in
  /// bulk instead of resolving each reference at a time.  See the code in
  /// ResolveConstantForwardRefs for more information about this.
  ///
  /// The key of this vector is the placeholder constant, the value is the slot
  /// number that holds the resolved value.
  typedef std::vector<std::pair<Constant*, unsigned> > ResolveConstantsTy;
  ResolveConstantsTy ResolveConstants;
  LLVMContext &Context;
public:
  explicit BitcodeReaderValueList(LLVMContext &C) : Context(C) {}
  ~BitcodeReaderValueList() {
    assert(ResolveConstants.empty() && "Constants not resolved?");
  }

  // vector compatibility methods
  unsigned size() const { return ValuePtrs.size(); }
  void resize(unsigned N) { ValuePtrs.resize(N); }
  void push_back(Value *V) {
    ValuePtrs.push_back(V);
  }

  void clear() {
    assert(ResolveConstants.empty() && "Constants not resolved?");
    ValuePtrs.clear();
  }

  Value *operator[](unsigned i) const {
    assert(i < ValuePtrs.size());
    return ValuePtrs[i];
  }

  Value *back() const { return ValuePtrs.back(); }
    void pop_back() { ValuePtrs.pop_back(); }
  bool empty() const { return ValuePtrs.empty(); }
  void shrinkTo(unsigned N) {
    assert(N <= size() && "Invalid shrinkTo request!");
    ValuePtrs.resize(N);
  }

  Constant *getConstantFwdRef(unsigned Idx, Type *Ty);
  Value *getValueFwdRef(unsigned Idx, Type *Ty);

  void AssignValue(Value *V, unsigned Idx);

  /// ResolveConstantForwardRefs - Once all constants are read, this method bulk
  /// resolves any forward references.
  void ResolveConstantForwardRefs();
};


//===----------------------------------------------------------------------===//
//                          BitcodeReaderMDValueList Class
//===----------------------------------------------------------------------===//

class BitcodeReaderMDValueList {
  unsigned NumFwdRefs;
  bool AnyFwdRefs;
  std::vector<TrackingMDRef> MDValuePtrs;

  LLVMContext &Context;
public:
  explicit BitcodeReaderMDValueList(LLVMContext &C)
      : NumFwdRefs(0), AnyFwdRefs(false), Context(C) {}

  // vector compatibility methods
  unsigned size() const       { return MDValuePtrs.size(); }
  void resize(unsigned N)     { MDValuePtrs.resize(N); }
  void push_back(Metadata *MD) { MDValuePtrs.emplace_back(MD); }
  void clear()                { MDValuePtrs.clear();  }
  Metadata *back() const      { return MDValuePtrs.back(); }
  void pop_back()             { MDValuePtrs.pop_back(); }
  bool empty() const          { return MDValuePtrs.empty(); }

  Metadata *operator[](unsigned i) const {
    assert(i < MDValuePtrs.size());
    return MDValuePtrs[i];
  }

  void shrinkTo(unsigned N) {
    assert(N <= size() && "Invalid shrinkTo request!");
    MDValuePtrs.resize(N);
  }

  Metadata *getValueFwdRef(unsigned Idx);
  void AssignValue(Metadata *MD, unsigned Idx);
  void tryToResolveCycles();
};

class BitcodeReader : public GVMaterializer {
  LLVMContext &Context;
  DiagnosticHandlerFunction DiagnosticHandler;
  Module *TheModule;
  std::unique_ptr<MemoryBuffer> Buffer;
  std::unique_ptr<BitstreamReader> StreamFile;
  BitstreamCursor Stream;
  std::unique_ptr<DataStreamer> LazyStreamer;
  uint64_t NextUnreadBit;
  bool SeenValueSymbolTable;

  std::vector<Type*> TypeList;
  BitcodeReaderValueList ValueList;
  BitcodeReaderMDValueList MDValueList;
  SmallVector<Instruction *, 64> InstructionList;

  std::vector<std::pair<GlobalVariable*, unsigned> > GlobalInits;
  std::vector<std::pair<GlobalAlias*, unsigned> > AliasInits;

  /// MAttributes - The set of attributes by index.  Index zero in the
  /// file is for null, and is thus not represented here.  As such all indices
  /// are off by one.
  std::vector<AttributeSet> MAttributes;

  /// \brief The set of attribute groups.
  std::map<unsigned, AttributeSet> MAttributeGroups;

  /// FunctionBBs - While parsing a function body, this is a list of the basic
  /// blocks for the function.
  std::vector<BasicBlock*> FunctionBBs;

  // When reading the module header, this list is populated with functions that
  // have bodies later in the file.
  std::vector<Function*> FunctionsWithBodies;

  // When intrinsic functions are encountered which require upgrading they are
  // stored here with their replacement function.
  typedef std::vector<std::pair<Function*, Function*> > UpgradedIntrinsicMap;
  UpgradedIntrinsicMap UpgradedIntrinsics;

  // Map the bitcode's custom MDKind ID to the Module's MDKind ID.
  DenseMap<unsigned, unsigned> MDKindMap;

  // Several operations happen after the module header has been read, but
  // before function bodies are processed. This keeps track of whether
  // we've done this yet.
  bool SeenFirstFunctionBody;

  /// DeferredFunctionInfo - When function bodies are initially scanned, this
  /// map contains info about where to find deferred function body in the
  /// stream.
  DenseMap<Function*, uint64_t> DeferredFunctionInfo;

  /// BlockAddrFwdRefs - These are blockaddr references to basic blocks.  These
  /// are resolved lazily when functions are loaded.
  typedef std::pair<unsigned, GlobalVariable*> BlockAddrRefTy;
  DenseMap<Function*, std::vector<BlockAddrRefTy> > BlockAddrFwdRefs;

  static const std::error_category &BitcodeErrorCategory();

public:
  std::error_code Error(BitcodeError E, const Twine &Message);
  std::error_code Error(BitcodeError E);
  std::error_code Error(const Twine &Message);

  explicit BitcodeReader(MemoryBuffer *buffer, LLVMContext &C,
                         DiagnosticHandlerFunction DiagnosticHandler);
  ~BitcodeReader() { FreeState(); }

  void FreeState();

  void releaseBuffer();

  bool isDematerializable(const GlobalValue *GV) const;
  std::error_code materialize(GlobalValue *GV) override;
  std::error_code materializeModule() override;
  std::vector<StructType *> getIdentifiedStructTypes() const override;
  void dematerialize(GlobalValue *GV);

  /// @brief Main interface to parsing a bitcode buffer.
  /// @returns true if an error occurred.
  std::error_code ParseBitcodeInto(Module *M);

  /// @brief Cheap mechanism to just extract module triple
  /// @returns true if an error occurred.
  llvm::ErrorOr<std::string> parseTriple();

  static uint64_t decodeSignRotatedValue(uint64_t V);

  /// Materialize any deferred Metadata block.
  std::error_code materializeMetadata() override;

  void setStripDebugInfo() override;

private:
  std::vector<StructType *> IdentifiedStructTypes;
  StructType *createIdentifiedStructType(LLVMContext &Context, StringRef Name);
  StructType *createIdentifiedStructType(LLVMContext &Context);

  Type *getTypeByID(unsigned ID);
  Type *getTypeByIDOrNull(unsigned ID);
  Value *getFnValueByID(unsigned ID, Type *Ty) {
    if (Ty && Ty->isMetadataTy())
      return MetadataAsValue::get(Ty->getContext(), getFnMetadataByID(ID));
    return ValueList.getValueFwdRef(ID, Ty);
  }
  Metadata *getFnMetadataByID(unsigned ID) {
    return MDValueList.getValueFwdRef(ID);
  }
  BasicBlock *getBasicBlock(unsigned ID) const {
    if (ID >= FunctionBBs.size()) return nullptr; // Invalid ID
    return FunctionBBs[ID];
  }
  AttributeSet getAttributes(unsigned i) const {
    if (i-1 < MAttributes.size())
      return MAttributes[i-1];
    return AttributeSet();
  }

  /// getValueTypePair - Read a value/type pair out of the specified record from
  /// slot 'Slot'.  Increment Slot past the number of slots used in the record.
  /// Return true on failure.
  bool getValueTypePair(SmallVectorImpl<uint64_t> &Record, unsigned &Slot,
                        unsigned InstNum, Value *&ResVal) {
    if (Slot == Record.size()) return true;
    unsigned ValNo = (unsigned)Record[Slot++];
    if (ValNo < InstNum) {
      // If this is not a forward reference, just return the value we already
      // have.
      ResVal = getFnValueByID(ValNo, nullptr);
      return ResVal == nullptr;
    } else if (Slot == Record.size()) {
      return true;
    }

    unsigned TypeNo = (unsigned)Record[Slot++];
    ResVal = getFnValueByID(ValNo, getTypeByID(TypeNo));
    return ResVal == nullptr;
  }
  bool getValue(SmallVector<uint64_t, 64> &Record, unsigned &Slot,
                Type *Ty, Value *&ResVal) {
    if (Slot == Record.size()) return true;
    unsigned ValNo = (unsigned)Record[Slot++];
    ResVal = getFnValueByID(ValNo, Ty);
    return ResVal == 0;
  }


  std::error_code ParseModule(bool Resume);
  std::error_code ParseAttributeBlock();
  std::error_code ParseTypeTable();
  std::error_code ParseOldTypeTable();         // FIXME: Remove in LLVM 3.1
  std::error_code ParseTypeTableBody();

  std::error_code ParseOldTypeSymbolTable();   // FIXME: Remove in LLVM 3.1
  std::error_code ParseValueSymbolTable();
  std::error_code ParseConstants();
  std::error_code RememberAndSkipFunctionBody();
  std::error_code ParseFunctionBody(Function *F);
  std::error_code GlobalCleanup();
  std::error_code ResolveGlobalAndAliasInits();
  std::error_code ParseMetadata();
  std::error_code ParseMetadataAttachment();
  llvm::ErrorOr<std::string> parseModuleTriple();
  std::error_code InitStream();
  std::error_code InitStreamFromBuffer();
  std::error_code InitLazyStream();
};

} // end anonymous namespace

static std::error_code Error(const DiagnosticHandlerFunction &DiagnosticHandler,
                             std::error_code EC, const Twine &Message) {
  BitcodeDiagnosticInfo DI(EC, DS_Error, Message);
  DiagnosticHandler(DI);
  return EC;
}

static std::error_code Error(const DiagnosticHandlerFunction &DiagnosticHandler,
                             std::error_code EC) {
  return Error(DiagnosticHandler, EC, EC.message());
}

std::error_code BitcodeReader::Error(BitcodeError E, const Twine &Message) {
  return ::Error(DiagnosticHandler, make_error_code(E), Message);
}

std::error_code BitcodeReader::Error(const Twine &Message) {
  return ::Error(DiagnosticHandler,
                 make_error_code(BitcodeError::CorruptedBitcode), Message);
}

std::error_code BitcodeReader::Error(BitcodeError E) {
  return ::Error(DiagnosticHandler, make_error_code(E));
}

static DiagnosticHandlerFunction getDiagHandler(DiagnosticHandlerFunction F,
                                                LLVMContext &C) {
  if (F)
    return F;
  return [&C](const DiagnosticInfo &DI) { C.diagnose(DI); };
}

BitcodeReader::BitcodeReader(MemoryBuffer *buffer, LLVMContext &C,
                             DiagnosticHandlerFunction DiagnosticHandler)
    : Context(C), DiagnosticHandler(getDiagHandler(DiagnosticHandler, C)),
      TheModule(nullptr), Buffer(buffer), LazyStreamer(nullptr),
      NextUnreadBit(0), SeenValueSymbolTable(false), ValueList(C),
      MDValueList(C), SeenFirstFunctionBody(false) {}


void BitcodeReader::FreeState() {
  Buffer = nullptr;
  std::vector<Type*>().swap(TypeList);
  ValueList.clear();
  MDValueList.clear();

  std::vector<AttributeSet>().swap(MAttributes);
  std::vector<BasicBlock*>().swap(FunctionBBs);
  std::vector<Function*>().swap(FunctionsWithBodies);
  DeferredFunctionInfo.clear();
  MDKindMap.clear();
}

//===----------------------------------------------------------------------===//
//  Helper functions to implement forward reference resolution, etc.
//===----------------------------------------------------------------------===//

/// ConvertToString - Convert a string from a record into an std::string, return
/// true on failure.
template<typename StrTy>
static bool ConvertToString(ArrayRef<uint64_t> Record, unsigned Idx,
                            StrTy &Result) {
  if (Idx > Record.size())
    return true;

  for (unsigned i = Idx, e = Record.size(); i != e; ++i)
    Result += (char)Record[i];
  return false;
}

static GlobalValue::LinkageTypes getDecodedLinkage(unsigned Val) {
  switch (Val) {
  default: // Map unknown/new linkages to external
  case 0:
    return GlobalValue::ExternalLinkage;
  case 1:
    return GlobalValue::WeakAnyLinkage;
  case 2:
    return GlobalValue::AppendingLinkage;
  case 3:
    return GlobalValue::InternalLinkage;
  case 4:
    return GlobalValue::LinkOnceAnyLinkage;
  case 5:
    return GlobalValue::ExternalLinkage; // Obsolete DLLImportLinkage
  case 6:
    return GlobalValue::ExternalLinkage; // Obsolete DLLExportLinkage
  case 7:
    return GlobalValue::ExternalWeakLinkage;
  case 8:
    return GlobalValue::CommonLinkage;
  case 9:
    return GlobalValue::PrivateLinkage;
  case 10:
    return GlobalValue::WeakODRLinkage;
  case 11:
    return GlobalValue::LinkOnceODRLinkage;
  case 12:
    return GlobalValue::AvailableExternallyLinkage;
  case 13:
    return GlobalValue::PrivateLinkage; // Obsolete LinkerPrivateLinkage
  case 14:
    return GlobalValue::ExternalWeakLinkage; // Obsolete LinkerPrivateWeakLinkage
  //ANDROID: convert LinkOnceODRAutoHideLinkage -> LinkOnceODRLinkage
  case 15:
    return GlobalValue::LinkOnceODRLinkage;
  }
}

static GlobalValue::VisibilityTypes GetDecodedVisibility(unsigned Val) {
  switch (Val) {
  default: // Map unknown visibilities to default.
  case 0: return GlobalValue::DefaultVisibility;
  case 1: return GlobalValue::HiddenVisibility;
  case 2: return GlobalValue::ProtectedVisibility;
  }
}

static GlobalVariable::ThreadLocalMode GetDecodedThreadLocalMode(unsigned Val) {
  switch (Val) {
    case 0: return GlobalVariable::NotThreadLocal;
    default: // Map unknown non-zero value to general dynamic.
    case 1: return GlobalVariable::GeneralDynamicTLSModel;
    case 2: return GlobalVariable::LocalDynamicTLSModel;
    case 3: return GlobalVariable::InitialExecTLSModel;
    case 4: return GlobalVariable::LocalExecTLSModel;
  }
}

static GlobalVariable::UnnamedAddr getDecodedUnnamedAddrType(unsigned Val) {
  switch (Val) {
    default: // Map unknown to UnnamedAddr::None.
    case 0: return GlobalVariable::UnnamedAddr::None;
    case 1: return GlobalVariable::UnnamedAddr::Global;
    case 2: return GlobalVariable::UnnamedAddr::Local;
  }
}

static int GetDecodedCastOpcode(unsigned Val) {
  switch (Val) {
  default: return -1;
  case bitc::CAST_TRUNC   : return Instruction::Trunc;
  case bitc::CAST_ZEXT    : return Instruction::ZExt;
  case bitc::CAST_SEXT    : return Instruction::SExt;
  case bitc::CAST_FPTOUI  : return Instruction::FPToUI;
  case bitc::CAST_FPTOSI  : return Instruction::FPToSI;
  case bitc::CAST_UITOFP  : return Instruction::UIToFP;
  case bitc::CAST_SITOFP  : return Instruction::SIToFP;
  case bitc::CAST_FPTRUNC : return Instruction::FPTrunc;
  case bitc::CAST_FPEXT   : return Instruction::FPExt;
  case bitc::CAST_PTRTOINT: return Instruction::PtrToInt;
  case bitc::CAST_INTTOPTR: return Instruction::IntToPtr;
  case bitc::CAST_BITCAST : return Instruction::BitCast;
  }
}
static int GetDecodedBinaryOpcode(unsigned Val, Type *Ty) {
  switch (Val) {
  default: return -1;
  case bitc::BINOP_ADD:
    return Ty->isFPOrFPVectorTy() ? Instruction::FAdd : Instruction::Add;
  case bitc::BINOP_SUB:
    return Ty->isFPOrFPVectorTy() ? Instruction::FSub : Instruction::Sub;
  case bitc::BINOP_MUL:
    return Ty->isFPOrFPVectorTy() ? Instruction::FMul : Instruction::Mul;
  case bitc::BINOP_UDIV: return Instruction::UDiv;
  case bitc::BINOP_SDIV:
    return Ty->isFPOrFPVectorTy() ? Instruction::FDiv : Instruction::SDiv;
  case bitc::BINOP_UREM: return Instruction::URem;
  case bitc::BINOP_SREM:
    return Ty->isFPOrFPVectorTy() ? Instruction::FRem : Instruction::SRem;
  case bitc::BINOP_SHL:  return Instruction::Shl;
  case bitc::BINOP_LSHR: return Instruction::LShr;
  case bitc::BINOP_ASHR: return Instruction::AShr;
  case bitc::BINOP_AND:  return Instruction::And;
  case bitc::BINOP_OR:   return Instruction::Or;
  case bitc::BINOP_XOR:  return Instruction::Xor;
  }
}

static AtomicRMWInst::BinOp GetDecodedRMWOperation(unsigned Val) {
  switch (Val) {
  default: return AtomicRMWInst::BAD_BINOP;
  case bitc::RMW_XCHG: return AtomicRMWInst::Xchg;
  case bitc::RMW_ADD: return AtomicRMWInst::Add;
  case bitc::RMW_SUB: return AtomicRMWInst::Sub;
  case bitc::RMW_AND: return AtomicRMWInst::And;
  case bitc::RMW_NAND: return AtomicRMWInst::Nand;
  case bitc::RMW_OR: return AtomicRMWInst::Or;
  case bitc::RMW_XOR: return AtomicRMWInst::Xor;
  case bitc::RMW_MAX: return AtomicRMWInst::Max;
  case bitc::RMW_MIN: return AtomicRMWInst::Min;
  case bitc::RMW_UMAX: return AtomicRMWInst::UMax;
  case bitc::RMW_UMIN: return AtomicRMWInst::UMin;
  }
}

static AtomicOrdering GetDecodedOrdering(unsigned Val) {
  switch (Val) {
  case bitc::ORDERING_NOTATOMIC: return AtomicOrdering::NotAtomic;
  case bitc::ORDERING_UNORDERED: return AtomicOrdering::Unordered;
  case bitc::ORDERING_MONOTONIC: return AtomicOrdering::Monotonic;
  case bitc::ORDERING_ACQUIRE: return AtomicOrdering::Acquire;
  case bitc::ORDERING_RELEASE: return AtomicOrdering::Release;
  case bitc::ORDERING_ACQREL: return AtomicOrdering::AcquireRelease;
  default: // Map unknown orderings to sequentially-consistent.
  case bitc::ORDERING_SEQCST: return AtomicOrdering::SequentiallyConsistent;
  }
}

static SynchronizationScope GetDecodedSynchScope(unsigned Val) {
  switch (Val) {
  case bitc::SYNCHSCOPE_SINGLETHREAD: return SingleThread;
  default: // Map unknown scopes to cross-thread.
  case bitc::SYNCHSCOPE_CROSSTHREAD: return CrossThread;
  }
}

namespace llvm {
namespace {
  /// @brief A class for maintaining the slot number definition
  /// as a placeholder for the actual definition for forward constants defs.
  class ConstantPlaceHolder : public ConstantExpr {
    void operator=(const ConstantPlaceHolder &) = delete;
  public:
    // allocate space for exactly one operand
    void *operator new(size_t s) {
      return User::operator new(s, 1);
    }
    explicit ConstantPlaceHolder(Type *Ty, LLVMContext& Context)
      : ConstantExpr(Ty, Instruction::UserOp1, &Op<0>(), 1) {
      Op<0>() = UndefValue::get(Type::getInt32Ty(Context));
    }

    /// @brief Methods to support type inquiry through isa, cast, and dyn_cast.
    static bool classof(const Value *V) {
      return isa<ConstantExpr>(V) &&
             cast<ConstantExpr>(V)->getOpcode() == Instruction::UserOp1;
    }


    /// Provide fast operand accessors
    DECLARE_TRANSPARENT_OPERAND_ACCESSORS(Value);
  };
}

// FIXME: can we inherit this from ConstantExpr?
template <>
struct OperandTraits<ConstantPlaceHolder> :
  public FixedNumOperandTraits<ConstantPlaceHolder, 1> {
};
DEFINE_TRANSPARENT_OPERAND_ACCESSORS(ConstantPlaceHolder, Value)
}


void BitcodeReaderValueList::AssignValue(Value *V, unsigned Idx) {
  if (Idx == size()) {
    push_back(V);
    return;
  }

  if (Idx >= size())
    resize(Idx+1);

  WeakVH &OldV = ValuePtrs[Idx];
  if (!OldV) {
    OldV = V;
    return;
  }

  // Handle constants and non-constants (e.g. instrs) differently for
  // efficiency.
  if (Constant *PHC = dyn_cast<Constant>(&*OldV)) {
    ResolveConstants.push_back(std::make_pair(PHC, Idx));
    OldV = V;
  } else {
    // If there was a forward reference to this value, replace it.
    Value *PrevVal = OldV;
    OldV->replaceAllUsesWith(V);
    delete PrevVal;
  }
}


Constant *BitcodeReaderValueList::getConstantFwdRef(unsigned Idx,
                                                    Type *Ty) {
  if (Idx >= size())
    resize(Idx + 1);

  if (Value *V = ValuePtrs[Idx]) {
    assert(Ty == V->getType() && "Type mismatch in constant table!");
    return cast<Constant>(V);
  }

  // Create and return a placeholder, which will later be RAUW'd.
  Constant *C = new ConstantPlaceHolder(Ty, Context);
  ValuePtrs[Idx] = C;
  return C;
}

Value *BitcodeReaderValueList::getValueFwdRef(unsigned Idx, Type *Ty) {
  if (Idx >= size())
    resize(Idx + 1);

  if (Value *V = ValuePtrs[Idx]) {
    assert((!Ty || Ty == V->getType()) && "Type mismatch in value table!");
    return V;
  }

  // No type specified, must be invalid reference.
  if (!Ty) return nullptr;

  // Create and return a placeholder, which will later be RAUW'd.
  Value *V = new Argument(Ty);
  ValuePtrs[Idx] = V;
  return V;
}

/// ResolveConstantForwardRefs - Once all constants are read, this method bulk
/// resolves any forward references.  The idea behind this is that we sometimes
/// get constants (such as large arrays) which reference *many* forward ref
/// constants.  Replacing each of these causes a lot of thrashing when
/// building/reuniquing the constant.  Instead of doing this, we look at all the
/// uses and rewrite all the place holders at once for any constant that uses
/// a placeholder.
void BitcodeReaderValueList::ResolveConstantForwardRefs() {
  // Sort the values by-pointer so that they are efficient to look up with a
  // binary search.
  std::sort(ResolveConstants.begin(), ResolveConstants.end());

  SmallVector<Constant*, 64> NewOps;

  while (!ResolveConstants.empty()) {
    Value *RealVal = operator[](ResolveConstants.back().second);
    Constant *Placeholder = ResolveConstants.back().first;
    ResolveConstants.pop_back();

    // Loop over all users of the placeholder, updating them to reference the
    // new value.  If they reference more than one placeholder, update them all
    // at once.
    while (!Placeholder->use_empty()) {
      auto UI = Placeholder->user_begin();
      User *U = *UI;

      // If the using object isn't uniqued, just update the operands.  This
      // handles instructions and initializers for global variables.
      if (!isa<Constant>(U) || isa<GlobalValue>(U)) {
        UI.getUse().set(RealVal);
        continue;
      }

      // Otherwise, we have a constant that uses the placeholder.  Replace that
      // constant with a new constant that has *all* placeholder uses updated.
      Constant *UserC = cast<Constant>(U);
      for (User::op_iterator I = UserC->op_begin(), E = UserC->op_end();
           I != E; ++I) {
        Value *NewOp;
        if (!isa<ConstantPlaceHolder>(*I)) {
          // Not a placeholder reference.
          NewOp = *I;
        } else if (*I == Placeholder) {
          // Common case is that it just references this one placeholder.
          NewOp = RealVal;
        } else {
          // Otherwise, look up the placeholder in ResolveConstants.
          ResolveConstantsTy::iterator It =
            std::lower_bound(ResolveConstants.begin(), ResolveConstants.end(),
                             std::pair<Constant*, unsigned>(cast<Constant>(*I),
                                                            0));
          assert(It != ResolveConstants.end() && It->first == *I);
          NewOp = operator[](It->second);
        }

        NewOps.push_back(cast<Constant>(NewOp));
      }

      // Make the new constant.
      Constant *NewC;
      if (ConstantArray *UserCA = dyn_cast<ConstantArray>(UserC)) {
        NewC = ConstantArray::get(UserCA->getType(), NewOps);
      } else if (ConstantStruct *UserCS = dyn_cast<ConstantStruct>(UserC)) {
        NewC = ConstantStruct::get(UserCS->getType(), NewOps);
      } else if (isa<ConstantVector>(UserC)) {
        NewC = ConstantVector::get(NewOps);
      } else {
        assert(isa<ConstantExpr>(UserC) && "Must be a ConstantExpr.");
        NewC = cast<ConstantExpr>(UserC)->getWithOperands(NewOps);
      }

      UserC->replaceAllUsesWith(NewC);
      UserC->destroyConstant();
      NewOps.clear();
    }

    // Update all ValueHandles, they should be the only users at this point.
    Placeholder->replaceAllUsesWith(RealVal);
    delete Placeholder;
  }
}

void BitcodeReaderMDValueList::AssignValue(Metadata *MD, unsigned Idx) {
  if (Idx == size()) {
    push_back(MD);
    return;
  }

  if (Idx >= size())
    resize(Idx+1);

  TrackingMDRef &OldMD = MDValuePtrs[Idx];
  if (!OldMD) {
    OldMD.reset(MD);
    return;
  }

  // If there was a forward reference to this value, replace it.
  TempMDTuple PrevMD(cast<MDTuple>(OldMD.get()));
  PrevMD->replaceAllUsesWith(MD);
  --NumFwdRefs;
}

Metadata *BitcodeReaderMDValueList::getValueFwdRef(unsigned Idx) {
  if (Idx >= size())
    resize(Idx + 1);

  if (Metadata *MD = MDValuePtrs[Idx])
    return MD;

  // Create and return a placeholder, which will later be RAUW'd.
  AnyFwdRefs = true;
  ++NumFwdRefs;
  Metadata *MD = MDNode::getTemporary(Context, None).release();
  MDValuePtrs[Idx].reset(MD);
  return MD;
}

void BitcodeReaderMDValueList::tryToResolveCycles() {
  if (!AnyFwdRefs)
    // Nothing to do.
    return;

  if (NumFwdRefs)
    // Still forward references... can't resolve cycles.
    return;

  // Resolve any cycles.
  for (auto &MD : MDValuePtrs) {
    auto *N = dyn_cast_or_null<MDNode>(MD);
    if (!N)
      continue;

    assert(!N->isTemporary() && "Unexpected forward reference");
    N->resolveCycles();
  }
}

Type *BitcodeReader::getTypeByID(unsigned ID) {
  // The type table size is always specified correctly.
  if (ID >= TypeList.size())
    return nullptr;

  if (Type *Ty = TypeList[ID])
    return Ty;

  // If we have a forward reference, the only possible case is when it is to a
  // named struct.  Just create a placeholder for now.
  return TypeList[ID] = createIdentifiedStructType(Context);
}

StructType *BitcodeReader::createIdentifiedStructType(LLVMContext &Context,
                                                      StringRef Name) {
  auto *Ret = StructType::create(Context, Name);
  IdentifiedStructTypes.push_back(Ret);
  return Ret;
}

StructType *BitcodeReader::createIdentifiedStructType(LLVMContext &Context) {
  auto *Ret = StructType::create(Context);
  IdentifiedStructTypes.push_back(Ret);
  return Ret;
}


/// FIXME: Remove in LLVM 3.1, only used by ParseOldTypeTable.
Type *BitcodeReader::getTypeByIDOrNull(unsigned ID) {
  if (ID >= TypeList.size())
    TypeList.resize(ID+1);

  return TypeList[ID];
}

//===----------------------------------------------------------------------===//
//  Functions for parsing blocks from the bitcode file
//===----------------------------------------------------------------------===//


/// \brief This fills an AttrBuilder object with the LLVM attributes that have
/// been decoded from the given integer. This function must stay in sync with
/// 'encodeLLVMAttributesForBitcode'.
static void decodeLLVMAttributesForBitcode(AttrBuilder &B,
                                           uint64_t EncodedAttrs) {
  // FIXME: Remove in 4.0.

  // The alignment is stored as a 16-bit raw value from bits 31--16.  We shift
  // the bits above 31 down by 11 bits.
  unsigned Alignment = (EncodedAttrs & (0xffffULL << 16)) >> 16;
  assert((!Alignment || isPowerOf2_32(Alignment)) &&
         "Alignment must be a power of two.");

  if (Alignment)
    B.addAlignmentAttr(Alignment);
  B.addRawValue(((EncodedAttrs & (0xfffffULL << 32)) >> 11) |
                (EncodedAttrs & 0xffff));
}

std::error_code BitcodeReader::ParseAttributeBlock() {
  if (Stream.EnterSubBlock(bitc::PARAMATTR_BLOCK_ID))
    return Error("Invalid record");

  if (!MAttributes.empty())
    return Error("Invalid multiple blocks");

  SmallVector<uint64_t, 64> Record;

  SmallVector<AttributeSet, 8> Attrs;

  // Read all the records.
  while (1) {
    BitstreamEntry Entry = Stream.advanceSkippingSubblocks();

    switch (Entry.Kind) {
    case BitstreamEntry::SubBlock: // Handled for us already.
    case BitstreamEntry::Error:
      return Error("Malformed block");
    case BitstreamEntry::EndBlock:
      return std::error_code();
    case BitstreamEntry::Record:
      // The interesting case.
      break;
    }

    // Read a record.
    Record.clear();
    switch (Stream.readRecord(Entry.ID, Record)) {
    default:  // Default behavior: ignore.
      break;
    case bitc::PARAMATTR_CODE_ENTRY_OLD: { // ENTRY: [paramidx0, attr0, ...]
      // FIXME: Remove in 4.0.
      if (Record.size() & 1)
        return Error("Invalid record");

      for (unsigned i = 0, e = Record.size(); i != e; i += 2) {
        AttrBuilder B;
        decodeLLVMAttributesForBitcode(B, Record[i+1]);
        Attrs.push_back(AttributeSet::get(Context, Record[i], B));
      }

      MAttributes.push_back(AttributeSet::get(Context, Attrs));
      Attrs.clear();
      break;
    }
    case bitc::PARAMATTR_CODE_ENTRY: { // ENTRY: [attrgrp0, attrgrp1, ...]
      for (unsigned i = 0, e = Record.size(); i != e; ++i)
        Attrs.push_back(MAttributeGroups[Record[i]]);

      MAttributes.push_back(AttributeSet::get(Context, Attrs));
      Attrs.clear();
      break;
    }
    }
  }
}


std::error_code BitcodeReader::ParseTypeTable() {
  if (Stream.EnterSubBlock(bitc::TYPE_BLOCK_ID_NEW))
    return Error("Invalid record");

  return ParseTypeTableBody();
}

std::error_code BitcodeReader::ParseTypeTableBody() {
  if (!TypeList.empty())
    return Error("Invalid multiple blocks");

  SmallVector<uint64_t, 64> Record;
  unsigned NumRecords = 0;

  SmallString<64> TypeName;

  // Read all the records for this type table.
  while (1) {
    BitstreamEntry Entry = Stream.advanceSkippingSubblocks();

    switch (Entry.Kind) {
    case BitstreamEntry::SubBlock: // Handled for us already.
    case BitstreamEntry::Error:
      return Error("Malformed block");
    case BitstreamEntry::EndBlock:
      if (NumRecords != TypeList.size())
        return Error("Malformed block");
      return std::error_code();
    case BitstreamEntry::Record:
      // The interesting case.
      break;
    }

    // Read a record.
    Record.clear();
    Type *ResultTy = nullptr;
    switch (Stream.readRecord(Entry.ID, Record)) {
    default:
      return Error("Invalid value");
    case bitc::TYPE_CODE_NUMENTRY: // TYPE_CODE_NUMENTRY: [numentries]
      // TYPE_CODE_NUMENTRY contains a count of the number of types in the
      // type list.  This allows us to reserve space.
      if (Record.size() < 1)
        return Error("Invalid record");
      TypeList.resize(Record[0]);
      continue;
    case bitc::TYPE_CODE_VOID:      // VOID
      ResultTy = Type::getVoidTy(Context);
      break;
    case bitc::TYPE_CODE_HALF:     // HALF
      ResultTy = Type::getHalfTy(Context);
      break;
    case bitc::TYPE_CODE_FLOAT:     // FLOAT
      ResultTy = Type::getFloatTy(Context);
      break;
    case bitc::TYPE_CODE_DOUBLE:    // DOUBLE
      ResultTy = Type::getDoubleTy(Context);
      break;
    case bitc::TYPE_CODE_X86_FP80:  // X86_FP80
      ResultTy = Type::getX86_FP80Ty(Context);
      break;
    case bitc::TYPE_CODE_FP128:     // FP128
      ResultTy = Type::getFP128Ty(Context);
      break;
    case bitc::TYPE_CODE_PPC_FP128: // PPC_FP128
      ResultTy = Type::getPPC_FP128Ty(Context);
      break;
    case bitc::TYPE_CODE_LABEL:     // LABEL
      ResultTy = Type::getLabelTy(Context);
      break;
    case bitc::TYPE_CODE_METADATA:  // METADATA
      ResultTy = Type::getMetadataTy(Context);
      break;
    case bitc::TYPE_CODE_X86_MMX:   // X86_MMX
      ResultTy = Type::getX86_MMXTy(Context);
      break;
    case bitc::TYPE_CODE_INTEGER:   // INTEGER: [width]
      if (Record.size() < 1)
        return Error("Invalid record");

      ResultTy = IntegerType::get(Context, Record[0]);
      break;
    case bitc::TYPE_CODE_POINTER: { // POINTER: [pointee type] or
                                    //          [pointee type, address space]
      if (Record.size() < 1)
        return Error("Invalid record");
      unsigned AddressSpace = 0;
      if (Record.size() == 2)
        AddressSpace = Record[1];
      ResultTy = getTypeByID(Record[0]);
      if (!ResultTy)
        return Error("Invalid type");
      ResultTy = PointerType::get(ResultTy, AddressSpace);
      break;
    }
    case bitc::TYPE_CODE_FUNCTION_OLD: {
      // FIXME: attrid is dead, remove it in LLVM 4.0
      // FUNCTION: [vararg, attrid, retty, paramty x N]
      if (Record.size() < 3)
        return Error("Invalid record");
      SmallVector<Type*, 8> ArgTys;
      for (unsigned i = 3, e = Record.size(); i != e; ++i) {
        if (Type *T = getTypeByID(Record[i]))
          ArgTys.push_back(T);
        else
          break;
      }

      ResultTy = getTypeByID(Record[2]);
      if (!ResultTy || ArgTys.size() < Record.size()-3)
        return Error("Invalid type");

      ResultTy = FunctionType::get(ResultTy, ArgTys, Record[0]);
      break;
    }
    case bitc::TYPE_CODE_FUNCTION: {
      // FUNCTION: [vararg, retty, paramty x N]
      if (Record.size() < 2)
        return Error("Invalid record");
      SmallVector<Type*, 8> ArgTys;
      for (unsigned i = 2, e = Record.size(); i != e; ++i) {
        if (Type *T = getTypeByID(Record[i]))
          ArgTys.push_back(T);
        else
          break;
      }

      ResultTy = getTypeByID(Record[1]);
      if (!ResultTy || ArgTys.size() < Record.size()-2)
        return Error("Invalid type");

      ResultTy = FunctionType::get(ResultTy, ArgTys, Record[0]);
      break;
    }
    case bitc::TYPE_CODE_STRUCT_ANON: {  // STRUCT: [ispacked, eltty x N]
      if (Record.size() < 1)
        return Error("Invalid record");
      SmallVector<Type*, 8> EltTys;
      for (unsigned i = 1, e = Record.size(); i != e; ++i) {
        if (Type *T = getTypeByID(Record[i]))
          EltTys.push_back(T);
        else
          break;
      }
      if (EltTys.size() != Record.size()-1)
        return Error("Invalid type");
      ResultTy = StructType::get(Context, EltTys, Record[0]);
      break;
    }
    case bitc::TYPE_CODE_STRUCT_NAME:   // STRUCT_NAME: [strchr x N]
      if (ConvertToString(Record, 0, TypeName))
        return Error("Invalid record");
      continue;

    case bitc::TYPE_CODE_STRUCT_NAMED: { // STRUCT: [ispacked, eltty x N]
      if (Record.size() < 1)
        return Error("Invalid record");

      if (NumRecords >= TypeList.size())
        return Error("Invalid TYPE table");

      // Check to see if this was forward referenced, if so fill in the temp.
      StructType *Res = cast_or_null<StructType>(TypeList[NumRecords]);
      if (Res) {
        Res->setName(TypeName);
        TypeList[NumRecords] = nullptr;
      } else  // Otherwise, create a new struct.
        Res = createIdentifiedStructType(Context, TypeName);
      TypeName.clear();

      SmallVector<Type*, 8> EltTys;
      for (unsigned i = 1, e = Record.size(); i != e; ++i) {
        if (Type *T = getTypeByID(Record[i]))
          EltTys.push_back(T);
        else
          break;
      }
      if (EltTys.size() != Record.size()-1)
        return Error("Invalid record");
      Res->setBody(EltTys, Record[0]);
      ResultTy = Res;
      break;
    }
    case bitc::TYPE_CODE_OPAQUE: {       // OPAQUE: []
      if (Record.size() != 1)
        return Error("Invalid record");

      if (NumRecords >= TypeList.size())
        return Error("Invalid TYPE table");

      // Check to see if this was forward referenced, if so fill in the temp.
      StructType *Res = cast_or_null<StructType>(TypeList[NumRecords]);
      if (Res) {
        Res->setName(TypeName);
        TypeList[NumRecords] = nullptr;
      } else  // Otherwise, create a new struct with no body.
        Res = createIdentifiedStructType(Context, TypeName);
      TypeName.clear();
      ResultTy = Res;
      break;
    }
    case bitc::TYPE_CODE_ARRAY:     // ARRAY: [numelts, eltty]
      if (Record.size() < 2)
        return Error("Invalid record");
      if ((ResultTy = getTypeByID(Record[1])))
        ResultTy = ArrayType::get(ResultTy, Record[0]);
      else
        return Error("Invalid type");
      break;
    case bitc::TYPE_CODE_VECTOR:    // VECTOR: [numelts, eltty]
      if (Record.size() < 2)
        return Error("Invalid record");
      if ((ResultTy = getTypeByID(Record[1])))
        ResultTy = VectorType::get(ResultTy, Record[0]);
      else
        return Error("Invalid type");
      break;
    }

    if (NumRecords >= TypeList.size())
      return Error("Invalid TYPE table");
    assert(ResultTy && "Didn't read a type?");
    assert(!TypeList[NumRecords] && "Already read type?");
    TypeList[NumRecords++] = ResultTy;
  }
}

// FIXME: Remove in LLVM 3.1
std::error_code BitcodeReader::ParseOldTypeTable() {
  if (Stream.EnterSubBlock(TYPE_BLOCK_ID_OLD_3_0))
    return Error("Malformed block");

  if (!TypeList.empty())
    return Error("Invalid TYPE table");


  // While horrible, we have no good ordering of types in the bc file.  Just
  // iteratively parse types out of the bc file in multiple passes until we get
  // them all.  Do this by saving a cursor for the start of the type block.
  BitstreamCursor StartOfTypeBlockCursor(Stream);

  unsigned NumTypesRead = 0;

  SmallVector<uint64_t, 64> Record;
RestartScan:
  unsigned NextTypeID = 0;
  bool ReadAnyTypes = false;

  // Read all the records for this type table.
  while (1) {
    unsigned Code = Stream.ReadCode();
    if (Code == bitc::END_BLOCK) {
      if (NextTypeID != TypeList.size())
        return Error("Invalid TYPE table");

      // If we haven't read all of the types yet, iterate again.
      if (NumTypesRead != TypeList.size()) {
        // If we didn't successfully read any types in this pass, then we must
        // have an unhandled forward reference.
        if (!ReadAnyTypes)
          return Error("Invalid TYPE table");

        Stream = StartOfTypeBlockCursor;
        goto RestartScan;
      }

      if (Stream.ReadBlockEnd())
        return Error("Invalid TYPE table");
      return std::error_code();
    }

    if (Code == bitc::ENTER_SUBBLOCK) {
      // No known subblocks, always skip them.
      Stream.ReadSubBlockID();
      if (Stream.SkipBlock())
        return Error("Malformed block");
      continue;
    }

    if (Code == bitc::DEFINE_ABBREV) {
      Stream.ReadAbbrevRecord();
      continue;
    }

    // Read a record.
    Record.clear();
    Type *ResultTy = nullptr;
    switch (Stream.readRecord(Code, Record)) {
    default: return Error("Invalid TYPE table");
    case bitc::TYPE_CODE_NUMENTRY: // TYPE_CODE_NUMENTRY: [numentries]
      // TYPE_CODE_NUMENTRY contains a count of the number of types in the
      // type list.  This allows us to reserve space.
      if (Record.size() < 1)
        return Error("Invalid TYPE table");
      TypeList.resize(Record[0]);
      continue;
    case bitc::TYPE_CODE_VOID:      // VOID
      ResultTy = Type::getVoidTy(Context);
      break;
    case bitc::TYPE_CODE_FLOAT:     // FLOAT
      ResultTy = Type::getFloatTy(Context);
      break;
    case bitc::TYPE_CODE_DOUBLE:    // DOUBLE
      ResultTy = Type::getDoubleTy(Context);
      break;
    case bitc::TYPE_CODE_X86_FP80:  // X86_FP80
      ResultTy = Type::getX86_FP80Ty(Context);
      break;
    case bitc::TYPE_CODE_FP128:     // FP128
      ResultTy = Type::getFP128Ty(Context);
      break;
    case bitc::TYPE_CODE_PPC_FP128: // PPC_FP128
      ResultTy = Type::getPPC_FP128Ty(Context);
      break;
    case bitc::TYPE_CODE_LABEL:     // LABEL
      ResultTy = Type::getLabelTy(Context);
      break;
    case bitc::TYPE_CODE_METADATA:  // METADATA
      ResultTy = Type::getMetadataTy(Context);
      break;
    case bitc::TYPE_CODE_X86_MMX:   // X86_MMX
      ResultTy = Type::getX86_MMXTy(Context);
      break;
    case bitc::TYPE_CODE_INTEGER:   // INTEGER: [width]
      if (Record.size() < 1)
        return Error("Invalid TYPE table");
      ResultTy = IntegerType::get(Context, Record[0]);
      break;
    case bitc::TYPE_CODE_OPAQUE:    // OPAQUE
      if (NextTypeID < TypeList.size() && TypeList[NextTypeID] == 0)
        ResultTy = StructType::create(Context, "");
      break;
    case TYPE_CODE_STRUCT_OLD_3_0: {// STRUCT_OLD
      if (NextTypeID >= TypeList.size()) break;
      // If we already read it, don't reprocess.
      if (TypeList[NextTypeID] &&
          !cast<StructType>(TypeList[NextTypeID])->isOpaque())
        break;

      // Set a type.
      if (TypeList[NextTypeID] == 0)
        TypeList[NextTypeID] = StructType::create(Context, "");

      std::vector<Type*> EltTys;
      for (unsigned i = 1, e = Record.size(); i != e; ++i) {
        if (Type *Elt = getTypeByIDOrNull(Record[i]))
          EltTys.push_back(Elt);
        else
          break;
      }

      if (EltTys.size() != Record.size()-1)
        break;      // Not all elements are ready.

      cast<StructType>(TypeList[NextTypeID])->setBody(EltTys, Record[0]);
      ResultTy = TypeList[NextTypeID];
      TypeList[NextTypeID] = 0;
      break;
    }
    case bitc::TYPE_CODE_POINTER: { // POINTER: [pointee type] or
      //          [pointee type, address space]
      if (Record.size() < 1)
        return Error("Invalid TYPE table");
      unsigned AddressSpace = 0;
      if (Record.size() == 2)
        AddressSpace = Record[1];
      if ((ResultTy = getTypeByIDOrNull(Record[0])))
        ResultTy = PointerType::get(ResultTy, AddressSpace);
      break;
    }
    case bitc::TYPE_CODE_FUNCTION_OLD: {
      // FIXME: attrid is dead, remove it in LLVM 3.0
      // FUNCTION: [vararg, attrid, retty, paramty x N]
      if (Record.size() < 3)
        return Error("Invalid TYPE table");
      std::vector<Type*> ArgTys;
      for (unsigned i = 3, e = Record.size(); i != e; ++i) {
        if (Type *Elt = getTypeByIDOrNull(Record[i]))
          ArgTys.push_back(Elt);
        else
          break;
      }
      if (ArgTys.size()+3 != Record.size())
        break;  // Something was null.
      if ((ResultTy = getTypeByIDOrNull(Record[2])))
        ResultTy = FunctionType::get(ResultTy, ArgTys, Record[0]);
      break;
    }
    case bitc::TYPE_CODE_FUNCTION: {
      // FUNCTION: [vararg, retty, paramty x N]
      if (Record.size() < 2)
        return Error("Invalid TYPE table");
      std::vector<Type*> ArgTys;
      for (unsigned i = 2, e = Record.size(); i != e; ++i) {
        if (Type *Elt = getTypeByIDOrNull(Record[i]))
          ArgTys.push_back(Elt);
        else
          break;
      }
      if (ArgTys.size()+2 != Record.size())
        break;  // Something was null.
      if ((ResultTy = getTypeByIDOrNull(Record[1])))
        ResultTy = FunctionType::get(ResultTy, ArgTys, Record[0]);
      break;
    }
    case bitc::TYPE_CODE_ARRAY:     // ARRAY: [numelts, eltty]
      if (Record.size() < 2)
        return Error("Invalid TYPE table");
      if ((ResultTy = getTypeByIDOrNull(Record[1])))
        ResultTy = ArrayType::get(ResultTy, Record[0]);
      break;
    case bitc::TYPE_CODE_VECTOR:    // VECTOR: [numelts, eltty]
      if (Record.size() < 2)
        return Error("Invalid TYPE table");
      if ((ResultTy = getTypeByIDOrNull(Record[1])))
        ResultTy = VectorType::get(ResultTy, Record[0]);
      break;
    }

    if (NextTypeID >= TypeList.size())
      return Error("Invalid TYPE table");

    if (ResultTy && TypeList[NextTypeID] == 0) {
      ++NumTypesRead;
      ReadAnyTypes = true;

      TypeList[NextTypeID] = ResultTy;
    }

    ++NextTypeID;
  }
}


std::error_code BitcodeReader::ParseOldTypeSymbolTable() {
  if (Stream.EnterSubBlock(TYPE_SYMTAB_BLOCK_ID_OLD_3_0))
    return Error("Malformed block");

  SmallVector<uint64_t, 64> Record;

  // Read all the records for this type table.
  std::string TypeName;
  while (1) {
    unsigned Code = Stream.ReadCode();
    if (Code == bitc::END_BLOCK) {
      if (Stream.ReadBlockEnd())
        return Error("Malformed block");
      return std::error_code();
    }

    if (Code == bitc::ENTER_SUBBLOCK) {
      // No known subblocks, always skip them.
      Stream.ReadSubBlockID();
      if (Stream.SkipBlock())
        return Error("Malformed block");
      continue;
    }

    if (Code == bitc::DEFINE_ABBREV) {
      Stream.ReadAbbrevRecord();
      continue;
    }

    // Read a record.
    Record.clear();
    switch (Stream.readRecord(Code, Record)) {
    default:  // Default behavior: unknown type.
      break;
    case bitc::TST_CODE_ENTRY:    // TST_ENTRY: [typeid, namechar x N]
      if (ConvertToString(Record, 1, TypeName))
        return Error("Invalid record");
      unsigned TypeID = Record[0];
      if (TypeID >= TypeList.size())
        return Error("Invalid record");

      // Only apply the type name to a struct type with no name.
      if (StructType *STy = dyn_cast<StructType>(TypeList[TypeID]))
        if (!STy->isLiteral() && !STy->hasName())
          STy->setName(TypeName);
      TypeName.clear();
      break;
    }
  }
}

std::error_code BitcodeReader::ParseValueSymbolTable() {
  if (Stream.EnterSubBlock(bitc::VALUE_SYMTAB_BLOCK_ID))
    return Error("Invalid record");

  SmallVector<uint64_t, 64> Record;

  // Read all the records for this value table.
  SmallString<128> ValueName;
  while (1) {
    unsigned Code = Stream.ReadCode();
    if (Code == bitc::END_BLOCK) {
      if (Stream.ReadBlockEnd())
        return Error("Malformed block");
      return std::error_code();
    }
    if (Code == bitc::ENTER_SUBBLOCK) {
      // No known subblocks, always skip them.
      Stream.ReadSubBlockID();
      if (Stream.SkipBlock())
        return Error("Malformed block");
      continue;
    }

    if (Code == bitc::DEFINE_ABBREV) {
      Stream.ReadAbbrevRecord();
      continue;
    }

    // Read a record.
    Record.clear();
    switch (Stream.readRecord(Code, Record)) {
    default:  // Default behavior: unknown type.
      break;
    case bitc::VST_CODE_ENTRY: {  // VST_ENTRY: [valueid, namechar x N]
      if (ConvertToString(Record, 1, ValueName))
        return Error("Invalid record");
      unsigned ValueID = Record[0];
      if (ValueID >= ValueList.size())
        return Error("Invalid record");
      Value *V = ValueList[ValueID];

      V->setName(StringRef(ValueName.data(), ValueName.size()));
      ValueName.clear();
      break;
    }
    case bitc::VST_CODE_BBENTRY: {
      if (ConvertToString(Record, 1, ValueName))
        return Error("Invalid record");
      BasicBlock *BB = getBasicBlock(Record[0]);
      if (!BB)
        return Error("Invalid record");

      BB->setName(StringRef(ValueName.data(), ValueName.size()));
      ValueName.clear();
      break;
    }
    }
  }
}

std::error_code BitcodeReader::ParseMetadata() {
  unsigned NextMDValueNo = MDValueList.size();

  if (Stream.EnterSubBlock(bitc::METADATA_BLOCK_ID))
    return Error("Invalid record");

  SmallVector<uint64_t, 64> Record;

  // Read all the records.
  while (1) {
    unsigned Code = Stream.ReadCode();
    if (Code == bitc::END_BLOCK) {
      if (Stream.ReadBlockEnd())
        return Error("Malformed block");
      return std::error_code();
    }

    if (Code == bitc::ENTER_SUBBLOCK) {
      // No known subblocks, always skip them.
      Stream.ReadSubBlockID();
      if (Stream.SkipBlock())
        return Error("Malformed block");
      continue;
    }

    if (Code == bitc::DEFINE_ABBREV) {
      Stream.ReadAbbrevRecord();
      continue;
    }

    bool IsFunctionLocal = false;
    // Read a record.
    Record.clear();
    Code = Stream.readRecord(Code, Record);
    switch (Code) {
    default:  // Default behavior: ignore.
      break;
    case bitc::METADATA_NAME: {
      // Read name of the named metadata.
      SmallString<8> Name(Record.begin(), Record.end());
      Record.clear();
      Code = Stream.ReadCode();

      // METADATA_NAME is always followed by METADATA_NAMED_NODE.
      unsigned NextBitCode = Stream.readRecord(Code, Record);
      assert(NextBitCode == bitc::METADATA_NAMED_NODE); (void)NextBitCode;

      // Read named metadata elements.
      unsigned Size = Record.size();
      NamedMDNode *NMD = TheModule->getOrInsertNamedMetadata(Name);
      for (unsigned i = 0; i != Size; ++i) {
        MDNode *MD = dyn_cast_or_null<MDNode>(MDValueList.getValueFwdRef(Record[i]));
        if (!MD)
          return Error("Invalid record");
        NMD->addOperand(MD);
      }
      break;
    }
    case bitc::METADATA_OLD_FN_NODE:
      IsFunctionLocal = true;
      // fall-through
    case bitc::METADATA_OLD_NODE: {
      if (Record.size() % 2 == 1)
        return Error("Invalid record");

      unsigned Size = Record.size();
      SmallVector<Metadata *, 8> Elts;
      for (unsigned i = 0; i != Size; i += 2) {
        Type *Ty = getTypeByID(Record[i]);
        if (!Ty)
          return Error("Invalid record");
        if (Ty->isMetadataTy())
          Elts.push_back(MDValueList.getValueFwdRef(Record[i+1]));
        else if (!Ty->isVoidTy()) {
          auto *MD =
              ValueAsMetadata::get(ValueList.getValueFwdRef(Record[i + 1], Ty));
          assert(isa<ConstantAsMetadata>(MD) &&
                 "Expected non-function-local metadata");
          Elts.push_back(MD);
        } else
          Elts.push_back(nullptr);
      }
      MDValueList.AssignValue(MDNode::get(Context, Elts), NextMDValueNo++);
      break;
    }
    case bitc::METADATA_STRING_OLD: {
      std::string String(Record.begin(), Record.end());

      // Test for upgrading !llvm.loop.
      mayBeOldLoopAttachmentTag(String);

      Metadata *MD = MDString::get(Context, String);
      MDValueList.AssignValue(MD, NextMDValueNo++);
      break;
    }
    case bitc::METADATA_KIND: {
      if (Record.size() < 2)
        return Error("Invalid record");

      unsigned Kind = Record[0];
      SmallString<8> Name(Record.begin()+1, Record.end());

      unsigned NewKind = TheModule->getMDKindID(Name.str());
      if (!MDKindMap.insert(std::make_pair(Kind, NewKind)).second)
        return Error("Conflicting METADATA_KIND records");
      break;
    }
    }
  }
}

/// decodeSignRotatedValue - Decode a signed value stored with the sign bit in
/// the LSB for dense VBR encoding.
uint64_t BitcodeReader::decodeSignRotatedValue(uint64_t V) {
  if ((V & 1) == 0)
    return V >> 1;
  if (V != 1)
    return -(V >> 1);
  // There is no such thing as -0 with integers.  "-0" really means MININT.
  return 1ULL << 63;
}

// FIXME: Delete this in LLVM 4.0 and just assert that the aliasee is a
// GlobalObject.
static GlobalObject &
getGlobalObjectInExpr(const DenseMap<GlobalAlias *, Constant *> &Map,
                      Constant &C) {
  auto *GO = dyn_cast<GlobalObject>(&C);
  if (GO)
    return *GO;

  auto *GA = dyn_cast<GlobalAlias>(&C);
  if (GA)
    return getGlobalObjectInExpr(Map, *Map.find(GA)->second);

  auto &CE = cast<ConstantExpr>(C);
  assert(CE.getOpcode() == Instruction::BitCast ||
         CE.getOpcode() == Instruction::GetElementPtr ||
         CE.getOpcode() == Instruction::AddrSpaceCast);
  if (CE.getOpcode() == Instruction::GetElementPtr)
    assert(cast<GEPOperator>(CE).hasAllZeroIndices());
  return getGlobalObjectInExpr(Map, *CE.getOperand(0));
}

/// ResolveGlobalAndAliasInits - Resolve all of the initializers for global
/// values and aliases that we can.
std::error_code BitcodeReader::ResolveGlobalAndAliasInits() {
  std::vector<std::pair<GlobalVariable*, unsigned> > GlobalInitWorklist;
  std::vector<std::pair<GlobalAlias*, unsigned> > AliasInitWorklist;

  GlobalInitWorklist.swap(GlobalInits);
  AliasInitWorklist.swap(AliasInits);

  while (!GlobalInitWorklist.empty()) {
    unsigned ValID = GlobalInitWorklist.back().second;
    if (ValID >= ValueList.size()) {
      // Not ready to resolve this yet, it requires something later in the file.
      GlobalInits.push_back(GlobalInitWorklist.back());
    } else {
      if (Constant *C = dyn_cast_or_null<Constant>(ValueList[ValID]))
        GlobalInitWorklist.back().first->setInitializer(C);
      else
        return Error("Expected a constant");
    }
    GlobalInitWorklist.pop_back();
  }

  // FIXME: Delete this in LLVM 4.0
  // Older versions of llvm could write an alias pointing to another. We cannot
  // construct those aliases, so we first collect an alias to aliasee expression
  // and then compute the actual aliasee.
  DenseMap<GlobalAlias *, Constant *> AliasInit;

  while (!AliasInitWorklist.empty()) {
    unsigned ValID = AliasInitWorklist.back().second;
    if (ValID >= ValueList.size()) {
      AliasInits.push_back(AliasInitWorklist.back());
    } else {
      if (Constant *C = dyn_cast_or_null<Constant>(ValueList[ValID]))
        AliasInit.insert(std::make_pair(AliasInitWorklist.back().first, C));
      else
        return Error("Expected a constant");
    }
    AliasInitWorklist.pop_back();
  }

  for (auto &Pair : AliasInit) {
    auto &GO = getGlobalObjectInExpr(AliasInit, *Pair.second);
    Pair.first->setAliasee(&GO);
  }

  return std::error_code();
}

static APInt ReadWideAPInt(ArrayRef<uint64_t> Vals, unsigned TypeBits) {
  SmallVector<uint64_t, 8> Words(Vals.size());
  std::transform(Vals.begin(), Vals.end(), Words.begin(),
                 BitcodeReader::decodeSignRotatedValue);

  return APInt(TypeBits, Words);
}

std::error_code BitcodeReader::ParseConstants() {
  if (Stream.EnterSubBlock(bitc::CONSTANTS_BLOCK_ID))
    return Error("Invalid record");

  SmallVector<uint64_t, 64> Record;

  // Read all the records for this value table.
  Type *CurTy = Type::getInt32Ty(Context);
  unsigned NextCstNo = ValueList.size();
  while (1) {
    BitstreamEntry Entry = Stream.advanceSkippingSubblocks();

    switch (Entry.Kind) {
    case BitstreamEntry::SubBlock: // Handled for us already.
    case BitstreamEntry::Error:
      return Error("Malformed block");
    case BitstreamEntry::EndBlock:
      if (NextCstNo != ValueList.size())
        return Error("Invalid constant reference");

      // Once all the constants have been read, go through and resolve forward
      // references.
      ValueList.ResolveConstantForwardRefs();
      return std::error_code();
    case BitstreamEntry::Record:
      // The interesting case.
      break;
    }

    // Read a record.
    Record.clear();
    Value *V = nullptr;
    unsigned BitCode = Stream.readRecord(Entry.ID, Record);
    switch (BitCode) {
    default:  // Default behavior: unknown constant
    case bitc::CST_CODE_UNDEF:     // UNDEF
      V = UndefValue::get(CurTy);
      break;
    case bitc::CST_CODE_SETTYPE:   // SETTYPE: [typeid]
      if (Record.empty())
        return Error("Invalid record");
      if (Record[0] >= TypeList.size())
        return Error("Invalid record");
      CurTy = TypeList[Record[0]];
      continue;  // Skip the ValueList manipulation.
    case bitc::CST_CODE_NULL:      // NULL
      V = Constant::getNullValue(CurTy);
      break;
    case bitc::CST_CODE_INTEGER:   // INTEGER: [intval]
      if (!CurTy->isIntegerTy() || Record.empty())
        return Error("Invalid record");
      V = ConstantInt::get(CurTy, decodeSignRotatedValue(Record[0]));
      break;
    case bitc::CST_CODE_WIDE_INTEGER: {// WIDE_INTEGER: [n x intval]
      if (!CurTy->isIntegerTy() || Record.empty())
        return Error("Invalid record");

      APInt VInt = ReadWideAPInt(Record,
                                 cast<IntegerType>(CurTy)->getBitWidth());
      V = ConstantInt::get(Context, VInt);

      break;
    }
    case bitc::CST_CODE_FLOAT: {    // FLOAT: [fpval]
      if (Record.empty())
        return Error("Invalid record");
      if (CurTy->isHalfTy())
        V = ConstantFP::get(Context, APFloat(APFloat::IEEEhalf,
                                             APInt(16, (uint16_t)Record[0])));
      else if (CurTy->isFloatTy())
        V = ConstantFP::get(Context, APFloat(APFloat::IEEEsingle,
                                             APInt(32, (uint32_t)Record[0])));
      else if (CurTy->isDoubleTy())
        V = ConstantFP::get(Context, APFloat(APFloat::IEEEdouble,
                                             APInt(64, Record[0])));
      else if (CurTy->isX86_FP80Ty()) {
        // Bits are not stored the same way as a normal i80 APInt, compensate.
        uint64_t Rearrange[2];
        Rearrange[0] = (Record[1] & 0xffffLL) | (Record[0] << 16);
        Rearrange[1] = Record[0] >> 48;
        V = ConstantFP::get(Context, APFloat(APFloat::x87DoubleExtended,
                                             APInt(80, Rearrange)));
      } else if (CurTy->isFP128Ty())
        V = ConstantFP::get(Context, APFloat(APFloat::IEEEquad,
                                             APInt(128, Record)));
      else if (CurTy->isPPC_FP128Ty())
        V = ConstantFP::get(Context, APFloat(APFloat::PPCDoubleDouble,
                                             APInt(128, Record)));
      else
        V = UndefValue::get(CurTy);
      break;
    }

    case bitc::CST_CODE_AGGREGATE: {// AGGREGATE: [n x value number]
      if (Record.empty())
        return Error("Invalid record");

      unsigned Size = Record.size();
      SmallVector<Constant*, 16> Elts;

      if (StructType *STy = dyn_cast<StructType>(CurTy)) {
        for (unsigned i = 0; i != Size; ++i)
          Elts.push_back(ValueList.getConstantFwdRef(Record[i],
                                                     STy->getElementType(i)));
        V = ConstantStruct::get(STy, Elts);
      } else if (ArrayType *ATy = dyn_cast<ArrayType>(CurTy)) {
        Type *EltTy = ATy->getElementType();
        for (unsigned i = 0; i != Size; ++i)
          Elts.push_back(ValueList.getConstantFwdRef(Record[i], EltTy));
        V = ConstantArray::get(ATy, Elts);
      } else if (VectorType *VTy = dyn_cast<VectorType>(CurTy)) {
        Type *EltTy = VTy->getElementType();
        for (unsigned i = 0; i != Size; ++i)
          Elts.push_back(ValueList.getConstantFwdRef(Record[i], EltTy));
        V = ConstantVector::get(Elts);
      } else {
        V = UndefValue::get(CurTy);
      }
      break;
    }
    case bitc::CST_CODE_STRING: { // STRING: [values]
      if (Record.empty())
        return Error("Invalid record");

      ArrayType *ATy = cast<ArrayType>(CurTy);
      Type *EltTy = ATy->getElementType();

      unsigned Size = Record.size();
      std::vector<Constant*> Elts;
      for (unsigned i = 0; i != Size; ++i)
        Elts.push_back(ConstantInt::get(EltTy, Record[i]));
      V = ConstantArray::get(ATy, Elts);
      break;
    }
    case bitc::CST_CODE_CSTRING: { // CSTRING: [values]
      if (Record.empty())
        return Error("Invalid record");

      ArrayType *ATy = cast<ArrayType>(CurTy);
      Type *EltTy = ATy->getElementType();

      unsigned Size = Record.size();
      std::vector<Constant*> Elts;
      for (unsigned i = 0; i != Size; ++i)
        Elts.push_back(ConstantInt::get(EltTy, Record[i]));
      Elts.push_back(Constant::getNullValue(EltTy));
      V = ConstantArray::get(ATy, Elts);
      break;
    }
    case bitc::CST_CODE_CE_BINOP: {  // CE_BINOP: [opcode, opval, opval]
      if (Record.size() < 3)
        return Error("Invalid record");
      int Opc = GetDecodedBinaryOpcode(Record[0], CurTy);
      if (Opc < 0) {
        V = UndefValue::get(CurTy);  // Unknown binop.
      } else {
        Constant *LHS = ValueList.getConstantFwdRef(Record[1], CurTy);
        Constant *RHS = ValueList.getConstantFwdRef(Record[2], CurTy);
        unsigned Flags = 0;
        if (Record.size() >= 4) {
          if (Opc == Instruction::Add ||
              Opc == Instruction::Sub ||
              Opc == Instruction::Mul ||
              Opc == Instruction::Shl) {
            if (Record[3] & (1 << bitc::OBO_NO_SIGNED_WRAP))
              Flags |= OverflowingBinaryOperator::NoSignedWrap;
            if (Record[3] & (1 << bitc::OBO_NO_UNSIGNED_WRAP))
              Flags |= OverflowingBinaryOperator::NoUnsignedWrap;
          } else if (Opc == Instruction::SDiv ||
                     Opc == Instruction::UDiv ||
                     Opc == Instruction::LShr ||
                     Opc == Instruction::AShr) {
            if (Record[3] & (1 << bitc::PEO_EXACT))
              Flags |= SDivOperator::IsExact;
          }
        }
        V = ConstantExpr::get(Opc, LHS, RHS, Flags);
      }
      break;
    }
    case bitc::CST_CODE_CE_CAST: {  // CE_CAST: [opcode, opty, opval]
      if (Record.size() < 3)
        return Error("Invalid record");
      int Opc = GetDecodedCastOpcode(Record[0]);
      if (Opc < 0) {
        V = UndefValue::get(CurTy);  // Unknown cast.
      } else {
        Type *OpTy = getTypeByID(Record[1]);
        if (!OpTy)
          return Error("Invalid record");
        Constant *Op = ValueList.getConstantFwdRef(Record[2], OpTy);
        V = ConstantExpr::getCast(Opc, Op, CurTy);
      }
      break;
    }
    case bitc::CST_CODE_CE_INBOUNDS_GEP:
    case bitc::CST_CODE_CE_GEP: {  // CE_GEP:        [n x operands]
      Type *PointeeType = nullptr;
      if (Record.size() & 1)
        return Error("Invalid record");
      SmallVector<Constant*, 16> Elts;
      for (unsigned i = 0, e = Record.size(); i != e; i += 2) {
        Type *ElTy = getTypeByID(Record[i]);
        if (!ElTy)
          return Error("Invalid record");
        Elts.push_back(ValueList.getConstantFwdRef(Record[i+1], ElTy));
      }
      ArrayRef<Constant *> Indices(Elts.begin() + 1, Elts.end());
      V = ConstantExpr::getGetElementPtr(PointeeType, Elts[0], Indices,
                                         BitCode ==
                                           bitc::CST_CODE_CE_INBOUNDS_GEP);
      break;
    }
    case bitc::CST_CODE_CE_SELECT:  // CE_SELECT: [opval#, opval#, opval#]
      if (Record.size() < 3)
        return Error("Invalid record");
      V = ConstantExpr::getSelect(ValueList.getConstantFwdRef(Record[0],
                                                              Type::getInt1Ty(Context)),
                                  ValueList.getConstantFwdRef(Record[1],CurTy),
                                  ValueList.getConstantFwdRef(Record[2],CurTy));
      break;
    case bitc::CST_CODE_CE_EXTRACTELT: { // CE_EXTRACTELT: [opty, opval, opval]
      if (Record.size() < 3)
        return Error("Invalid record");
      VectorType *OpTy =
        dyn_cast_or_null<VectorType>(getTypeByID(Record[0]));
      if (!OpTy)
        return Error("Invalid record");
      Constant *Op0 = ValueList.getConstantFwdRef(Record[1], OpTy);
      Constant *Op1 = ValueList.getConstantFwdRef(Record[2], Type::getInt32Ty(Context));
      V = ConstantExpr::getExtractElement(Op0, Op1);
      break;
    }
    case bitc::CST_CODE_CE_INSERTELT: { // CE_INSERTELT: [opval, opval, opval]
      VectorType *OpTy = dyn_cast<VectorType>(CurTy);
      if (Record.size() < 3 || !OpTy)
        return Error("Invalid record");
      Constant *Op0 = ValueList.getConstantFwdRef(Record[0], OpTy);
      Constant *Op1 = ValueList.getConstantFwdRef(Record[1],
                                                  OpTy->getElementType());
      Constant *Op2 = ValueList.getConstantFwdRef(Record[2], Type::getInt32Ty(Context));
      V = ConstantExpr::getInsertElement(Op0, Op1, Op2);
      break;
    }
    case bitc::CST_CODE_CE_SHUFFLEVEC: { // CE_SHUFFLEVEC: [opval, opval, opval]
      VectorType *OpTy = dyn_cast<VectorType>(CurTy);
      if (Record.size() < 3 || !OpTy)
        return Error("Invalid record");
      Constant *Op0 = ValueList.getConstantFwdRef(Record[0], OpTy);
      Constant *Op1 = ValueList.getConstantFwdRef(Record[1], OpTy);
      Type *ShufTy = VectorType::get(Type::getInt32Ty(Context),
                                                 OpTy->getNumElements());
      Constant *Op2 = ValueList.getConstantFwdRef(Record[2], ShufTy);
      V = ConstantExpr::getShuffleVector(Op0, Op1, Op2);
      break;
    }
    case bitc::CST_CODE_CE_SHUFVEC_EX: { // [opty, opval, opval, opval]
      VectorType *RTy = dyn_cast<VectorType>(CurTy);
      VectorType *OpTy =
        dyn_cast_or_null<VectorType>(getTypeByID(Record[0]));
      if (Record.size() < 4 || !RTy || !OpTy)
        return Error("Invalid record");
      Constant *Op0 = ValueList.getConstantFwdRef(Record[1], OpTy);
      Constant *Op1 = ValueList.getConstantFwdRef(Record[2], OpTy);
      Type *ShufTy = VectorType::get(Type::getInt32Ty(Context),
                                                 RTy->getNumElements());
      Constant *Op2 = ValueList.getConstantFwdRef(Record[3], ShufTy);
      V = ConstantExpr::getShuffleVector(Op0, Op1, Op2);
      break;
    }
    case bitc::CST_CODE_CE_CMP: {     // CE_CMP: [opty, opval, opval, pred]
      if (Record.size() < 4)
        return Error("Invalid record");
      Type *OpTy = getTypeByID(Record[0]);
      if (!OpTy)
        return Error("Invalid record");
      Constant *Op0 = ValueList.getConstantFwdRef(Record[1], OpTy);
      Constant *Op1 = ValueList.getConstantFwdRef(Record[2], OpTy);

      if (OpTy->isFPOrFPVectorTy())
        V = ConstantExpr::getFCmp(Record[3], Op0, Op1);
      else
        V = ConstantExpr::getICmp(Record[3], Op0, Op1);
      break;
    }
    case bitc::CST_CODE_INLINEASM:
    case bitc::CST_CODE_INLINEASM_OLD: {
      if (Record.size() < 2)
        return Error("Invalid record");
      std::string AsmStr, ConstrStr;
      bool HasSideEffects = Record[0] & 1;
      bool IsAlignStack = Record[0] >> 1;
      unsigned AsmStrSize = Record[1];
      if (2+AsmStrSize >= Record.size())
        return Error("Invalid record");
      unsigned ConstStrSize = Record[2+AsmStrSize];
      if (3+AsmStrSize+ConstStrSize > Record.size())
        return Error("Invalid record");

      for (unsigned i = 0; i != AsmStrSize; ++i)
        AsmStr += (char)Record[2+i];
      for (unsigned i = 0; i != ConstStrSize; ++i)
        ConstrStr += (char)Record[3+AsmStrSize+i];
      PointerType *PTy = cast<PointerType>(CurTy);
      V = InlineAsm::get(cast<FunctionType>(PTy->getElementType()),
                         AsmStr, ConstrStr, HasSideEffects, IsAlignStack);
      break;
    }
    case bitc::CST_CODE_BLOCKADDRESS:{
      if (Record.size() < 3)
        return Error("Invalid record");
      Type *FnTy = getTypeByID(Record[0]);
      if (!FnTy)
        return Error("Invalid record");
      Function *Fn =
        dyn_cast_or_null<Function>(ValueList.getConstantFwdRef(Record[1],FnTy));
      if (!Fn)
        return Error("Invalid record");

      GlobalVariable *FwdRef = new GlobalVariable(*Fn->getParent(),
                                                  Type::getInt8Ty(Context),
                                            false, GlobalValue::InternalLinkage,
                                                  0, "");
      BlockAddrFwdRefs[Fn].push_back(std::make_pair(Record[2], FwdRef));
      V = FwdRef;
      break;
    }
    }

    ValueList.AssignValue(V, NextCstNo);
    ++NextCstNo;
  }

  if (NextCstNo != ValueList.size())
    return Error("Invalid constant reference");

  if (Stream.ReadBlockEnd())
    return Error("Expected a constant");

  // Once all the constants have been read, go through and resolve forward
  // references.
  ValueList.ResolveConstantForwardRefs();
  return std::error_code();
}

std::error_code BitcodeReader::materializeMetadata() {
  return std::error_code();
}

void BitcodeReader::setStripDebugInfo() { }

/// RememberAndSkipFunctionBody - When we see the block for a function body,
/// remember where it is and then skip it.  This lets us lazily deserialize the
/// functions.
std::error_code BitcodeReader::RememberAndSkipFunctionBody() {
  // Get the function we are talking about.
  if (FunctionsWithBodies.empty())
    return Error("Insufficient function protos");

  Function *Fn = FunctionsWithBodies.back();
  FunctionsWithBodies.pop_back();

  // Save the current stream state.
  uint64_t CurBit = Stream.GetCurrentBitNo();
  DeferredFunctionInfo[Fn] = CurBit;

  // Skip over the function block for now.
  if (Stream.SkipBlock())
    return Error("Invalid record");
  return std::error_code();
}

std::error_code BitcodeReader::GlobalCleanup() {
  // Patch the initializers for globals and aliases up.
  ResolveGlobalAndAliasInits();
  if (!GlobalInits.empty() || !AliasInits.empty())
    return Error("Malformed global initializer set");

  // Look for intrinsic functions which need to be upgraded at some point
  for (Module::iterator FI = TheModule->begin(), FE = TheModule->end();
       FI != FE; ++FI) {
    Function *NewFn;
    if (UpgradeIntrinsicFunction(&*FI, NewFn))
      UpgradedIntrinsics.push_back(std::make_pair(&*FI, NewFn));
  }

  // Look for global variables which need to be renamed.
  for (Module::global_iterator
         GI = TheModule->global_begin(), GE = TheModule->global_end();
       GI != GE; GI++) {
    GlobalVariable *GV = &*GI;
    UpgradeGlobalVariable(GV);
  }

  // Force deallocation of memory for these vectors to favor the client that
  // want lazy deserialization.
  std::vector<std::pair<GlobalVariable*, unsigned> >().swap(GlobalInits);
  std::vector<std::pair<GlobalAlias*, unsigned> >().swap(AliasInits);
  return std::error_code();
}

std::error_code BitcodeReader::ParseModule(bool Resume) {
  if (Resume)
    Stream.JumpToBit(NextUnreadBit);
  else if (Stream.EnterSubBlock(bitc::MODULE_BLOCK_ID))
    return Error("Invalid record");

  SmallVector<uint64_t, 64> Record;
  std::vector<std::string> SectionTable;
  std::vector<std::string> GCTable;

  // Read all the records for this module.
  while (1) {
    BitstreamEntry Entry = Stream.advance();

    switch (Entry.Kind) {
    case BitstreamEntry::Error:
      return Error("Malformed block");
    case BitstreamEntry::EndBlock:
      return GlobalCleanup();

    case BitstreamEntry::SubBlock:
      switch (Entry.ID) {
      default:  // Skip unknown content.
        if (Stream.SkipBlock())
          return Error("Invalid record");
        break;
      case bitc::BLOCKINFO_BLOCK_ID:
        if (Stream.ReadBlockInfoBlock())
          return Error("Malformed block");
        break;
      case bitc::PARAMATTR_BLOCK_ID:
        if (std::error_code EC = ParseAttributeBlock())
          return EC;
        break;
      case bitc::TYPE_BLOCK_ID_NEW:
        if (std::error_code EC = ParseTypeTable())
          return EC;
        break;
      case TYPE_BLOCK_ID_OLD_3_0:
        if (std::error_code EC = ParseOldTypeTable())
          return EC;
        break;
      case TYPE_SYMTAB_BLOCK_ID_OLD_3_0:
        if (std::error_code EC = ParseOldTypeSymbolTable())
          return EC;
        break;
      case bitc::VALUE_SYMTAB_BLOCK_ID:
        if (std::error_code EC = ParseValueSymbolTable())
          return EC;
        SeenValueSymbolTable = true;
        break;
      case bitc::CONSTANTS_BLOCK_ID:
        if (std::error_code EC = ParseConstants())
          return EC;
        if (std::error_code EC = ResolveGlobalAndAliasInits())
          return EC;
        break;
      case bitc::METADATA_BLOCK_ID:
        if (std::error_code EC = ParseMetadata())
          return EC;
        break;
      case bitc::FUNCTION_BLOCK_ID:
        // If this is the first function body we've seen, reverse the
        // FunctionsWithBodies list.
        if (!SeenFirstFunctionBody) {
          std::reverse(FunctionsWithBodies.begin(), FunctionsWithBodies.end());
          if (std::error_code EC = GlobalCleanup())
            return EC;
          SeenFirstFunctionBody = true;
        }

        if (std::error_code EC = RememberAndSkipFunctionBody())
          return EC;
        // For streaming bitcode, suspend parsing when we reach the function
        // bodies. Subsequent materialization calls will resume it when
        // necessary. For streaming, the function bodies must be at the end of
        // the bitcode. If the bitcode file is old, the symbol table will be
        // at the end instead and will not have been seen yet. In this case,
        // just finish the parse now.
        if (LazyStreamer && SeenValueSymbolTable) {
          NextUnreadBit = Stream.GetCurrentBitNo();
          return std::error_code();
        }
        break;
        break;
      }
      continue;

    case BitstreamEntry::Record:
      // The interesting case.
      break;
    }


    // Read a record.
    switch (Stream.readRecord(Entry.ID, Record)) {
    default: break;  // Default behavior, ignore unknown content.
    case bitc::MODULE_CODE_VERSION: {  // VERSION: [version#]
      if (Record.size() < 1)
        return Error("Invalid record");
      // Only version #0 is supported so far.
      if (Record[0] != 0)
        return Error("Invalid value");
      break;
    }
    case bitc::MODULE_CODE_TRIPLE: {  // TRIPLE: [strchr x N]
      std::string S;
      if (ConvertToString(Record, 0, S))
        return Error("Invalid record");
      TheModule->setTargetTriple(S);
      break;
    }
    case bitc::MODULE_CODE_DATALAYOUT: {  // DATALAYOUT: [strchr x N]
      std::string S;
      if (ConvertToString(Record, 0, S))
        return Error("Invalid record");
      TheModule->setDataLayout(S);
      break;
    }
    case bitc::MODULE_CODE_ASM: {  // ASM: [strchr x N]
      std::string S;
      if (ConvertToString(Record, 0, S))
        return Error("Invalid record");
      TheModule->setModuleInlineAsm(S);
      break;
    }
    case bitc::MODULE_CODE_DEPLIB: {  // DEPLIB: [strchr x N]
      std::string S;
      if (ConvertToString(Record, 0, S))
        return Error("Invalid record");
      // ANDROID: Ignore value, since we never used it anyways.
      // TheModule->addLibrary(S);
      break;
    }
    case bitc::MODULE_CODE_SECTIONNAME: {  // SECTIONNAME: [strchr x N]
      std::string S;
      if (ConvertToString(Record, 0, S))
        return Error("Invalid record");
      SectionTable.push_back(S);
      break;
    }
    case bitc::MODULE_CODE_GCNAME: {  // SECTIONNAME: [strchr x N]
      std::string S;
      if (ConvertToString(Record, 0, S))
        return Error("Invalid record");
      GCTable.push_back(S);
      break;
    }
    // GLOBALVAR: [pointer type, isconst, initid,
    //             linkage, alignment, section, visibility, threadlocal,
    //             unnamed_addr]
    case bitc::MODULE_CODE_GLOBALVAR: {
      if (Record.size() < 6)
        return Error("Invalid record");
      Type *Ty = getTypeByID(Record[0]);
      if (!Ty)
        return Error("Invalid record");
      if (!Ty->isPointerTy())
        return Error("Invalid type for value");
      unsigned AddressSpace = cast<PointerType>(Ty)->getAddressSpace();
      Ty = cast<PointerType>(Ty)->getElementType();

      bool isConstant = Record[1];
      uint64_t RawLinkage = Record[3];
      GlobalValue::LinkageTypes Linkage = getDecodedLinkage(RawLinkage);
      unsigned Alignment = (1 << Record[4]) >> 1;
      std::string Section;
      if (Record[5]) {
        if (Record[5]-1 >= SectionTable.size())
          return Error("Invalid ID");
        Section = SectionTable[Record[5]-1];
      }
      GlobalValue::VisibilityTypes Visibility = GlobalValue::DefaultVisibility;
      if (Record.size() > 6)
        Visibility = GetDecodedVisibility(Record[6]);

      GlobalVariable::ThreadLocalMode TLM = GlobalVariable::NotThreadLocal;
      if (Record.size() > 7)
        TLM = GetDecodedThreadLocalMode(Record[7]);

      GlobalValue::UnnamedAddr UnnamedAddr = GlobalValue::UnnamedAddr::None;
      if (Record.size() > 8)
        UnnamedAddr = getDecodedUnnamedAddrType(Record[8]);

      GlobalVariable *NewGV =
        new GlobalVariable(*TheModule, Ty, isConstant, Linkage, nullptr, "", nullptr,
                           TLM, AddressSpace);
      NewGV->setAlignment(Alignment);
      if (!Section.empty())
        NewGV->setSection(Section);
      NewGV->setVisibility(Visibility);
      NewGV->setUnnamedAddr(UnnamedAddr);

      ValueList.push_back(NewGV);

      // Remember which value to use for the global initializer.
      if (unsigned InitID = Record[2])
        GlobalInits.push_back(std::make_pair(NewGV, InitID-1));
      break;
    }
    // FUNCTION:  [type, callingconv, isproto, linkage, paramattr,
    //             alignment, section, visibility, gc, unnamed_addr]
    case bitc::MODULE_CODE_FUNCTION: {
      if (Record.size() < 8)
        return Error("Invalid record");
      Type *Ty = getTypeByID(Record[0]);
      if (!Ty)
        return Error("Invalid record");
      if (!Ty->isPointerTy())
        return Error("Invalid type for value");
      FunctionType *FTy =
        dyn_cast<FunctionType>(cast<PointerType>(Ty)->getElementType());
      if (!FTy)
        return Error("Invalid type for value");

      Function *Func = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                        "", TheModule);

      Func->setCallingConv(static_cast<CallingConv::ID>(Record[1]));
      bool isProto = Record[2];
      uint64_t RawLinkage = Record[3];
      Func->setLinkage(getDecodedLinkage(RawLinkage));
      Func->setAttributes(getAttributes(Record[4]));

      Func->setAlignment((1 << Record[5]) >> 1);
      if (Record[6]) {
        if (Record[6]-1 >= SectionTable.size())
          return Error("Invalid ID");
        Func->setSection(SectionTable[Record[6]-1]);
      }
      Func->setVisibility(GetDecodedVisibility(Record[7]));
      if (Record.size() > 8 && Record[8]) {
        if (Record[8]-1 > GCTable.size())
          return Error("Invalid ID");
        Func->setGC(GCTable[Record[8]-1].c_str());
      }
      GlobalValue::UnnamedAddr UnnamedAddr = GlobalValue::UnnamedAddr::None;
      if (Record.size() > 9)
        UnnamedAddr = getDecodedUnnamedAddrType(Record[9]);
      Func->setUnnamedAddr(UnnamedAddr);
      ValueList.push_back(Func);

      // If this is a function with a body, remember the prototype we are
      // creating now, so that we can match up the body with them later.
      if (!isProto) {
        Func->setIsMaterializable(true);
        FunctionsWithBodies.push_back(Func);
        if (LazyStreamer)
          DeferredFunctionInfo[Func] = 0;
      }
      break;
    }
    // ALIAS: [alias type, aliasee val#, linkage]
    // ALIAS: [alias type, aliasee val#, linkage, visibility]
    case bitc::MODULE_CODE_ALIAS_OLD: {
      if (Record.size() < 3)
        return Error("Invalid record");
      Type *Ty = getTypeByID(Record[0]);
      if (!Ty)
        return Error("Invalid record");
      auto *PTy = dyn_cast<PointerType>(Ty);
      if (!PTy)
        return Error("Invalid type for value");

      auto *NewGA =
          GlobalAlias::create(PTy->getElementType(), PTy->getAddressSpace(),
                              getDecodedLinkage(Record[2]), "", TheModule);
      // Old bitcode files didn't have visibility field.
      if (Record.size() > 3)
        NewGA->setVisibility(GetDecodedVisibility(Record[3]));
      ValueList.push_back(NewGA);
      AliasInits.push_back(std::make_pair(NewGA, Record[1]));
      break;
    }
    /// MODULE_CODE_PURGEVALS: [numvals]
    case bitc::MODULE_CODE_PURGEVALS:
      // Trim down the value list to the specified size.
      if (Record.size() < 1 || Record[0] > ValueList.size())
        return Error("Invalid record");
      ValueList.shrinkTo(Record[0]);
      break;
    }
    Record.clear();
  }
}

std::error_code BitcodeReader::ParseBitcodeInto(Module *M) {
  TheModule = nullptr;

  if (std::error_code EC = InitStream())
    return EC;

  // Sniff for the signature.
  if (Stream.Read(8) != 'B' ||
      Stream.Read(8) != 'C' ||
      Stream.Read(4) != 0x0 ||
      Stream.Read(4) != 0xC ||
      Stream.Read(4) != 0xE ||
      Stream.Read(4) != 0xD)
    return Error("Invalid bitcode signature");

  // We expect a number of well-defined blocks, though we don't necessarily
  // need to understand them all.
  while (1) {
    if (Stream.AtEndOfStream())
      return std::error_code();

    BitstreamEntry Entry =
      Stream.advance(BitstreamCursor::AF_DontAutoprocessAbbrevs);

    switch (Entry.Kind) {
    case BitstreamEntry::Error:
      return Error("Malformed block");
    case BitstreamEntry::EndBlock:
      return std::error_code();

    case BitstreamEntry::SubBlock:
      switch (Entry.ID) {
      case bitc::BLOCKINFO_BLOCK_ID:
        if (Stream.ReadBlockInfoBlock())
          return Error("Malformed block");
        break;
      case bitc::MODULE_BLOCK_ID:
        // Reject multiple MODULE_BLOCK's in a single bitstream.
        if (TheModule)
          return Error("Invalid multiple blocks");
        TheModule = M;
        if (std::error_code EC = ParseModule(false))
          return EC;
        if (LazyStreamer)
          return std::error_code();
        break;
      default:
        if (Stream.SkipBlock())
          return Error("Invalid record");
        break;
      }
      continue;
    case BitstreamEntry::Record:
      // There should be no records in the top-level of blocks.

      // The ranlib in Xcode 4 will align archive members by appending newlines
      // to the end of them. If this file size is a multiple of 4 but not 8, we
      // have to read and ignore these final 4 bytes :-(
      if (Stream.getAbbrevIDWidth() == 2 && Entry.ID == 2 &&
          Stream.Read(6) == 2 && Stream.Read(24) == 0xa0a0a &&
          Stream.AtEndOfStream())
        return std::error_code();

      return Error("Invalid record");
    }
  }
}

llvm::ErrorOr<std::string> BitcodeReader::parseModuleTriple() {
  if (Stream.EnterSubBlock(bitc::MODULE_BLOCK_ID))
    return Error("Invalid record");

  SmallVector<uint64_t, 64> Record;

  std::string Triple;
  // Read all the records for this module.
  while (1) {
    BitstreamEntry Entry = Stream.advanceSkippingSubblocks();

    switch (Entry.Kind) {
    case BitstreamEntry::SubBlock: // Handled for us already.
    case BitstreamEntry::Error:
      return Error("Malformed block");
    case BitstreamEntry::EndBlock:
      return Triple;
    case BitstreamEntry::Record:
      // The interesting case.
      break;
    }

    // Read a record.
    switch (Stream.readRecord(Entry.ID, Record)) {
    default: break;  // Default behavior, ignore unknown content.
    case bitc::MODULE_CODE_VERSION:  // VERSION: [version#]
      if (Record.size() < 1)
        return Error("Invalid record");
      // Only version #0 is supported so far.
      if (Record[0] != 0)
        return Error("Invalid record");
      break;
    case bitc::MODULE_CODE_TRIPLE: {  // TRIPLE: [strchr x N]
      std::string S;
      if (ConvertToString(Record, 0, S))
        return Error("Invalid record");
      Triple = S;
      break;
    }
    }
    Record.clear();
  }

  return Error("Invalid bitcode signature");
}

llvm::ErrorOr<std::string> BitcodeReader::parseTriple() {
  if (std::error_code EC = InitStream())
    return EC;

  // Sniff for the signature.
  if (Stream.Read(8) != 'B' ||
      Stream.Read(8) != 'C' ||
      Stream.Read(4) != 0x0 ||
      Stream.Read(4) != 0xC ||
      Stream.Read(4) != 0xE ||
      Stream.Read(4) != 0xD)
    return Error("Invalid bitcode signature");

  // We expect a number of well-defined blocks, though we don't necessarily
  // need to understand them all.
  while (1) {
    BitstreamEntry Entry = Stream.advance();

    switch (Entry.Kind) {
    case BitstreamEntry::Error:
      return Error("Malformed block");
    case BitstreamEntry::EndBlock:
      return std::error_code();

    case BitstreamEntry::SubBlock:
      if (Entry.ID == bitc::MODULE_BLOCK_ID)
        return parseModuleTriple();

      // Ignore other sub-blocks.
      if (Stream.SkipBlock())
        return Error("Malformed block");
      continue;

    case BitstreamEntry::Record:
      Stream.skipRecord(Entry.ID);
      continue;
    }
  }
}

/// ParseMetadataAttachment - Parse metadata attachments.
std::error_code BitcodeReader::ParseMetadataAttachment() {
  if (Stream.EnterSubBlock(bitc::METADATA_ATTACHMENT_ID))
    return Error("Invalid record");

  SmallVector<uint64_t, 64> Record;
  while (1) {
    BitstreamEntry Entry = Stream.advanceSkippingSubblocks();

    switch (Entry.Kind) {
    case BitstreamEntry::SubBlock: // Handled for us already.
    case BitstreamEntry::Error:
      return Error("Malformed block");
    case BitstreamEntry::EndBlock:
      return std::error_code();
    case BitstreamEntry::Record:
      // The interesting case.
      break;
    }

    // Read a metadata attachment record.
    Record.clear();
    switch (Stream.readRecord(Entry.ID, Record)) {
    default:  // Default behavior: ignore.
      break;
    case bitc::METADATA_ATTACHMENT: {
      unsigned RecordLength = Record.size();
      if (Record.empty() || (RecordLength - 1) % 2 == 1)
        return Error("Invalid record");
      Instruction *Inst = InstructionList[Record[0]];
      for (unsigned i = 1; i != RecordLength; i = i+2) {
        unsigned Kind = Record[i];
        DenseMap<unsigned, unsigned>::iterator I =
          MDKindMap.find(Kind);
        if (I == MDKindMap.end())
          return Error("Invalid ID");
        Metadata *Node = MDValueList.getValueFwdRef(Record[i + 1]);
        Inst->setMetadata(I->second, cast<MDNode>(Node));
      }
      break;
    }
    }
  }
}

/// ParseFunctionBody - Lazily parse the specified function body block.
std::error_code BitcodeReader::ParseFunctionBody(Function *F) {
  if (Stream.EnterSubBlock(bitc::FUNCTION_BLOCK_ID))
    return Error("Invalid record");

  InstructionList.clear();
  unsigned ModuleValueListSize = ValueList.size();
  unsigned ModuleMDValueListSize = MDValueList.size();

  // Add all the function arguments to the value table.
  for(Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I)
    ValueList.push_back(&*I);

  unsigned NextValueNo = ValueList.size();
  BasicBlock *CurBB = nullptr;
  unsigned CurBBNo = 0;

  DebugLoc LastLoc;

  // Read all the records.
  SmallVector<uint64_t, 64> Record;
  while (1) {
    unsigned Code = Stream.ReadCode();
    if (Code == bitc::END_BLOCK) {
      if (Stream.ReadBlockEnd())
        return Error("Malformed block");
      break;
    }

    if (Code == bitc::ENTER_SUBBLOCK) {
      switch (Stream.ReadSubBlockID()) {
      default:  // Skip unknown content.
        if (Stream.SkipBlock())
          return Error("Invalid record");
        break;
      case bitc::CONSTANTS_BLOCK_ID:
        if (std::error_code EC = ParseConstants())
          return EC;
        NextValueNo = ValueList.size();
        break;
      case bitc::VALUE_SYMTAB_BLOCK_ID:
        if (std::error_code EC = ParseValueSymbolTable())
          return EC;
        break;
      case bitc::METADATA_ATTACHMENT_ID:
        if (std::error_code EC = ParseMetadataAttachment())
          return EC;
        break;
      case bitc::METADATA_BLOCK_ID:
        if (std::error_code EC = ParseMetadata())
          return EC;
        break;
      }
      continue;
    }

    if (Code == bitc::DEFINE_ABBREV) {
      Stream.ReadAbbrevRecord();
      continue;
    }

    // Read a record.
    Record.clear();
    Instruction *I = nullptr;
    unsigned BitCode = Stream.readRecord(Code, Record);
    switch (BitCode) {
    default: // Default behavior: reject
      return Error("Invalid value");
    case bitc::FUNC_CODE_DECLAREBLOCKS:     // DECLAREBLOCKS: [nblocks]
      if (Record.size() < 1 || Record[0] == 0)
        return Error("Invalid record");
      // Create all the basic blocks for the function.
      FunctionBBs.resize(Record[0]);
      for (unsigned i = 0, e = FunctionBBs.size(); i != e; ++i)
        FunctionBBs[i] = BasicBlock::Create(Context, "", F);
      CurBB = FunctionBBs[0];
      continue;

    case bitc::FUNC_CODE_DEBUG_LOC_AGAIN:  // DEBUG_LOC_AGAIN
      // This record indicates that the last instruction is at the same
      // location as the previous instruction with a location.
      I = nullptr;

      // Get the last instruction emitted.
      if (CurBB && !CurBB->empty())
        I = &CurBB->back();
      else if (CurBBNo && FunctionBBs[CurBBNo-1] &&
               !FunctionBBs[CurBBNo-1]->empty())
        I = &FunctionBBs[CurBBNo-1]->back();

      if (!I)
        return Error("Invalid record");
      I->setDebugLoc(LastLoc);
      I = nullptr;
      continue;

    case bitc::FUNC_CODE_DEBUG_LOC: {      // DEBUG_LOC: [line, col, scope, ia]
      I = nullptr;     // Get the last instruction emitted.
      if (CurBB && !CurBB->empty())
        I = &CurBB->back();
      else if (CurBBNo && FunctionBBs[CurBBNo-1] &&
               !FunctionBBs[CurBBNo-1]->empty())
        I = &FunctionBBs[CurBBNo-1]->back();
      if (!I || Record.size() < 4)
        return Error("Invalid record");

      unsigned Line = Record[0], Col = Record[1];
      unsigned ScopeID = Record[2], IAID = Record[3];

      MDNode *Scope = nullptr, *IA = nullptr;
      if (ScopeID) Scope = cast<MDNode>(MDValueList.getValueFwdRef(ScopeID-1));
      if (IAID)    IA = cast<MDNode>(MDValueList.getValueFwdRef(IAID-1));
      LastLoc = DebugLoc::get(Line, Col, Scope, IA);
      I->setDebugLoc(LastLoc);
      I = nullptr;
      continue;
    }

    case bitc::FUNC_CODE_INST_BINOP: {    // BINOP: [opval, ty, opval, opcode]
      unsigned OpNum = 0;
      Value *LHS, *RHS;
      if (getValueTypePair(Record, OpNum, NextValueNo, LHS) ||
          getValue(Record, OpNum, LHS->getType(), RHS) ||
          OpNum+1 > Record.size())
        return Error("Invalid record");

      int Opc = GetDecodedBinaryOpcode(Record[OpNum++], LHS->getType());
      if (Opc == -1)
        return Error("Invalid record");
      I = BinaryOperator::Create((Instruction::BinaryOps)Opc, LHS, RHS);
      InstructionList.push_back(I);
      if (OpNum < Record.size()) {
        if (Opc == Instruction::Add ||
            Opc == Instruction::Sub ||
            Opc == Instruction::Mul ||
            Opc == Instruction::Shl) {
          if (Record[OpNum] & (1 << bitc::OBO_NO_SIGNED_WRAP))
            cast<BinaryOperator>(I)->setHasNoSignedWrap(true);
          if (Record[OpNum] & (1 << bitc::OBO_NO_UNSIGNED_WRAP))
            cast<BinaryOperator>(I)->setHasNoUnsignedWrap(true);
        } else if (Opc == Instruction::SDiv ||
                   Opc == Instruction::UDiv ||
                   Opc == Instruction::LShr ||
                   Opc == Instruction::AShr) {
          if (Record[OpNum] & (1 << bitc::PEO_EXACT))
            cast<BinaryOperator>(I)->setIsExact(true);
        }
      }
      break;
    }
    case bitc::FUNC_CODE_INST_CAST: {    // CAST: [opval, opty, destty, castopc]
      unsigned OpNum = 0;
      Value *Op;
      if (getValueTypePair(Record, OpNum, NextValueNo, Op) ||
          OpNum+2 != Record.size())
        return Error("Invalid record");

      Type *ResTy = getTypeByID(Record[OpNum]);
      int Opc = GetDecodedCastOpcode(Record[OpNum+1]);
      if (Opc == -1 || !ResTy)
        return Error("Invalid record");
      I = CastInst::Create((Instruction::CastOps)Opc, Op, ResTy);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_INBOUNDS_GEP_OLD:
    case bitc::FUNC_CODE_INST_GEP_OLD: // GEP: [n x operands]
    case bitc::FUNC_CODE_INST_GEP: { // GEP: [n x operands]
      unsigned OpNum = 0;

      Type *Ty;
      bool InBounds;

      if (BitCode == bitc::FUNC_CODE_INST_GEP) {
        InBounds = Record[OpNum++];
        Ty = getTypeByID(Record[OpNum++]);
      } else {
        InBounds = BitCode == bitc::FUNC_CODE_INST_INBOUNDS_GEP_OLD;
        Ty = nullptr;
      }

      Value *BasePtr;
      if (getValueTypePair(Record, OpNum, NextValueNo, BasePtr))
        return Error("Invalid record");

      if (Ty &&
          Ty !=
              cast<SequentialType>(BasePtr->getType()->getScalarType())
                  ->getElementType())
        return Error(
            "Explicit gep type does not match pointee type of pointer operand");

      SmallVector<Value*, 16> GEPIdx;
      while (OpNum != Record.size()) {
        Value *Op;
        if (getValueTypePair(Record, OpNum, NextValueNo, Op))
          return Error("Invalid record");
        GEPIdx.push_back(Op);
      }

      I = GetElementPtrInst::Create(Ty, BasePtr, GEPIdx);

      InstructionList.push_back(I);
      if (InBounds)
        cast<GetElementPtrInst>(I)->setIsInBounds(true);
      break;
    }

    case bitc::FUNC_CODE_INST_EXTRACTVAL: {
                                       // EXTRACTVAL: [opty, opval, n x indices]
      unsigned OpNum = 0;
      Value *Agg;
      if (getValueTypePair(Record, OpNum, NextValueNo, Agg))
        return Error("Invalid record");

      SmallVector<unsigned, 4> EXTRACTVALIdx;
      for (unsigned RecSize = Record.size();
           OpNum != RecSize; ++OpNum) {
        uint64_t Index = Record[OpNum];
        if ((unsigned)Index != Index)
          return Error("Invalid value");
        EXTRACTVALIdx.push_back((unsigned)Index);
      }

      I = ExtractValueInst::Create(Agg, EXTRACTVALIdx);
      InstructionList.push_back(I);
      break;
    }

    case bitc::FUNC_CODE_INST_INSERTVAL: {
                           // INSERTVAL: [opty, opval, opty, opval, n x indices]
      unsigned OpNum = 0;
      Value *Agg;
      if (getValueTypePair(Record, OpNum, NextValueNo, Agg))
        return Error("Invalid record");
      Value *Val;
      if (getValueTypePair(Record, OpNum, NextValueNo, Val))
        return Error("Invalid record");

      SmallVector<unsigned, 4> INSERTVALIdx;
      for (unsigned RecSize = Record.size();
           OpNum != RecSize; ++OpNum) {
        uint64_t Index = Record[OpNum];
        if ((unsigned)Index != Index)
          return Error("Invalid value");
        INSERTVALIdx.push_back((unsigned)Index);
      }

      I = InsertValueInst::Create(Agg, Val, INSERTVALIdx);
      InstructionList.push_back(I);
      break;
    }

    case bitc::FUNC_CODE_INST_SELECT: { // SELECT: [opval, ty, opval, opval]
      // obsolete form of select
      // handles select i1 ... in old bitcode
      unsigned OpNum = 0;
      Value *TrueVal, *FalseVal, *Cond;
      if (getValueTypePair(Record, OpNum, NextValueNo, TrueVal) ||
          getValue(Record, OpNum, TrueVal->getType(), FalseVal) ||
          getValue(Record, OpNum, Type::getInt1Ty(Context), Cond))
        return Error("Invalid record");

      I = SelectInst::Create(Cond, TrueVal, FalseVal);
      InstructionList.push_back(I);
      break;
    }

    case bitc::FUNC_CODE_INST_VSELECT: {// VSELECT: [ty,opval,opval,predty,pred]
      // new form of select
      // handles select i1 or select [N x i1]
      unsigned OpNum = 0;
      Value *TrueVal, *FalseVal, *Cond;
      if (getValueTypePair(Record, OpNum, NextValueNo, TrueVal) ||
          getValue(Record, OpNum, TrueVal->getType(), FalseVal) ||
          getValueTypePair(Record, OpNum, NextValueNo, Cond))
        return Error("Invalid record");

      // select condition can be either i1 or [N x i1]
      if (VectorType* vector_type =
          dyn_cast<VectorType>(Cond->getType())) {
        // expect <n x i1>
        if (vector_type->getElementType() != Type::getInt1Ty(Context))
          return Error("Invalid type for value");
      } else {
        // expect i1
        if (Cond->getType() != Type::getInt1Ty(Context))
          return Error("Invalid type for value");
      }

      I = SelectInst::Create(Cond, TrueVal, FalseVal);
      InstructionList.push_back(I);
      break;
    }

    case bitc::FUNC_CODE_INST_EXTRACTELT: { // EXTRACTELT: [opty, opval, opval]
      unsigned OpNum = 0;
      Value *Vec, *Idx;
      if (getValueTypePair(Record, OpNum, NextValueNo, Vec) ||
          getValue(Record, OpNum, Type::getInt32Ty(Context), Idx))
        return Error("Invalid record");
      I = ExtractElementInst::Create(Vec, Idx);
      InstructionList.push_back(I);
      break;
    }

    case bitc::FUNC_CODE_INST_INSERTELT: { // INSERTELT: [ty, opval,opval,opval]
      unsigned OpNum = 0;
      Value *Vec, *Elt, *Idx;
      if (getValueTypePair(Record, OpNum, NextValueNo, Vec) ||
          getValue(Record, OpNum,
                   cast<VectorType>(Vec->getType())->getElementType(), Elt) ||
          getValue(Record, OpNum, Type::getInt32Ty(Context), Idx))
        return Error("Invalid record");
      I = InsertElementInst::Create(Vec, Elt, Idx);
      InstructionList.push_back(I);
      break;
    }

    case bitc::FUNC_CODE_INST_SHUFFLEVEC: {// SHUFFLEVEC: [opval,ty,opval,opval]
      unsigned OpNum = 0;
      Value *Vec1, *Vec2, *Mask;
      if (getValueTypePair(Record, OpNum, NextValueNo, Vec1) ||
          getValue(Record, OpNum, Vec1->getType(), Vec2))
        return Error("Invalid record");

      if (getValueTypePair(Record, OpNum, NextValueNo, Mask))
        return Error("Invalid record");
      I = new ShuffleVectorInst(Vec1, Vec2, Mask);
      InstructionList.push_back(I);
      break;
    }

    case bitc::FUNC_CODE_INST_CMP:   // CMP: [opty, opval, opval, pred]
      // Old form of ICmp/FCmp returning bool
      // Existed to differentiate between icmp/fcmp and vicmp/vfcmp which were
      // both legal on vectors but had different behaviour.
    case bitc::FUNC_CODE_INST_CMP2: { // CMP2: [opty, opval, opval, pred]
      // FCmp/ICmp returning bool or vector of bool

      unsigned OpNum = 0;
      Value *LHS, *RHS;
      if (getValueTypePair(Record, OpNum, NextValueNo, LHS) ||
          getValue(Record, OpNum, LHS->getType(), RHS) ||
          OpNum+1 != Record.size())
        return Error("Invalid record");

      if (LHS->getType()->isFPOrFPVectorTy())
        I = new FCmpInst((FCmpInst::Predicate)Record[OpNum], LHS, RHS);
      else
        I = new ICmpInst((ICmpInst::Predicate)Record[OpNum], LHS, RHS);
      InstructionList.push_back(I);
      break;
    }

    case bitc::FUNC_CODE_INST_RET: // RET: [opty,opval<optional>]
      {
        unsigned Size = Record.size();
        if (Size == 0) {
          I = ReturnInst::Create(Context);
          InstructionList.push_back(I);
          break;
        }

        unsigned OpNum = 0;
        Value *Op = nullptr;
        if (getValueTypePair(Record, OpNum, NextValueNo, Op))
          return Error("Invalid record");
        if (OpNum != Record.size())
          return Error("Invalid record");

        I = ReturnInst::Create(Context, Op);
        InstructionList.push_back(I);
        break;
      }
    case bitc::FUNC_CODE_INST_BR: { // BR: [bb#, bb#, opval] or [bb#]
      if (Record.size() != 1 && Record.size() != 3)
        return Error("Invalid record");
      BasicBlock *TrueDest = getBasicBlock(Record[0]);
      if (!TrueDest)
        return Error("Invalid record");

      if (Record.size() == 1) {
        I = BranchInst::Create(TrueDest);
        InstructionList.push_back(I);
      }
      else {
        BasicBlock *FalseDest = getBasicBlock(Record[1]);
        Value *Cond = getFnValueByID(Record[2], Type::getInt1Ty(Context));
        if (!FalseDest || !Cond)
          return Error("Invalid record");
        I = BranchInst::Create(TrueDest, FalseDest, Cond);
        InstructionList.push_back(I);
      }
      break;
    }
    case bitc::FUNC_CODE_INST_SWITCH: { // SWITCH: [opty, op0, op1, ...]
      if (Record.size() < 3 || (Record.size() & 1) == 0)
        return Error("Invalid record");
      Type *OpTy = getTypeByID(Record[0]);
      Value *Cond = getFnValueByID(Record[1], OpTy);
      BasicBlock *Default = getBasicBlock(Record[2]);
      if (!OpTy || !Cond || !Default)
        return Error("Invalid record");
      unsigned NumCases = (Record.size()-3)/2;
      SwitchInst *SI = SwitchInst::Create(Cond, Default, NumCases);
      InstructionList.push_back(SI);
      for (unsigned i = 0, e = NumCases; i != e; ++i) {
        ConstantInt *CaseVal =
          dyn_cast_or_null<ConstantInt>(getFnValueByID(Record[3+i*2], OpTy));
        BasicBlock *DestBB = getBasicBlock(Record[1+3+i*2]);
        if (!CaseVal || !DestBB) {
          delete SI;
          return Error("Invalid record");
        }
        SI->addCase(CaseVal, DestBB);
      }
      I = SI;
      break;
    }
    case bitc::FUNC_CODE_INST_INDIRECTBR: { // INDIRECTBR: [opty, op0, op1, ...]
      if (Record.size() < 2)
        return Error("Invalid record");
      Type *OpTy = getTypeByID(Record[0]);
      Value *Address = getFnValueByID(Record[1], OpTy);
      if (!OpTy || !Address)
        return Error("Invalid record");
      unsigned NumDests = Record.size()-2;
      IndirectBrInst *IBI = IndirectBrInst::Create(Address, NumDests);
      InstructionList.push_back(IBI);
      for (unsigned i = 0, e = NumDests; i != e; ++i) {
        if (BasicBlock *DestBB = getBasicBlock(Record[2+i])) {
          IBI->addDestination(DestBB);
        } else {
          delete IBI;
          return Error("Invalid record");
        }
      }
      I = IBI;
      break;
    }

    case bitc::FUNC_CODE_INST_INVOKE: {
      // INVOKE: [attrs, cc, normBB, unwindBB, fnty, op0,op1,op2, ...]
      if (Record.size() < 4)
        return Error("Invalid record");
      AttributeSet PAL = getAttributes(Record[0]);
      unsigned CCInfo = Record[1];
      BasicBlock *NormalBB = getBasicBlock(Record[2]);
      BasicBlock *UnwindBB = getBasicBlock(Record[3]);

      unsigned OpNum = 4;
      Value *Callee;
      if (getValueTypePair(Record, OpNum, NextValueNo, Callee))
        return Error("Invalid record");

      PointerType *CalleeTy = dyn_cast<PointerType>(Callee->getType());
      FunctionType *FTy = !CalleeTy ? nullptr :
        dyn_cast<FunctionType>(CalleeTy->getElementType());

      // Check that the right number of fixed parameters are here.
      if (!FTy || !NormalBB || !UnwindBB ||
          Record.size() < OpNum+FTy->getNumParams())
        return Error("Invalid record");

      SmallVector<Value*, 16> Ops;
      for (unsigned i = 0, e = FTy->getNumParams(); i != e; ++i, ++OpNum) {
        Ops.push_back(getFnValueByID(Record[OpNum], FTy->getParamType(i)));
        if (!Ops.back())
          return Error("Invalid record");
      }

      if (!FTy->isVarArg()) {
        if (Record.size() != OpNum)
          return Error("Invalid record");
      } else {
        // Read type/value pairs for varargs params.
        while (OpNum != Record.size()) {
          Value *Op;
          if (getValueTypePair(Record, OpNum, NextValueNo, Op))
            return Error("Invalid record");
          Ops.push_back(Op);
        }
      }

      I = InvokeInst::Create(Callee, NormalBB, UnwindBB, Ops);
      InstructionList.push_back(I);
      cast<InvokeInst>(I)->setCallingConv(
        static_cast<CallingConv::ID>(CCInfo));
      cast<InvokeInst>(I)->setAttributes(PAL);
      break;
    }
    case bitc::FUNC_CODE_INST_RESUME: { // RESUME: [opval]
      unsigned Idx = 0;
      Value *Val = nullptr;
      if (getValueTypePair(Record, Idx, NextValueNo, Val))
        return Error("Invalid record");
      I = ResumeInst::Create(Val);
      InstructionList.push_back(I);
      break;
    }
    case FUNC_CODE_INST_UNWIND_2_7: { // UNWIND_OLD
      // 'unwind' instruction has been removed in LLVM 3.1
      // Replace 'unwind' with 'landingpad' and 'resume'.
      Type *ExnTy = StructType::get(Type::getInt8PtrTy(Context),
                                    Type::getInt32Ty(Context), nullptr);

      LandingPadInst *LP = LandingPadInst::Create(ExnTy, 1);
      LP->setCleanup(true);

      CurBB->getInstList().push_back(LP);
      I = ResumeInst::Create(LP);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_UNREACHABLE: // UNREACHABLE
      I = new UnreachableInst(Context);
      InstructionList.push_back(I);
      break;
    case bitc::FUNC_CODE_INST_PHI: { // PHI: [ty, val0,bb0, ...]
      if (Record.size() < 1 || ((Record.size()-1)&1))
        return Error("Invalid record");
      Type *Ty = getTypeByID(Record[0]);
      if (!Ty)
        return Error("Invalid record");

      PHINode *PN = PHINode::Create(Ty, (Record.size()-1)/2);
      InstructionList.push_back(PN);

      for (unsigned i = 0, e = Record.size()-1; i != e; i += 2) {
        Value *V = getFnValueByID(Record[1+i], Ty);
        BasicBlock *BB = getBasicBlock(Record[2+i]);
        if (!V || !BB)
          return Error("Invalid record");
        PN->addIncoming(V, BB);
      }
      I = PN;
      break;
    }

    case bitc::FUNC_CODE_INST_LANDINGPAD_OLD: {
      // LANDINGPAD: [ty, val, val, num, (id0,val0 ...)?]
      unsigned Idx = 0;
      if (Record.size() < 4)
        return Error("Invalid record");
      Type *Ty = getTypeByID(Record[Idx++]);
      if (!Ty)
        return Error("Invalid record");
      Value *PersFn = nullptr;
      if (getValueTypePair(Record, Idx, NextValueNo, PersFn))
        return Error("Invalid record");

      bool IsCleanup = !!Record[Idx++];
      unsigned NumClauses = Record[Idx++];
      LandingPadInst *LP = LandingPadInst::Create(Ty, NumClauses);
      LP->setCleanup(IsCleanup);
      for (unsigned J = 0; J != NumClauses; ++J) {
        LandingPadInst::ClauseType CT =
          LandingPadInst::ClauseType(Record[Idx++]); (void)CT;
        Value *Val;

        if (getValueTypePair(Record, Idx, NextValueNo, Val)) {
          delete LP;
          return Error("Invalid record");
        }

        assert((CT != LandingPadInst::Catch ||
                !isa<ArrayType>(Val->getType())) &&
               "Catch clause has a invalid type!");
        assert((CT != LandingPadInst::Filter ||
                isa<ArrayType>(Val->getType())) &&
               "Filter clause has invalid type!");
        LP->addClause(cast<Constant>(Val));
      }

      I = LP;
      InstructionList.push_back(I);
      break;
    }

    case bitc::FUNC_CODE_INST_ALLOCA: { // ALLOCA: [instty, opty, op, align]
      if (Record.size() != 4)
        return Error("Invalid record");
      PointerType *Ty =
        dyn_cast_or_null<PointerType>(getTypeByID(Record[0]));
      Type *OpTy = getTypeByID(Record[1]);
      Value *Size = getFnValueByID(Record[2], OpTy);
      unsigned Align = Record[3];
      if (!Ty || !Size)
        return Error("Invalid record");
      I = new AllocaInst(Ty->getElementType(), Size, (1 << Align) >> 1);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_LOAD: { // LOAD: [opty, op, align, vol]
      unsigned OpNum = 0;
      Value *Op;
      if (getValueTypePair(Record, OpNum, NextValueNo, Op) ||
          OpNum+2 != Record.size())
        return Error("Invalid record");

      I = new LoadInst(Op, "", Record[OpNum+1], (1 << Record[OpNum]) >> 1);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_LOADATOMIC: {
       // LOADATOMIC: [opty, op, align, vol, ordering, synchscope]
      unsigned OpNum = 0;
      Value *Op;
      if (getValueTypePair(Record, OpNum, NextValueNo, Op) ||
          OpNum+4 != Record.size())
        return Error("Invalid record");

      AtomicOrdering Ordering = GetDecodedOrdering(Record[OpNum+2]);
      if (Ordering == AtomicOrdering::NotAtomic ||
          Ordering == AtomicOrdering::Release ||
          Ordering == AtomicOrdering::AcquireRelease)
        return Error("Invalid record");
      if (Ordering != AtomicOrdering::NotAtomic && Record[OpNum] == 0)
        return Error("Invalid record");
      SynchronizationScope SynchScope = GetDecodedSynchScope(Record[OpNum+3]);

      I = new LoadInst(Op, "", Record[OpNum+1], (1 << Record[OpNum]) >> 1,
                       Ordering, SynchScope);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_STORE_OLD: { // STORE2:[ptrty, ptr, val, align, vol]
      unsigned OpNum = 0;
      Value *Val, *Ptr;
      if (getValueTypePair(Record, OpNum, NextValueNo, Ptr) ||
          getValue(Record, OpNum,
                    cast<PointerType>(Ptr->getType())->getElementType(), Val) ||
          OpNum+2 != Record.size())
        return Error("Invalid record");

      I = new StoreInst(Val, Ptr, Record[OpNum+1], (1 << Record[OpNum]) >> 1);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_STOREATOMIC_OLD: {
      // STOREATOMIC: [ptrty, ptr, val, align, vol, ordering, synchscope]
      unsigned OpNum = 0;
      Value *Val, *Ptr;
      if (getValueTypePair(Record, OpNum, NextValueNo, Ptr) ||
          getValue(Record, OpNum,
                    cast<PointerType>(Ptr->getType())->getElementType(), Val) ||
          OpNum+4 != Record.size())
        return Error("Invalid record");

      AtomicOrdering Ordering = GetDecodedOrdering(Record[OpNum+2]);
      if (Ordering == AtomicOrdering::NotAtomic ||
          Ordering == AtomicOrdering::Acquire ||
          Ordering == AtomicOrdering::AcquireRelease)
        return Error("Invalid record");
      SynchronizationScope SynchScope = GetDecodedSynchScope(Record[OpNum+3]);
      if (Ordering != AtomicOrdering::NotAtomic && Record[OpNum] == 0)
        return Error("Invalid record");

      I = new StoreInst(Val, Ptr, Record[OpNum+1], (1 << Record[OpNum]) >> 1,
                        Ordering, SynchScope);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_CMPXCHG_OLD: {
      // CMPXCHG:[ptrty, ptr, cmp, new, vol, ordering, synchscope]
      unsigned OpNum = 0;
      Value *Ptr, *Cmp, *New;
      if (getValueTypePair(Record, OpNum, NextValueNo, Ptr) ||
          getValue(Record, OpNum,
                    cast<PointerType>(Ptr->getType())->getElementType(), Cmp) ||
          getValue(Record, OpNum,
                    cast<PointerType>(Ptr->getType())->getElementType(), New) ||
          OpNum+3 != Record.size())
        return Error("Invalid record");
      AtomicOrdering Ordering = GetDecodedOrdering(Record[OpNum+1]);
      if (Ordering == AtomicOrdering::NotAtomic ||
          Ordering == AtomicOrdering::Unordered)
        return Error("Invalid record");
      SynchronizationScope SynchScope = GetDecodedSynchScope(Record[OpNum+2]);
      I = new AtomicCmpXchgInst(Ptr, Cmp, New, Ordering, Ordering, SynchScope);
      cast<AtomicCmpXchgInst>(I)->setVolatile(Record[OpNum]);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_ATOMICRMW: {
      // ATOMICRMW:[ptrty, ptr, val, op, vol, ordering, synchscope]
      unsigned OpNum = 0;
      Value *Ptr, *Val;
      if (getValueTypePair(Record, OpNum, NextValueNo, Ptr) ||
          getValue(Record, OpNum,
                    cast<PointerType>(Ptr->getType())->getElementType(), Val) ||
          OpNum+4 != Record.size())
        return Error("Invalid record");
      AtomicRMWInst::BinOp Operation = GetDecodedRMWOperation(Record[OpNum]);
      if (Operation < AtomicRMWInst::FIRST_BINOP ||
          Operation > AtomicRMWInst::LAST_BINOP)
        return Error("Invalid record");
      AtomicOrdering Ordering = GetDecodedOrdering(Record[OpNum+2]);
      if (Ordering == AtomicOrdering::NotAtomic ||
          Ordering == AtomicOrdering::Unordered)
        return Error("Invalid record");
      SynchronizationScope SynchScope = GetDecodedSynchScope(Record[OpNum+3]);
      I = new AtomicRMWInst(Operation, Ptr, Val, Ordering, SynchScope);
      cast<AtomicRMWInst>(I)->setVolatile(Record[OpNum+1]);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_FENCE: { // FENCE:[ordering, synchscope]
      if (2 != Record.size())
        return Error("Invalid record");
      AtomicOrdering Ordering = GetDecodedOrdering(Record[0]);
      if (Ordering == AtomicOrdering::NotAtomic ||
          Ordering == AtomicOrdering::Unordered ||
          Ordering == AtomicOrdering::Monotonic)
        return Error("Invalid record");
      SynchronizationScope SynchScope = GetDecodedSynchScope(Record[1]);
      I = new FenceInst(Context, Ordering, SynchScope);
      InstructionList.push_back(I);
      break;
    }
    case bitc::FUNC_CODE_INST_CALL: {
      // CALL: [paramattrs, cc, fnty, fnid, arg0, arg1...]
      if (Record.size() < 3)
        return Error("Invalid record");

      AttributeSet PAL = getAttributes(Record[0]);
      unsigned CCInfo = Record[1];

      unsigned OpNum = 2;
      Value *Callee;
      if (getValueTypePair(Record, OpNum, NextValueNo, Callee))
        return Error("Invalid record");

      PointerType *OpTy = dyn_cast<PointerType>(Callee->getType());
      FunctionType *FTy = nullptr;
      if (OpTy) FTy = dyn_cast<FunctionType>(OpTy->getElementType());
      if (!FTy || Record.size() < FTy->getNumParams()+OpNum)
        return Error("Invalid record");

      SmallVector<Value*, 16> Args;
      // Read the fixed params.
      for (unsigned i = 0, e = FTy->getNumParams(); i != e; ++i, ++OpNum) {
        if (FTy->getParamType(i)->isLabelTy())
          Args.push_back(getBasicBlock(Record[OpNum]));
        else
          Args.push_back(getFnValueByID(Record[OpNum], FTy->getParamType(i)));
        if (!Args.back())
          return Error("Invalid record");
      }

      // Read type/value pairs for varargs params.
      if (!FTy->isVarArg()) {
        if (OpNum != Record.size())
          return Error("Invalid record");
      } else {
        while (OpNum != Record.size()) {
          Value *Op;
          if (getValueTypePair(Record, OpNum, NextValueNo, Op))
            return Error("Invalid record");
          Args.push_back(Op);
        }
      }

      I = CallInst::Create(Callee, Args);
      InstructionList.push_back(I);
      cast<CallInst>(I)->setCallingConv(
        static_cast<CallingConv::ID>(CCInfo>>1));
      cast<CallInst>(I)->setTailCall(CCInfo & 1);
      cast<CallInst>(I)->setAttributes(PAL);
      break;
    }
    case bitc::FUNC_CODE_INST_VAARG: { // VAARG: [valistty, valist, instty]
      if (Record.size() < 3)
        return Error("Invalid record");
      Type *OpTy = getTypeByID(Record[0]);
      Value *Op = getFnValueByID(Record[1], OpTy);
      Type *ResTy = getTypeByID(Record[2]);
      if (!OpTy || !Op || !ResTy)
        return Error("Invalid record");
      I = new VAArgInst(Op, ResTy);
      InstructionList.push_back(I);
      break;
    }
    }

    // Add instruction to end of current BB.  If there is no current BB, reject
    // this file.
    if (!CurBB) {
      delete I;
      return Error("Invalid instruction with no BB");
    }
    CurBB->getInstList().push_back(I);

    // If this was a terminator instruction, move to the next block.
    if (isa<TerminatorInst>(I)) {
      ++CurBBNo;
      CurBB = CurBBNo < FunctionBBs.size() ? FunctionBBs[CurBBNo] : nullptr;
    }

    // Non-void values get registered in the value table for future use.
    if (I && !I->getType()->isVoidTy())
      ValueList.AssignValue(I, NextValueNo++);
  }

  // Check the function list for unresolved values.
  if (Argument *A = dyn_cast<Argument>(ValueList.back())) {
    if (!A->getParent()) {
      // We found at least one unresolved value.  Nuke them all to avoid leaks.
      for (unsigned i = ModuleValueListSize, e = ValueList.size(); i != e; ++i){
        if ((A = dyn_cast_or_null<Argument>(ValueList[i])) && !A->getParent()) {
          A->replaceAllUsesWith(UndefValue::get(A->getType()));
          delete A;
        }
      }
      return Error("Never resolved value found in function");
    }
  }

  // FIXME: Check for unresolved forward-declared metadata references
  // and clean up leaks.

  // See if anything took the address of blocks in this function.  If so,
  // resolve them now.
  DenseMap<Function*, std::vector<BlockAddrRefTy> >::iterator BAFRI =
    BlockAddrFwdRefs.find(F);
  if (BAFRI != BlockAddrFwdRefs.end()) {
    std::vector<BlockAddrRefTy> &RefList = BAFRI->second;
    for (unsigned i = 0, e = RefList.size(); i != e; ++i) {
      unsigned BlockIdx = RefList[i].first;
      if (BlockIdx >= FunctionBBs.size())
        return Error("Invalid ID");

      GlobalVariable *FwdRef = RefList[i].second;
      FwdRef->replaceAllUsesWith(BlockAddress::get(F, FunctionBBs[BlockIdx]));
      FwdRef->eraseFromParent();
    }

    BlockAddrFwdRefs.erase(BAFRI);
  }

  // Trim the value list down to the size it was before we parsed this function.
  ValueList.shrinkTo(ModuleValueListSize);
  MDValueList.shrinkTo(ModuleMDValueListSize);
  std::vector<BasicBlock*>().swap(FunctionBBs);
  return std::error_code();
}

//===----------------------------------------------------------------------===//
// GVMaterializer implementation
//===----------------------------------------------------------------------===//

void BitcodeReader::releaseBuffer() { Buffer.release(); }

std::error_code BitcodeReader::materialize(GlobalValue *GV) {
  if (std::error_code EC = materializeMetadata())
    return EC;

  Function *F = dyn_cast<Function>(GV);
  // If it's not a function or is already material, ignore the request.
  if (!F || !F->isMaterializable())
    return std::error_code();

  DenseMap<Function*, uint64_t>::iterator DFII = DeferredFunctionInfo.find(F);
  assert(DFII != DeferredFunctionInfo.end() && "Deferred function not found!");

  // Move the bit stream to the saved position of the deferred function body.
  Stream.JumpToBit(DFII->second);

  if (std::error_code EC = ParseFunctionBody(F))
    return EC;
  F->setIsMaterializable(false);

  // Upgrade any old intrinsic calls in the function.
  for (UpgradedIntrinsicMap::iterator I = UpgradedIntrinsics.begin(),
       E = UpgradedIntrinsics.end(); I != E; ++I) {
    if (I->first != I->second) {
      for (auto UI = I->first->user_begin(), UE = I->first->user_end();
           UI != UE;) {
        if (CallInst* CI = dyn_cast<CallInst>(*UI++))
          UpgradeIntrinsicCall(CI, I->second);
      }
    }
  }

  return std::error_code();
}

bool BitcodeReader::isDematerializable(const GlobalValue *GV) const {
  const Function *F = dyn_cast<Function>(GV);
  if (!F || F->isDeclaration())
    return false;
  return DeferredFunctionInfo.count(const_cast<Function*>(F));
}

void BitcodeReader::dematerialize(GlobalValue *GV) {
  Function *F = dyn_cast<Function>(GV);
  // If this function isn't dematerializable, this is a noop.
  if (!F || !isDematerializable(F))
    return;

  assert(DeferredFunctionInfo.count(F) && "No info to read function later?");

  // Just forget the function body, we can remat it later.
  F->deleteBody();
  F->setIsMaterializable(true);
}

std::error_code BitcodeReader::materializeModule() {
  // Iterate over the module, deserializing any functions that are still on
  // disk.
  for (Module::iterator F = TheModule->begin(), E = TheModule->end();
       F != E; ++F) {
    if (std::error_code EC = materialize(&*F))
      return EC;
  }
  // At this point, if there are any function bodies, the current bit is
  // pointing to the END_BLOCK record after them. Now make sure the rest
  // of the bits in the module have been read.
  if (NextUnreadBit)
    ParseModule(true);

  // Upgrade any intrinsic calls that slipped through (should not happen!) and
  // delete the old functions to clean up. We can't do this unless the entire
  // module is materialized because there could always be another function body
  // with calls to the old function.
  for (std::vector<std::pair<Function*, Function*> >::iterator I =
       UpgradedIntrinsics.begin(), E = UpgradedIntrinsics.end(); I != E; ++I) {
    if (I->first != I->second) {
      for (auto UI = I->first->user_begin(), UE = I->first->user_end();
           UI != UE;) {
        if (CallInst* CI = dyn_cast<CallInst>(*UI++))
          UpgradeIntrinsicCall(CI, I->second);
      }
      if (!I->first->use_empty())
        I->first->replaceAllUsesWith(I->second);
      I->first->eraseFromParent();
    }
  }
  std::vector<std::pair<Function*, Function*> >().swap(UpgradedIntrinsics);

  // Upgrade to new EH scheme. N.B. This will go away in 3.1.
  UpgradeExceptionHandling(TheModule);

  // Check debug info intrinsics.
  CheckDebugInfoIntrinsics(TheModule);

  return std::error_code();
}

std::vector<StructType *> BitcodeReader::getIdentifiedStructTypes() const {
  return IdentifiedStructTypes;
}

std::error_code BitcodeReader::InitStream() {
  if (LazyStreamer)
    return InitLazyStream();
  return InitStreamFromBuffer();
}

std::error_code BitcodeReader::InitStreamFromBuffer() {
  const unsigned char *BufPtr = (const unsigned char*)Buffer->getBufferStart();
  const unsigned char *BufEnd = BufPtr+Buffer->getBufferSize();

  if (Buffer->getBufferSize() & 3)
    return Error("Invalid bitcode signature");

  // If we have a wrapper header, parse it and ignore the non-bc file contents.
  // The magic number is 0x0B17C0DE stored in little endian.
  if (isBitcodeWrapper(BufPtr, BufEnd))
    if (SkipBitcodeWrapperHeader(BufPtr, BufEnd, true))
      return Error("Invalid bitcode wrapper header");

  StreamFile.reset(new BitstreamReader(BufPtr, BufEnd));
  Stream.init(&*StreamFile);

  return std::error_code();
}

std::error_code BitcodeReader::InitLazyStream() {
  // Check and strip off the bitcode wrapper; BitstreamReader expects never to
  // see it.
  auto OwnedBytes = llvm::make_unique<StreamingMemoryObject>(
      std::move(LazyStreamer));
  StreamingMemoryObject &Bytes = *OwnedBytes;
  StreamFile = llvm::make_unique<BitstreamReader>(std::move(OwnedBytes));
  Stream.init(&*StreamFile);

  unsigned char buf[16];
  if (Bytes.readBytes(buf, 16, 0) != 16)
    return Error("Invalid bitcode signature");

  if (!isBitcode(buf, buf + 16))
    return Error("Invalid bitcode signature");

  if (isBitcodeWrapper(buf, buf + 4)) {
    const unsigned char *bitcodeStart = buf;
    const unsigned char *bitcodeEnd = buf + 16;
    SkipBitcodeWrapperHeader(bitcodeStart, bitcodeEnd, false);
    Bytes.dropLeadingBytes(bitcodeStart - buf);
    Bytes.setKnownObjectSize(bitcodeEnd - bitcodeStart);
  }
  return std::error_code();
}

namespace {
class BitcodeErrorCategoryType : public std::error_category {
  const char *name() const LLVM_NOEXCEPT override {
    return "llvm.bitcode";
  }
  std::string message(int IE) const override {
    BitcodeError E = static_cast<BitcodeError>(IE);
    switch (E) {
    case BitcodeError::InvalidBitcodeSignature:
      return "Invalid bitcode signature";
    case BitcodeError::CorruptedBitcode:
      return "Corrupted bitcode";
    }
    llvm_unreachable("Unknown error type!");
  }
};
}

static ManagedStatic<BitcodeErrorCategoryType> ErrorCategory;

const std::error_category &BitcodeReader::BitcodeErrorCategory() {
  return *ErrorCategory;
}

//===----------------------------------------------------------------------===//
// External interface
//===----------------------------------------------------------------------===//

/// getLazyBitcodeModule - lazy function-at-a-time loading from a file.
///
static llvm::ErrorOr<llvm::Module *>
getLazyBitcodeModuleImpl(std::unique_ptr<MemoryBuffer> &&Buffer,
                         LLVMContext &Context, bool WillMaterializeAll,
                         const DiagnosticHandlerFunction &DiagnosticHandler) {
  Module *M = new Module(Buffer->getBufferIdentifier(), Context);
  BitcodeReader *R =
      new BitcodeReader(Buffer.get(), Context, DiagnosticHandler);
  M->setMaterializer(R);

  auto cleanupOnError = [&](std::error_code EC) {
    R->releaseBuffer(); // Never take ownership on error.
    delete M;  // Also deletes R.
    return EC;
  };

  if (std::error_code EC = R->ParseBitcodeInto(M))
    return cleanupOnError(EC);

  Buffer.release(); // The BitcodeReader owns it now.
  return M;
}

llvm::ErrorOr<Module *>
llvm_3_0::getLazyBitcodeModule(std::unique_ptr<MemoryBuffer> &&Buffer,
                           LLVMContext &Context,
                           const DiagnosticHandlerFunction &DiagnosticHandler) {
  return getLazyBitcodeModuleImpl(std::move(Buffer), Context, false,
                                  DiagnosticHandler);
}

/// ParseBitcodeFile - Read the specified bitcode file, returning the module.
/// If an error occurs, return null and fill in *ErrMsg if non-null.
llvm::ErrorOr<llvm::Module *>
llvm_3_0::parseBitcodeFile(MemoryBufferRef Buffer, LLVMContext &Context,
                       const DiagnosticHandlerFunction &DiagnosticHandler) {
  std::unique_ptr<MemoryBuffer> Buf = MemoryBuffer::getMemBuffer(Buffer, false);
  ErrorOr<Module *> ModuleOrErr = getLazyBitcodeModuleImpl(
      std::move(Buf), Context, true, DiagnosticHandler);
  if (!ModuleOrErr)
    return ModuleOrErr;
  Module *M = ModuleOrErr.get();
  // Read in the entire module, and destroy the BitcodeReader.
  if (std::error_code EC = M->materializeAll()) {
    delete M;
    return EC;
  }

  return M;
}

std::string
llvm_3_0::getBitcodeTargetTriple(MemoryBufferRef Buffer, LLVMContext &Context,
                             DiagnosticHandlerFunction DiagnosticHandler) {
  std::unique_ptr<MemoryBuffer> Buf = MemoryBuffer::getMemBuffer(Buffer, false);
  auto R = llvm::make_unique<BitcodeReader>(Buf.release(), Context,
                                            DiagnosticHandler);
  ErrorOr<std::string> Triple = R->parseTriple();
  if (Triple.getError())
    return "";
  return Triple.get();
}
