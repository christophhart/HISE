
#include "JitCompiler.h"



HiseJITCompiler::HiseJITCompiler(const String& codeToCompile, bool useCppMode)
{
	pimpl = new Pimpl(codeToCompile, useCppMode);
}

HiseJITCompiler::~HiseJITCompiler()
{
	pimpl = nullptr;
}


HiseJITScope* HiseJITCompiler::compileAndReturnScope() const
{
	return pimpl->compileAndReturnScope();
}

bool HiseJITCompiler::wasCompiledOK() const
{
	return pimpl->wasCompiledOK();
}

String HiseJITCompiler::getErrorMessage() const
{
	return pimpl->getErrorMessage();
}

String HiseJITCompiler::getCode(bool getPreprocessedCode) const
{
	return pimpl->getCode(getPreprocessedCode);
}

HiseJITScope::HiseJITScope()
{
	pimpl = new Pimpl();
}

HiseJITScope::~HiseJITScope()
{
	pimpl = nullptr;
}