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

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;

template <typename R>
BaseNodePtr hnode::jit::FunctionParser<R>::parseParameterReferenceTyped(const Identifier& id)
{
	const int pIndex = getParameterIndex(id);
	const String pTypeName = Types::Helpers::getCppTypeName(info.args[pIndex]);

	const TypeInfo pType = JITTypeHelpers::getTypeForToken(pTypeName.getCharPointer());

	ScopedBaseNodePointer pNode;// = AsmJitHelpers::Parameter(*asmCompiler, pIndex, pType);;
	pNode->setId(id.toString());

	return pNode.release();
}

template <typename R>
void hnode::jit::FunctionParser<R>::parseReturn()
{

	ScopedBaseNodePointer rt;

	if (matchIf(JitTokens::semicolon))
	{
		addVoidReturnStatement();
		voidReturnWasFound = true;
	}
	else
	{
		rt = parseTypedExpression<R>();
		match(JitTokens::semicolon);

		storeGlobalsBeforeReturn();

		AsmJitHelpers::Return(*asmCompiler, getTypedNode<R>(rt));
	}
}

template <typename R>
void hnode::jit::FunctionParser<R>::storeGlobalsBeforeReturn()
{
	for (int i = 0; i < globalNodes.size(); i++)
	{
		if (globalNodes[i]->isChangedGlobal())
		{

			jassertfalse;
#if 0
			void* data = scope->getGlobal(globalNodes[i]->getId())->getDataPointer();

			TypeInfo thisType = globalNodes[i]->getType();

			if (JITTypeHelpers::matchesType<float>(thisType)) AsmJitHelpers::StoreGlobal<float>(*asmCompiler, data, globalNodes[i]);
			if (JITTypeHelpers::matchesType<double>(thisType)) AsmJitHelpers::StoreGlobal<double>(*asmCompiler, data, globalNodes[i]);
			if (JITTypeHelpers::matchesType<int>(thisType)) AsmJitHelpers::StoreGlobal<int>(*asmCompiler, data, globalNodes[i]);
			if (JITTypeHelpers::matchesType<BooleanType>(thisType)) AsmJitHelpers::StoreGlobal<BooleanType>(*asmCompiler, data, globalNodes[i]);
#endif
		}
	}
}

template <typename R>
hnode::jit::FunctionParser<R>::~FunctionParser()
{

}

template <typename R>
void hnode::jit::FunctionParser<R>::addVoidReturnStatement()
{
	if (!JITTypeHelpers::is<R, void>() || voidReturnWasFound)
		return;

	storeGlobalsBeforeReturn();

	asmCompiler->ret();
}

} // end namespace jit
} // end namespace hnode

