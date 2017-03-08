
#include "hi_native_jit.h"

#if NATIVE_JIT_64_BIT
#include "native_jit/src/CodeGen/Allocator.cpp"
#include "native_jit/src/CodeGen/Assert.cpp"
#include "native_jit/src/NativeJIT/CallNode.cpp"
#include "native_jit/src/CodeGen/CodeBuffer.cpp"
#include "native_jit/src/CodeGen/ExecutionBuffer.cpp"
#include "native_jit/src/NativeJIT/ExpressionNodeFactory.cpp"
#include "native_jit/src/NativeJIT/ExpressionTree.cpp"
#include "native_jit/src/CodeGen/FunctionBuffer.cpp"
#include "native_jit/src/CodeGen/FunctionSpecification.cpp"
#include "native_jit/src/CodeGen/JumpTable.cpp"
#include "native_jit/src/NativeJIT/Node.cpp"
#include "native_jit/src/CodeGen/Register.cpp"
#include "native_jit/src/CodeGen/UnwindCode.cpp"
#include "native_jit/src/CodeGen/ValuePredicates.cpp"
#include "native_jit/src/CodeGen/X64CodeGenerator.cpp"
#endif

#include "hi_jit/hi_jit_compiler.cpp"
