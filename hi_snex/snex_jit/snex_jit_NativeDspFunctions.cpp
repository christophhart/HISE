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
	auto floatType = TypeInfo(Types::ID::Float);
	auto float2 = new SpanType(floatType, 2);
	auto dynFloat = new DynType(floatType);
	ComplexType::Ptr blockType = new SpanType(TypeInfo(dynFloat), 2);
	ComplexType::Ptr frameType = new DynType(TypeInfo(float2));
	blockType->setAlias(NamespacedIdentifier("ChannelData"));
	frameType->setAlias(NamespacedIdentifier("FrameData"));

	compiler->namespaceHandler.registerComplexTypeOrReturnExisting(blockType);
	compiler->namespaceHandler.registerComplexTypeOrReturnExisting(frameType);

	{
		auto il = new FunctionData();
		il->id = getClassName().getChildId("interleave");
		il->returnType = TypeInfo(Types::ID::Dynamic);
		il->addArgs("input", TypeInfo(Types::ID::Dynamic));

		il->inliner = new Inliner(il->id, [frameType, blockType, compiler](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			auto& cc = d->gen.cc;

			FunctionData iData;

			iData.returnType = TypeInfo(Types::ID::Void);
			iData.addArgs("ptr", TypeInfo(Types::ID::Pointer, true, true));
			iData.addArgs("numFrames", TypeInfo(Types::ID::Integer));
			iData.addArgs("numChannels", TypeInfo(Types::ID::Integer));

			iData.function = Types::Interleaver::interleaveRaw;

			bool isFrame = d->args[0]->getTypeInfo().getComplexType() == frameType;
			bool isBlock = d->args[0]->getTypeInfo().getComplexType() == blockType;

			jassert(isFrame || isBlock);

			auto dataRegister = compiler->registerPool.getNextFreeRegister(d->target->getScope(), TypeInfo(Types::ID::Pointer, true));
			auto frameRegister = compiler->registerPool.getNextFreeRegister(d->target->getScope(), TypeInfo(Types::ID::Integer));
			auto channelRegister = compiler->registerPool.getNextFreeRegister(d->target->getScope(), TypeInfo(Types::ID::Integer));

			auto ptrReg = d->args[0];

			cc.setInlineComment("interleave call");

			dataRegister->createRegister(cc);
			channelRegister->createRegister(cc);
			cc.mov(INT_REG_W(channelRegister), 2);
			frameRegister->createRegister(cc);

			if (isFrame)
			{
				X86Mem sizePtr;

				if (ptrReg->isMemoryLocation())
				sizePtr = ptrReg->getAsMemoryLocation();
				else
				sizePtr = x86::ptr(PTR_REG_R(ptrReg));

				sizePtr = sizePtr.cloneAdjustedAndResized(4, 4);

				auto dataPtr = sizePtr.cloneAdjustedAndResized(4, 8);
				cc.mov(INT_REG_W(frameRegister), sizePtr);
				cc.mov(PTR_REG_W(dataRegister), dataPtr);
			}
			else
			{
				X86Mem sizePtr;

				if (ptrReg->isMemoryLocation())
					sizePtr = ptrReg->getAsMemoryLocation();
				else
					sizePtr = x86::ptr(PTR_REG_R(ptrReg));

				sizePtr = sizePtr.cloneAdjustedAndResized(4, 4);
				cc.mov(INT_REG_W(frameRegister), sizePtr);
				auto dataPtr = sizePtr.cloneAdjustedAndResized(4, 8);
				cc.mov(PTR_REG_W(dataRegister), dataPtr);

			}

			AssemblyRegister::List iArgs;

			iArgs.add(dataRegister);

			if (isFrame)
			{
				iArgs.add(channelRegister);
				iArgs.add(frameRegister);
			}
			else
			{
				iArgs.add(frameRegister);
				iArgs.add(channelRegister);
			}



			d->gen.emitFunctionCall(d->target, iData, nullptr, iArgs);

			if (isFrame)
			{
				d->target->createRegister(cc);
				cc.mov(PTR_REG_W(d->target), PTR_REG_R(dataRegister));
			}
			else
			{
				auto s = cc.newStack(frameType->getRequiredByteSize(), frameType->getRequiredAlignment());

				cc.mov(s.cloneAdjustedAndResized(0, 4), 0);
				cc.mov(s.cloneAdjustedAndResized(4, 4), INT_REG_R(frameRegister));
				cc.mov(s.cloneAdjustedAndResized(8, 8), PTR_REG_R(dataRegister));

				d->target->setCustomMemoryLocation(s, false);
			}

			dataRegister->flagForReuse();
			channelRegister->flagForReuse();
			frameRegister->flagForReuse();

			return Result::ok();
		}, {});

		il->inliner->returnTypeFunction = [blockType, frameType](InlineData* d)
		{
			auto rd = dynamic_cast<ReturnTypeInlineData*>(d);

			

			auto inType = rd->object->getSubExpr(0)->getTypeInfo();

			if (inType.getComplexType() == blockType)
			{
				rd->f.returnType = TypeInfo(frameType, false);
				rd->f.args.getReference(0).typeInfo = TypeInfo(blockType, false, true);
				return Result::ok();
			}
			else if (inType.getComplexType() == frameType)
			{
				rd->f.returnType = TypeInfo(blockType, false);
				rd->f.args.getReference(0).typeInfo = TypeInfo(frameType, false, true);
				return Result::ok();
			}

			return Result::fail("can't deduce correct type for interleave call");
		};


		compiler->namespaceHandler.addSymbol(il->id, il->returnType, NamespaceHandler::Function);
		addFunction(il);
	}

	{
		auto sliceFunction = new FunctionData();

		sliceFunction->id = getClassName().getChildId("slice");
		sliceFunction->returnType = TypeInfo(Types::ID::Dynamic);
		sliceFunction->addArgs("obj", TypeInfo(Types::ID::Dynamic, false, true));
		sliceFunction->addArgs("start", TypeInfo(Types::ID::Integer));
		sliceFunction->addArgs("size", TypeInfo(Types::ID::Integer));

		sliceFunction->inliner = Inliner::createAsmInliner(sliceFunction->id, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			auto& cc = d->gen.cc;
			auto& dst = d->target;
			auto& src = d->args[0];
			auto& start = d->args[1];
			auto& size = d->args[2];

			jassert(dst->getTypeInfo().getTypedIfComplexType<DynType>() != nullptr);
			jassert(src->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>() != nullptr);
			jassert(start->getType() == Types::ID::Integer);
			jassert(size->getType() == Types::ID::Integer);

			auto startReg = cc.newGpd();

			if (start->hasCustomMemoryLocation())
				cc.mov(startReg, start->getAsMemoryLocation());
			else if (start->isMemoryLocation())
				cc.mov(startReg, start->getImmediateIntValue());
			else
				cc.mov(startReg, INT_REG_R(start));

			auto sizeReg = cc.newGpd();

			if (size->hasCustomMemoryLocation())
				cc.mov(sizeReg, size->getAsMemoryLocation());
			else if (start->isMemoryLocation())
				cc.mov(sizeReg, size->getImmediateIntValue());
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

			auto sourceType = rt->object->getSubExpr(0)->getTypeInfo();
			TypeInfo elementType;
			

			if (auto st = sourceType.getTypedIfComplexType<ArrayTypeBase>())
				elementType = st->getElementType();
			else
				return Result::fail("Can't slice type " + sourceType.toString());

			if (rt->templateParameters.isEmpty())
			{
				auto sliceType = new DynType(elementType);
				rt->f.returnType = TypeInfo(sliceType, false, false);
			}
			else
			{
				jassertfalse;
			}

			return Result::ok();
		};

		compiler->namespaceHandler.addSymbol(sliceFunction->id, sliceFunction->returnType, NamespaceHandler::Function);
		addFunction(sliceFunction);
	}

	
}

}
}