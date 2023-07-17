

#if SNEX_ASMJIT_BACKEND
USE_ASMJIT_NAMESPACE;

namespace asmjit
{
using X86Gp = x86::Gp;
using X86Reg = x86::Reg;
using X86Mem = x86::Mem;
using X86Xmm = x86::Xmm;
using X86Gpq = x86::Gpq;
using X86Compiler = x86::Compiler;
using Runtime = JitRuntime;
using FuncSignatureX = FuncSignatureBuilder;
using CodeEmitter = x86::Emitter;
}

using AsmJitErrorHandler = asmjit::ErrorHandler;
using AsmJitX86Compiler = asmjit::X86Compiler;
using AsmJitStringLogger = asmjit::StringLogger;
using AsmJitRuntime = asmjit::JitRuntime;
using AsmJitLabel = asmjit::Label;

#else

struct AsmJitErrorHandler
{

};

struct AsmJitX86Compiler
{

};

struct AsmJitStringLogger
{

};

struct AsmJitRuntime
{

};

struct AsmJitLabel
{
	bool isValid() const { jassertfalse; return false; }
};


#endif

using String = juce::String;