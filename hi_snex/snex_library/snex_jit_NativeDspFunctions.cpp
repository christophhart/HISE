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


namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;

InbuiltFunctions::InbuiltFunctions(BaseCompiler* compiler) :
	FunctionClass(NamespacedIdentifier(Identifier()))
{


	auto frameType = compiler->namespaceHandler.getComplexType(NamespacedIdentifier("FrameData"));
	auto blockType = compiler->namespaceHandler.getComplexType(NamespacedIdentifier("ChannelData"));

	jassert(frameType != nullptr);
	jassert(blockType != nullptr);

	{
		
	}

#if SNEX_MIR_BACKEND
	{
		auto sliceFunction = new FunctionData();

		sliceFunction->id = getClassName().getChildId("slice");
		sliceFunction->returnType = TypeInfo(Types::ID::Dynamic);
		sliceFunction->addArgs("obj", TypeInfo(Types::ID::Dynamic, false, true));
		
		sliceFunction->addArgs("size", TypeInfo(Types::ID::Integer));
		sliceFunction->addArgs("offset", TypeInfo(Types::ID::Integer));
		sliceFunction->setDefaultParameter("size", VariableStorage(-1));
		sliceFunction->setDefaultParameter("offset", VariableStorage(0));

		sliceFunction->inliner = Inliner::createAsmInliner(sliceFunction->id, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			auto& cc = d->gen.cc;
			auto dst = d->target;
			auto src = d->args[0];
			auto start = d->args[2];
			auto size = d->args[1];

			jassert(dst->getTypeInfo().getTypedIfComplexType<DynType>() != nullptr);
			jassert(src->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>() != nullptr);
			jassert(start->getType() == Types::ID::Integer);
			jassert(size->getType() == Types::ID::Integer);

			auto startReg = cc.newGpd();

			if (start->hasCustomMemoryLocation())
				cc.mov(startReg, start->getAsMemoryLocation());
			else if (start->isMemoryLocation())
				cc.mov(startReg, (int64_t)start->getImmediateIntValue());
			else
				cc.mov(startReg, INT_REG_R(start));

			auto sizeReg = cc.newGpd();

			if (size->hasCustomMemoryLocation())
				cc.mov(sizeReg, size->getAsMemoryLocation());
			else if (start->isMemoryLocation())
				cc.mov(sizeReg, (int64_t)size->getImmediateIntValue());
			else
				cc.mov(sizeReg, INT_REG_R(size));

			auto maxReg = cc.newGpd();


			if (auto st = src->getTypeInfo().getTypedIfComplexType<SpanType>())
			{
				cc.mov(maxReg, st->getNumElements());
			}
			else
			{
				auto tempDataReg = cc.newGpq();

				if (src->isMemoryLocation())
					cc.lea(tempDataReg, src->getAsMemoryLocation());
				else
					cc.mov(tempDataReg, PTR_REG_R(src));

				cc.mov(maxReg, x86::ptr(tempDataReg).cloneAdjustedAndResized(4, 4));
			}


			{
				auto zero = cc.newGpd();
				auto valueReg = cc.newGpd();
				cc.xor_(zero, zero);
				cc.mov(valueReg, startReg);

				MathFunctions::Intrinsics::range(cc, startReg, valueReg, zero, maxReg);

				cc.sub(maxReg, startReg);
				cc.add(maxReg, 1);
				cc.mov(valueReg, sizeReg);

				MathFunctions::Intrinsics::range(cc, sizeReg, valueReg, zero, maxReg);
			}

			

			auto dataReg = cc.newGpq();

			if (src->isMemoryLocation())
				cc.lea(dataReg, src->getAsMemoryLocation());
			else
				cc.mov(dataReg, PTR_REG_R(src));

			if (auto dt = src->getTypeInfo().getTypedIfComplexType<DynType>())
				cc.mov(dataReg, x86::ptr(dataReg).cloneAdjustedAndResized(8, 8));

			cc.lea(dataReg, x86::ptr(dataReg, startReg.r64(), 2));// startReg.r64());

			auto stackMem = cc.newStack(16, 0);
			cc.mov(stackMem.cloneResized(4), 0);

			cc.mov(stackMem.cloneAdjustedAndResized(4, 4), sizeReg);
			cc.mov(stackMem.cloneAdjustedAndResized(8, 8), dataReg);

			dst->setCustomMemoryLocation(stackMem, false);

			return Result::ok();
		});

		sliceFunction->inliner->returnTypeFunction = [](InlineData* b)
		{
			auto rt = dynamic_cast<ReturnTypeInlineData*>(b);

			if (auto so = rt->object->getSubExpr(0))
			{
				auto sourceType = so->getTypeInfo();
				TypeInfo elementType;

				if (auto st = sourceType.getTypedIfComplexType<ArrayTypeBase>())
					elementType = st->getElementType();
				else
					return Result::fail("Can't slice type " + sourceType.toString());

				if (rt->templateParameters.isEmpty())
				{
					auto sliceType = new DynType(elementType);
					rt->f.returnType = TypeInfo(sliceType);
				}
				else
				{
					jassertfalse;
				}
			}
			else
			{
				return Result::fail("Can't deduce array type");
			}

			

			return Result::ok();
		};

		compiler->namespaceHandler.addSymbol(sliceFunction->id, sliceFunction->returnType, NamespaceHandler::Function, {});
		addFunction(sliceFunction);
	}
#endif

	
}

}
}
