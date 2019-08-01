/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

#include "JitCompiler.h"

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;

JITCompiler::JITCompiler(const String& codeToCompile)
{
	pimpl = new Pimpl(codeToCompile);
}

JITCompiler::~JITCompiler()
{
	pimpl = nullptr;
}

JITScope* JITCompiler::compileAndReturnScope(GlobalScope* globalMemoryPool) const
{
	return pimpl->compileAndReturnScope(globalMemoryPool);
}


hnode::jit::JITScope* JITCompiler::compileOneLineExpressionAndReturnScope(const Array<Types::ID>& typeList, GlobalScope* globalMemoryPool) const
{
	return pimpl->compileOneLineExpressionAndReturnScope(typeList, globalMemoryPool);
}

bool JITCompiler::wasCompiledOK() const
{
	return pimpl->wasCompiledOK();
}

String JITCompiler::getErrorMessage() const
{
	return pimpl->getErrorMessage();
}

String JITCompiler::getCode(bool getPreprocessedCode) const
{
	return pimpl->getCode(getPreprocessedCode);
}

JITScope::JITScope(GlobalScope* globalMemoryPool)
{
	pimpl = new Pimpl(globalMemoryPool);
}

JITScope::~JITScope()
{
	pimpl = nullptr;
}

} // end namespace jit
} // end namespace hnode

