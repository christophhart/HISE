
#ifndef HI_NATIVEJIT_INCLUDED
#define HI_NATIVEJIT_INCLUDED



#include "AppConfig.h"


#include "../JUCE/modules/juce_audio_basics/juce_audio_basics.h"


#ifdef _WIN64

#define NATIVEJIT_PLATFORM_WINDOWS 1

#define NATIVE_JIT_64_BIT 1
#define NATIVE_JIT_32_BIT 0
#else
#ifdef _WIN32

#define NATIVEJIT_PLATFORM_WINDOWS 1

#define NATIVE_JIT_64_BIT 0
#define NATIVE_JIT_32_BIT 1
#endif

#ifdef __LP64__


#define NATIVE_JIT_64_BIT 1
#define NATIVE_JIT_32_BIT 0
#else


#define NATIVE_JIT_64_BIT 0
#define NATIVE_JIT_32_BIT 1
#endif
#endif

#if NATIVE_JIT_64_BIT
#include "native_jit/inc/Temporary/Allocator.h"
#include "native_jit/inc/Temporary/AllocatorOperations.h"
#include "native_jit/inc/NativeJIT/AllocatorVector.h"
#include "native_jit/inc/Temporary/Assert.h"
#include "native_jit/inc/NativeJIT/BitOperations.h"
#include "native_jit/inc/NativeJIT/CodeGen/CallingConvention.h"
#include "native_jit/inc/NativeJIT/CodeGen/CodeBuffer.h"
#include "native_jit/inc/NativeJIT/CodeGen/ExecutionBuffer.h"
#include "native_jit/inc/NativeJIT/Function.h"
#include "native_jit/inc/NativeJIT/CodeGen/FunctionBuffer.h"
#include "native_jit/inc/NativeJIT/CodeGen/FunctionSpecification.h"
#include "native_jit/inc/Temporary/IAllocator.h"
#include "native_jit/inc/NativeJIT/CodeGen/JumpTable.h"
#include "native_jit/inc/NativeJIT/CodeGen/Register.h"
#include "native_jit/inc/Temporary/StlAllocator.h"
#include "native_jit/inc/NativeJIT/CodeGen/ValuePredicates.h"
#include "native_jit/inc/NativeJIT/CodeGen/X64CodeGenerator.h"
#endif


namespace juce
{
#include "../hi_core/hi_core/VariantBuffer.h"
};


#include <typeindex>

typedef std::type_index TypeInfo;


#include "hi_jit/hi_jit_compiler.h"



#endif   // HI_NATIVEJIT_INCLUDED



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

namespace juce
{
  //#include "../hi_core/hi_core/VariantBuffer.cpp"
};


#include "hi_jit/hi_jit_compiler.cpp"

#ifdef HI_CORE_INCLUDED
#error "Don't include hi_core"
#endif