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
namespace Types {
using namespace juce;
using namespace asmjit;




#define REGISTER_CPP_CLASS(compiler, className) c.registerExternalComplexType(className::createComplexType(c, #className));

#define REGISTER_ORIGINAL_CPP_CLASS(compiler, className, originalClass) c.registerExternalComplexType(className::createComplexType(c, #originalClass));


template <typename T>
jit::ComplexType::Ptr _ramp<T>::createComplexType(Compiler& c, const Identifier& id)
{
	Type s;

	auto obj = new StructType(NamespacedIdentifier(id));

	ADD_SNEX_PRIVATE_MEMBER(obj, s, value);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, targetValue);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, delta);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, stepDivider);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, numSteps);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, stepsToDo);

	ADD_SNEX_STRUCT_METHOD(obj, Type, reset);

	ADD_SNEX_STRUCT_METHOD(obj, Type, set);
	SET_SNEX_PARAMETER_IDS(obj, "newValue");

	ADD_SNEX_STRUCT_METHOD(obj, Type, advance);
	ADD_SNEX_STRUCT_METHOD(obj, Type, get);
	ADD_SNEX_STRUCT_METHOD(obj, Type, prepare);
	SET_SNEX_PARAMETER_IDS(obj, "sampleRate", "fadeTimeMilliSeconds");

	FunctionClass::Ptr fc = obj->getFunctionClass();

	ADD_INLINER(set, {
		SETUP_INLINER(T);

		auto skipSmooth = cc.newLabel();
		auto end = cc.newLabel();
		auto numSteps = cc.newGpd();
		d->args[0]->loadMemoryIntoRegister(cc);
		auto newTargetValue = FP_REG_R(d->args[0]);
		x86::Xmm dl;

		IF_(float)  dl = cc.newXmmSs();
		IF_(double) dl = cc.newXmmSd();

		cc.setInlineComment("Inlined sfloat::set");
		cc.mov(numSteps, MEMBER_PTR(numSteps));
		cc.test(numSteps, numSteps);
		cc.je(skipSmooth);

		// if(numSteps != 0)
		{
			IF_(float)
			{
				cc.movss(dl, newTargetValue);
				cc.subss(dl, MEMBER_PTR(value));
				cc.mulss(dl, MEMBER_PTR(stepDivider));
				cc.movss(MEMBER_PTR(delta), dl);
				cc.movss(MEMBER_PTR(targetValue), newTargetValue);
			}
			IF_(double)
			{
				cc.movsd(dl, newTargetValue);
				cc.subsd(dl, MEMBER_PTR(value));
				cc.mulsd(dl, MEMBER_PTR(stepDivider));
				cc.movsd(MEMBER_PTR(delta), dl);
				cc.movsd(MEMBER_PTR(targetValue), newTargetValue);
			}

			cc.mov(MEMBER_PTR(stepsToDo), numSteps);
			cc.jmp(end);
		}
		// else
		{
			cc.bind(skipSmooth);

			IF_(float) cc.movss(MEMBER_PTR(value), newTargetValue);
			IF_(double) cc.movsd(MEMBER_PTR(value), newTargetValue);

			cc.mov(MEMBER_PTR(stepsToDo), 0);
		}

		cc.bind(end);

		return Result::ok();
		});

	ADD_INLINER(get,
		{
			SETUP_INLINER(T);
			d->target->createRegister(cc);
			auto ret = FP_REG_W(d->target);

			cc.setInlineComment("inline get()");
			IF_(float) cc.movss(ret, MEMBER_PTR(value));
			IF_(double) cc.movsd(ret, MEMBER_PTR(value));

			return Result::ok();
		});

	ADD_INLINER(advance,
		{
			/*
			T advance()
			{
				auto v = value;

				if (stepsToDo <= 0)
					return v;

				value += delta;
				stepsToDo--;

				return v;
			}
			*/

			SETUP_INLINER(T);

			d->target->createRegister(cc);
			auto ret = FP_REG_W(d->target);
			auto end = cc.newLabel();
			auto stepsToDo = cc.newGpd();

			cc.setInlineComment("inline advance()");
			IF_(float) cc.movss(ret, MEMBER_PTR(value));
			IF_(double) cc.movsd(ret, MEMBER_PTR(value));

			cc.mov(stepsToDo, MEMBER_PTR(stepsToDo));
			cc.cmp(stepsToDo, 0);
			cc.jle(end);

			// if (stepsToDo > 0
			{
				IF_(float)
				{
					auto tmp = cc.newXmmSs();
					cc.movss(tmp, ret);
					cc.addss(tmp, MEMBER_PTR(delta));
					cc.movss(MEMBER_PTR(value), tmp);
				}
				IF_(double)
				{
					auto tmp = cc.newXmmSd();
					cc.movsd(tmp, ret);
					cc.addsd(tmp, MEMBER_PTR(delta));
					cc.movsd(MEMBER_PTR(value), tmp);
				}

				cc.dec(stepsToDo);
				cc.mov(MEMBER_PTR(stepsToDo), stepsToDo);
			}

			cc.bind(end);
			return Result::ok();
		});

	return obj;
}


snex::jit::ComplexType::Ptr EventWrapper::createComplexType(Compiler& c, const Identifier& id)
{
	auto obj = new StructType(NamespacedIdentifier(id));

	HiseEvent e;
	int* ptr = reinterpret_cast<int*>(&e);

	obj->addExternalMember("dword1", e, ptr[0], NamespaceHandler::Visibility::Private);
	obj->addExternalMember("dword2", e, ptr[1], NamespaceHandler::Visibility::Private);
	obj->addExternalMember("dword3", e, ptr[2], NamespaceHandler::Visibility::Private);
	obj->addExternalMember("dword4", e, ptr[3], NamespaceHandler::Visibility::Private);

	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getNoteNumber);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getVelocity);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getChannel);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, setChannel);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, setVelocity);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, setNoteNumber);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getTimeStamp);

	obj->setCustomDumpFunction([](void* ptr)
		{
			auto e = static_cast<HiseEvent*>(ptr);

			String s;

			s << "| HiseEvent { " << e->toDebugString() + " }\n";
			return s;
		});

	FunctionClass::Ptr fc = obj->getFunctionClass();

	ADD_INLINER(getChannel,
		{
			SETUP_INLINER(int);
			d->target->createRegister(cc);
			auto n = base.cloneAdjustedAndResized(0x01, 1);
			cc.movsx(INT_REG_W(d->target), n);
			return Result::ok();
		});

	ADD_INLINER(getNoteNumber,
		{
			SETUP_INLINER(int);
			d->target->createRegister(cc);
			auto n = base.cloneAdjustedAndResized(0x02, 1);
			cc.movsx(INT_REG_W(d->target), n);
			return Result::ok();
		});

	ADD_INLINER(getVelocity,
		{
			SETUP_INLINER(int);
			d->target->createRegister(cc);
			auto n = base.cloneAdjustedAndResized(0x03, 1);
			cc.movsx(INT_REG_W(d->target), n);
			return Result::ok();
		});

	ADD_INLINER(setChannel,
		{
			SETUP_INLINER(int);
			auto n = base.cloneAdjustedAndResized(0x01, 1);

			auto v = d->args[0];
			if (IS_MEM(v)) cc.mov(n, INT_IMM(v));
			else cc.mov(n, INT_REG_R(v));

			return Result::ok();
		});

	ADD_INLINER(setNoteNumber,
		{
			SETUP_INLINER(int);
			auto n = base.cloneAdjustedAndResized(0x02, 1);

			auto v = d->args[0];
			if (IS_MEM(v)) cc.mov(n, INT_IMM(v));
			else cc.mov(n, INT_REG_R(v));

			return Result::ok();
		});

	ADD_INLINER(setVelocity,
		{
			SETUP_INLINER(int);
			auto n = base.cloneAdjustedAndResized(0x03, 1);

			auto v = d->args[0];
			if (IS_MEM(v)) cc.mov(n, INT_IMM(v));
			else cc.mov(n, INT_REG_R(v));

			return Result::ok();
		});

	return obj;
}

snex::jit::ComplexType::Ptr OscProcessDataJit::createComplexType(Compiler& c, const Identifier& id)
{
	OscProcessData d;

	auto st = CREATE_SNEX_STRUCT(OscProcessData);
	auto blockType = c.getNamespaceHandler().getAliasType(NamespacedIdentifier("block")).getComplexType();

	ADD_SNEX_STRUCT_COMPLEX(st, blockType, d, data);

	ADD_SNEX_STRUCT_MEMBER(st, d, uptime);
	ADD_SNEX_STRUCT_MEMBER(st, d, delta);
	ADD_SNEX_STRUCT_MEMBER(st, d, voiceIndex);

	return st;
}

void InbuiltTypeLibraryBuilder::createProcessData(const TypeInfo& eventType)
{
	NamespacedIdentifier pId("ProcessData");

	TemplateObject ptc({ pId, {} });
	ptc.argList.add(TemplateParameter(pId.getChildId("NumChannels"), 0, false));
	ptc.description = "An object containing the data for the multichannel processing";

	ptc.makeClassType = [eventType](const TemplateObject::ConstructData& c)
	{
		ComplexType::Ptr p;

		if (!c.expectTemplateParameterAmount(1))
			return p;

		if (!c.expectNotIntegerValue(0, 0))
			return p;

		auto blockType = TypeInfo(c.handler->getAliasType(NamespacedIdentifier("block")));

		NamespacedIdentifier pId("ProcessData");

		TemplateParameter::List l;
		l.add(c.tp[0]);
		l.getReference(0).argumentId = pId.getChildId("NumChannels");

		jassert(l.getReference(0).argumentId.isValid());

		auto pType = new StructType(pId, l);

		pType->addMember("data", TypeInfo(Types::ID::Pointer, true), "the pointer to the channel values");
		pType->addMember("events", TypeInfo(Types::ID::Pointer, true), "a list containing all events for the current block");
		pType->addMember("numSamples", TypeInfo(Types::ID::Integer), "the number of samples to process");
		pType->addMember("numEvents", TypeInfo(Types::ID::Integer), "the number of events in this block");
		pType->addMember("numChannels", TypeInfo(Types::ID::Integer), "the number of channels to process");
		pType->addMember("shouldReset", TypeInfo(Types::ID::Integer), "flag that checks if the processing should be reseted");

		auto numChannels = c.tp[0].constant;

		pType->setDefaultValue("data", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("events", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numSamples", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numEvents", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numChannels", InitialiserList::makeSingleList(numChannels));
		pType->setDefaultValue("shouldReset", InitialiserList::makeSingleList(0));

		pType->setCustomDumpFunction([numChannels](void* obj)
			{
				String s;

				auto d = static_cast<ProcessDataDyn*>(obj);

				s << "| ProcessData<" << numChannels << ">\t{ ";
				s << "numSamples: " << d->getNumSamples() << " }\n";


				for (int i = 0; i < numChannels; i++)
				{
					s << "|   Channel " << numChannels << "\t{ }\n";

					auto chPtr = d->getRawChannelPointers()[i];

					for (int j = 0; j < d->getNumSamples(); j++)
					{
						String t;
						t << "d[" << i << "][" << j << "]";

						Types::Helpers::dumpNativeData(s, 2, t, chPtr, chPtr + i, sizeof(float), Types::ID::Float);
					}
				}

				return s;
			});

		{
			FunctionData subscript;
			subscript.id = pId.getChildId(FunctionClass::getSpecialSymbol(pId, jit::FunctionClass::Subscript));
			subscript.returnType = blockType;
			subscript.addArgs("obj", TypeInfo(Types::ID::Pointer, true, true));
			subscript.addArgs("index", TypeInfo(Types::ID::Integer));
			subscript.inliner = Inliner::createAsmInliner(subscript.id, [pType](InlineData* b)
				{
					auto d = b->toAsmInlineData();

					auto& cc = d->gen.cc;

					auto dynObj = cc.newStack(16, 32);
					cc.mov(dynObj.cloneResized(4), 128);

					auto dataObject = d->args[0];
					dataObject->loadMemoryIntoRegister(cc);

					auto tmp = cc.newGpq();

					auto indexReg = d->args[1];
					cc.mov(tmp, x86::ptr(PTR_REG_R(dataObject)));

					if (indexReg->isMemoryLocation())
					{
						int value = indexReg->getImmediateIntValue();
						cc.mov(tmp, x86::ptr(tmp, value * sizeof(float*), 8));
					}
					else
					{
						cc.mov(tmp, x86::ptr(tmp, INT_REG_R(indexReg), 3, 8));
					}

					cc.mov(dynObj.cloneAdjustedAndResized(8, 8), tmp);

					int sizeOffset = pType->getMemberOffset("numSamples");

					auto size = x86::ptr(PTR_REG_R(dataObject)).cloneAdjustedAndResized(sizeOffset, sizeof(int));
					auto tmp2 = cc.newGpd();

					cc.mov(tmp2, size);
					cc.mov(dynObj.cloneAdjustedAndResized(4, 4), tmp2);

					d->target->setCustomMemoryLocation(dynObj, false);

					return Result::ok();
				});

			pType->addJitCompiledMemberFunction(subscript);
		}

		auto channelPtrType = new StructType(NamespacedIdentifier("ChannelPtr"));
		channelPtrType->addMember("ptr", TypeInfo(Types::ID::Pointer, true, false));
		channelPtrType->finaliseAlignment();

		ComplexType::Ptr owned = channelPtrType;


		TypeInfo channelType(c.handler->registerComplexTypeOrReturnExisting(channelPtrType));



		{
			FunctionData beginF, sizeFunction;
			beginF.id = pId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::BeginIterator));
			beginF.returnType = channelType;
			sizeFunction.id = pId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::SizeFunction));
			sizeFunction.returnType = TypeInfo(Types::ID::Integer);

			beginF.inliner = Inliner::createAsmInliner(beginF.id, [](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto& cc = d->gen.cc;

					d->object->loadMemoryIntoRegister(cc);
					d->target->createRegister(cc);

					cc.mov(PTR_REG_W(d->target), x86::qword_ptr(PTR_REG_R(d->object)));


					return Result::ok();
				});

			sizeFunction.inliner = Inliner::createAsmInliner(sizeFunction.id, [pType](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto& cc = d->gen.cc;
					jassert(d->object->isActive());
					d->target->createRegister(cc);

					if (!d->templateParameters.isEmpty())
					{
						cc.mov(INT_REG_W(d->target), d->templateParameters[0].constant);
					}
					else
					{
						int sizeOffset = pType->getMemberOffset("numChannels");
						auto size = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));
						cc.mov(INT_REG_W(d->target), size);

					}

					return Result::ok();
				});

			pType->addJitCompiledMemberFunction(beginF);
			pType->addJitCompiledMemberFunction(sizeFunction);
		}

		{
			FunctionData tcd;
			tcd.id = pId.getChildId("toChannelData");
			tcd.returnType = blockType;
			tcd.addArgs("channelPtr", channelType);

			tcd.inliner = Inliner::createAsmInliner(tcd.id, [pType](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto& cc = d->gen.cc;

					d->object->loadMemoryIntoRegister(cc);
					d->args[0]->loadMemoryIntoRegister(cc);

					auto dynObj = cc.newStack(16, 0);

					cc.mov(dynObj.cloneResized(4), 128);

					auto data = cc.newGpq();
					cc.mov(data, x86::qword_ptr(PTR_REG_R(d->args[0])));

					cc.mov(dynObj.cloneAdjustedAndResized(8, 8), data);
					int sizeOffset = pType->getMemberOffset("numSamples");
					auto size = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));

					auto tmp = cc.newGpd();

					cc.mov(tmp, size);
					cc.mov(dynObj.cloneAdjustedAndResized(4, 4), tmp);

					d->target->setCustomMemoryLocation(dynObj, false);

					return Result::ok();
				});

			pType->addJitCompiledMemberFunction(tcd);
		}

		{
			FunctionData ted;
			ted.id = pId.getChildId("toEventData");

			auto eventType = c.handler->getComplexType(NamespacedIdentifier("HiseEvent"));
			auto eventBufferType = new DynType(TypeInfo(eventType));
			ted.returnType = TypeInfo(c.handler->registerComplexTypeOrReturnExisting(eventBufferType));

			ted.inliner = Inliner::createAsmInliner(ted.id, [pType](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto& cc = d->gen.cc;

					d->object->loadMemoryIntoRegister(cc);

					auto dynObj = cc.newStack(16, 0);

					cc.mov(dynObj.cloneResized(4), 128);
					auto data = cc.newGpq();
					cc.mov(dynObj.cloneAdjustedAndResized(8, 8), data);

					int eventOffset = pType->getMemberOffset("events");
					auto eventDataPtr = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(eventOffset, sizeof(int));

					cc.mov(data, eventDataPtr);
					cc.mov(dynObj.cloneAdjustedAndResized(8, 8), data);

					int sizeOffset = pType->getMemberOffset("numEvents");
					auto size = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));

					auto tmp = cc.newGpd();

					cc.mov(tmp, size);
					cc.mov(dynObj.cloneAdjustedAndResized(4, 4), tmp);

					d->target->setCustomMemoryLocation(dynObj, false);

					return Result::ok();
				});

			pType->addJitCompiledMemberFunction(ted);
		}


		{
			FunctionData tfd;
			tfd.id = pId.getChildId("toFrameData");

			int numChannels = c.tp[0].constant;

			auto frameProcessor = c.handler->createTemplateInstantiation({ NamespacedIdentifier("FrameProcessor"), {} }, c.tp, *c.r);

			tfd.returnType = TypeInfo(frameProcessor);

			tfd.inliner = Inliner::createAsmInliner(tfd.id, [pType, frameProcessor, numChannels](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto& cc = d->gen.cc;

					auto size = frameProcessor->getRequiredByteSize();

					auto frameStackData = cc.newStack(size, 0);

					/*
					span<float*, NumChannels>& channels; // 8 byte
					int frameLimit = 0;					 // 4 byte
					int frameIndex = 0;				     // 4 byte
					FrameType frameData;				 // sizeof(FrameData)
					*/

					cc.mov(frameStackData.cloneResized(8), PTR_REG_R(d->object));


					int sizeOffset = pType->getMemberOffset("numSamples");

					auto channelsPtrReg = cc.newGpq();
					cc.mov(channelsPtrReg, x86::qword_ptr(PTR_REG_R(d->object)));

					auto frameSize = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));

					auto sizeReg = cc.newGpd();

					cc.mov(sizeReg, frameSize);
					cc.mov(frameStackData.cloneAdjustedAndResized(8, 4), sizeReg);
					cc.mov(frameStackData.cloneAdjustedAndResized(12, 4), 0);

					auto cReg = cc.newGpq();
					auto fReg = cc.newXmmSs();

					auto frameStart = frameStackData.cloneAdjustedAndResized(16, 4);

					for (int i = 0; i < numChannels; i++)
					{
						cc.mov(cReg, x86::ptr(channelsPtrReg).cloneAdjustedAndResized(i * 8, 4));
						cc.movss(fReg, x86::ptr(cReg));
						cc.movss(frameStart.cloneAdjusted(i * 4), fReg);
					}

					d->target->setCustomMemoryLocation(frameStackData, false);

					return Result::ok();
				});

			pType->addJitCompiledMemberFunction(tfd);
		}

		{
			FunctionData numChannelF;
			numChannelF.id = pType->id.getChildId("getNumChannels");
			numChannelF.returnType = TypeInfo(Types::ID::Integer);

			int numChannels = c.tp[0].constant;

			numChannelF.inliner = Inliner::createHighLevelInliner(numChannelF.id, [numChannels](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();

					d->target = new Operations::Immediate(d->location, numChannels);

					return Result::ok();
				});

			pType->addJitCompiledMemberFunction(numChannelF);
		}

		{
			FunctionData numSamplesF;
			numSamplesF.id = pType->id.getChildId("getNumSamples");
			numSamplesF.returnType = TypeInfo(Types::ID::Integer);


			numSamplesF.inliner = Inliner::createAsmInliner(numSamplesF.id, [](InlineData* b)
				{


					jassertfalse;

					return Result::ok();
				});

			pType->addJitCompiledMemberFunction(numSamplesF);
		}


		pType->finaliseExternalDefinition();

		p = pType;

		return p;
	};

	c.addTemplateClass(ptc);
}

void InbuiltTypeLibraryBuilder::createFrameProcessor()
{
	NamespacedIdentifier pId("FrameProcessor");

	TemplateObject ftc({ pId,{} });
	ftc.argList.add(TemplateParameter(pId.getChildId("NumChannels"), 0, false));

	ftc.makeClassType = [](const TemplateObject::ConstructData& c)
	{
		ComplexType::Ptr p;

		if (!c.expectTemplateParameterAmount(1))
			return p;

		if (!c.expectNotIntegerValue(0, 0))
			return p;

		NamespacedIdentifier fId("FrameProcessor");

		TemplateParameter::List l;
		l.add(c.tp[0]);
		l.getReference(0).argumentId = fId.getChildId("NumChannels");

		jassert(l.getReference(0).argumentId.isValid());

		auto fType = new StructType(fId, l);

		int numChannels = c.tp[0].constant;

		/*
		span<float*, NumChannels>& channels; // 8 byte
		int frameLimit = 0;					 // 4 byte
		int frameIndex = 0;				     // 4 byte
		FrameType frameData;				 // sizeof(FrameData)
		*/

		fType->addMember("channels", TypeInfo(Types::ID::Pointer, true));
		fType->addMember("frameLimit", TypeInfo(Types::ID::Integer));
		fType->addMember("frameIndex", TypeInfo(Types::ID::Integer));

		auto frameDataType_ = new SpanType(TypeInfo(Types::ID::Float), numChannels);
		auto frameType = TypeInfo(c.handler->registerComplexTypeOrReturnExisting(frameDataType_));
		fType->addMember("frameData", frameType);

		{
			FunctionData beginF;
			beginF.id = fId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::BeginIterator));
			beginF.returnType = TypeInfo(Types::ID::Float, false, true);
			beginF.inliner = Inliner::createAsmInliner(beginF.id, [fType](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto& cc = d->gen.cc;
					auto frameDataOffset = fType->getMemberOffset("frameData");

					X86Mem framePtr;

					if (d->object->isActive())
						framePtr = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(frameDataOffset, 8);
					else
						framePtr = d->object->getAsMemoryLocation().cloneAdjustedAndResized(frameDataOffset, 8);

					cc.lea(PTR_REG_W(d->target), framePtr);
					return Result::ok();
				});

			fType->addJitCompiledMemberFunction(beginF);

			FunctionData sizeF;
			sizeF.id = fId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::SizeFunction));
			sizeF.returnType = TypeInfo(Types::ID::Integer);
			sizeF.inliner = Inliner::createAsmInliner(sizeF.id, [numChannels](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					d->target->setImmediateValue(numChannels);
					return Result::ok();
				});

			fType->addJitCompiledMemberFunction(sizeF);
		}
		{
			FunctionData nextF;
			nextF.id = fId.getChildId("next");

			nextF.returnType = TypeInfo(Types::ID::Integer);
			nextF.inliner = Inliner::createAsmInliner(nextF.id, [fType, numChannels](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto& cc = d->gen.cc;

					d->target->createRegister(cc);

					auto returnValue = INT_REG_W(d->target);

					X86Mem frameStack;

					if (d->object->isActive())
						frameStack = x86::ptr(PTR_REG_W(d->object));
					else
						frameStack = d->object->getAsMemoryLocation();

					auto channelPtrs = frameStack.cloneResized(8);
					auto frameLimit = frameStack.cloneAdjustedAndResized(8, 4);
					auto frameIndex = frameStack.cloneAdjustedAndResized(12, 4);
					auto frameData = frameStack.cloneAdjustedAndResized(16, 4);


					auto exit = cc.newLabel();
					auto writeLastFrame = cc.newLabel();

					cc.mov(returnValue, frameIndex);

					// if (fp->frameIndex == 0)
					cc.cmp(returnValue, 0);
					cc.jne(writeLastFrame);

					//++fp->frameIndex;
					cc.inc(frameIndex);

					//return fp->frameLimit;
					cc.mov(returnValue, frameLimit);
					cc.jmp(exit);

					cc.setInlineComment("Write the last frame");
					cc.bind(writeLastFrame);


					auto channelData = cc.newGpq();
					cc.mov(channelData, channelPtrs);
					cc.mov(channelData, x86::qword_ptr(channelData));
					auto tmp = cc.newXmmSs();
					auto cReg = cc.newGpq();

					for (int i = 0; i < numChannels; i++)
					{
						//channels[i][frameIndex - 1] = frameData[i];
						auto cPtr = x86::ptr(channelData).cloneAdjusted(i * 8);
						cc.mov(cReg, cPtr);

						auto src = frameData.cloneAdjusted(i * 4);
						auto dst = x86::ptr(cReg, returnValue, 2, -4, 4);

						cc.movss(tmp, src);
						cc.movss(dst, tmp);
					}


					// if(fp->frameIndex < fp->frameLimit)
					cc.cmp(returnValue, frameLimit);
					auto finished = cc.newLabel();
					cc.jnb(finished);
					cc.setInlineComment("Load the next frame");

					for (int i = 0; i < numChannels; i++)
					{
						//frameData[i] = channels[i][frameIndex];

						auto cPtr = x86::ptr(channelData).cloneAdjusted(i * 8);
						cc.mov(cReg, cPtr);

						auto src = x86::ptr(cReg, returnValue, 2, 0, 4);
						auto dst = frameData.cloneAdjusted(i * 4);

						cc.movss(tmp, src);
						cc.movss(dst, tmp);
					}

					//	++fp->frameIndex;
					cc.inc(frameIndex);

					//	return 1;
					cc.mov(returnValue, 1);
					cc.jmp(exit);
					cc.bind(finished);

					// return 0;
					cc.mov(returnValue, 0);
					cc.bind(exit);


					return Result::ok();
				});

			fType->addJitCompiledMemberFunction(nextF);

			{
				FunctionData subscript;
				subscript.id = fId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::Subscript));
				subscript.returnType = TypeInfo(Types::ID::Float, false, true);
				subscript.addArgs("obj", TypeInfo(Types::ID::Pointer, true)); // break ref-count cycle...
				subscript.addArgs("index", TypeInfo(Types::ID::Integer));
				subscript.inliner = Inliner::createAsmInliner(subscript.id, [](InlineData* b)
					{
						auto d = b->toAsmInlineData();
						auto& cc = d->gen.cc;

						X86Mem frameStack;

						if (d->args[0]->isActive())
							frameStack = x86::ptr(PTR_REG_W(d->args[0]));
						else
							frameStack = d->args[0]->getAsMemoryLocation();

						auto frameData = frameStack.cloneAdjustedAndResized(16, 4);

						if (d->args[1]->isMemoryLocation())
						{
							auto offset = INT_IMM(d->args[1]);
							d->target->setCustomMemoryLocation(frameData.cloneAdjusted(offset * 4), d->args[0]->isGlobalMemory());
						}
						else
						{
							return Result::fail("Can't use non-constant index for frame []-operator");
						}

						return Result::ok();
					});

				fType->addJitCompiledMemberFunction(subscript);
			}

			{
				FunctionData ts;
				ts.id = fId.getChildId("toSpan");
				ts.returnType = frameType.withModifiers(false, true, false);
				ts.inliner = Inliner::createHighLevelInliner({}, [](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();

					auto fType = d->object->getTypeInfo().getTypedComplexType<StructType>();

					auto mt = fType->getMemberTypeInfo("frameData");
					auto mo = fType->getMemberOffset("frameData");

					d->target = new Operations::MemoryReference(d->location, d->object->clone(d->location), mt, mo);

					return Result::ok();
				});

				fType->addJitCompiledMemberFunction(ts);
			}
		}


		fType->finaliseExternalDefinition();

		p = fType;

		return p;
	};

	c.addTemplateClass(ftc);
}

void InbuiltTypeLibraryBuilder::registerBuiltInFunctions()
{
	{
		NamespacedIdentifier iId("IndexType");

		NamespaceHandler::ScopedNamespaceSetter(c.getNamespaceHandler(), iId);

		TemplateObject cf({ iId.getChildId("wrapped"), {} });
		auto pId = cf.id.id.getChildId("ArrayType");

		cf.argList.add(TemplateParameter(pId, false, jit::TemplateParameter::Single));

		cf.functionArgs = [pId]()
		{
			Array<TypeInfo> l;
			l.add(TypeInfo(pId, true, true));
			l.add(TypeInfo(Types::ID::Integer));
			return l;
		};

		auto& compiler = c;

		cf.makeFunction = [&compiler, iId](const TemplateObject::ConstructData& cd)
		{
			if (!cd.expectTemplateParameterAmount(1))
				return;

			if (!cd.expectIsComplexType(0))
				return;

			auto f = new FunctionData();

			f->id = cd.id.id;
			f->returnType = TypeInfo(Types::ID::Dynamic);
			f->templateParameters.add(cd.tp[0]);
			f->addArgs("obj", TypeInfo(Types::ID::Dynamic));
			f->addArgs("index", TypeInfo(Types::ID::Integer));
			f->setDefaultParameter("index", VariableStorage(0));

			f->inliner = Inliner::createHighLevelInliner(f->id, [](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();

					if (auto i = d->args[1])
					{
						d->target = i->clone(d->location);

						return Result::ok();
					}
					else
						return Result::fail("Can't get init value for IndexType");
				});

			f->inliner->returnTypeFunction = [](InlineData* b)
			{
				auto rt = dynamic_cast<ReturnTypeInlineData*>(b);
				auto& handler = rt->object->currentCompiler->namespaceHandler;
				auto ct = b->templateParameters[0].type.getTypedIfComplexType<ComplexType>();

				if (ct == nullptr)
					return Result::fail("Can't deduce index type from parameter 1");

				if (auto st = dynamic_cast<StructType*>(ct))
				{
					auto t = handler.registerComplexTypeOrReturnExisting(new StructSubscriptIndexType(st, Identifier("wrapped")));
					rt->f.returnType = TypeInfo(t);
					return Result::ok();
				}

				SubTypeConstructData sd;
				sd.handler = &handler;
				sd.id = NamespacedIdentifier("wrapped");
				auto subType = ct->createSubType(&sd);
				subType = sd.handler->registerComplexTypeOrReturnExisting(subType);

				rt->f.returnType = TypeInfo(subType);

				if (!sd.r.wasOk())
					return sd.r;

				return Result::ok();
			};


			compiler.getInbuiltFunctionClass()->addFunction(f);

			
		};

		c.getNamespaceHandler().addTemplateFunction(cf);
	}


	{
		c.addConstant(NamespacedIdentifier("NumChannels"), numChannels);

		auto blockType = c.getNamespaceHandler().getComplexType(NamespacedIdentifier("block"));

		auto floatType = TypeInfo(Types::ID::Float);
		auto float2 = new SpanType(floatType, numChannels);

		ComplexType::Ptr channelType = new SpanType(TypeInfo(blockType, false, false), numChannels);
		ComplexType::Ptr frameType = new DynType(TypeInfo(c.registerExternalComplexType(float2), false, false));

		channelType->setAlias(NamespacedIdentifier("ChannelData"));
		frameType->setAlias(NamespacedIdentifier("FrameData"));

		c.registerExternalComplexType(channelType);
		c.registerExternalComplexType(frameType);
	}

	c.initInbuildFunctions();

	registerRangeFunctions();
}

void InbuiltTypeLibraryBuilder::registerRangeFunctions()
{
	FunctionClass::Ptr bf = c.getInbuiltFunctionClass();

	{
		auto getMathSymbol = [](const Identifier& id)
		{
			NamespacedIdentifier m("Math");
			return Symbol(m.getChildId(id), TypeInfo(Types::ID::Double));
		};

#define CLONED_ARG(x) d->args[x]->clone(d->location)


		// static constexpr T from0To1(T min, T max, T value) { return hmath::map(value, min, max); }
		bf->addFunction(createRangeFunction("from0To1", 3, Inliner::HighLevel, [getMathSymbol](InlineData* b)
		{
			SyntaxTreeInlineParser p(b, "{return Math.map($a3, $a1, $a2);}");
			return p.flush();

#if 0
			auto mCall = new Operations::FunctionCall(d->location, nullptr, getMathSymbol("map"), {});
			mCall->addArgument(CLONED_ARG(2)); mCall->addArgument(CLONED_ARG(0));
			mCall->addArgument(CLONED_ARG(1));
			d->target = mCall;
			return Result::ok();
#endif
		}));

		// static constexpr T to0To1(T min, T max, T value) { return (value - min) / (max - min); }
		bf->addFunction(createRangeFunction("to0To1", 3, Inliner::HighLevel, [getMathSymbol](InlineData* b)
		{
			SyntaxTreeInlineParser p(b, "{return ($a3 - $a1) / ($a2 - $a1);}");
			return p.flush();

#if 0
			auto d = b->toSyntaxTreeData();
			auto value = CLONED_ARG(2); auto min = CLONED_ARG(0);
			auto min2 = CLONED_ARG(0); auto max = CLONED_ARG(1);
			auto b1 =   new Operations::BinaryOp(d->location, value, min, JitTokens::minus);
			auto b2 =   new Operations::BinaryOp(d->location, max, min2, JitTokens::minus);
			d->target = new Operations::BinaryOp(d->location, b1, b2, JitTokens::divide);
			return Result::ok();
#endif
		}));

#if 0
		static constexpr T from0To1Skew(T min, T max, T skew, T value) { auto v = hmath::pow(value, skew); return from0To1(min, max, value); }
		static constexpr T to0To1Skew(T min, T max, T skew, T value) { return hmath::pow(to0To1(min, max, value), skew); }
		static constexpr T to0To1Step(T min, T max, T step, T value) { return to0To1(min, max, value - hmath::fmod(value, step)); }
		static constexpr T to0To1StepSkew(T min, T max, T step, T skew, T value) { return to0To1(min, max, skew, value - hmath::fmod(value, step)); }

		static constexpr T from0To1Step(T min, T max, T step, T value)
		{
			auto v = from0To1(min, max, value);
			return v - hmath::fmod(v, step);
		}
		static constexpr T from0To1StepSkew(T min, T max, T step, T skew, T value)
		{
			auto v = from0To1(min, max, skew, value);
			return v - hmath::fmod(v, step);
		}
#endif
	}
}

snex::jit::FunctionData* InbuiltTypeLibraryBuilder::createRangeFunction(const Identifier& id, int numArgs, Inliner::InlineType type, const Inliner::Func& inliner)
{
	auto f = new FunctionData();

	f->id = NamespacedIdentifier("ranges").getChildId(id);
	f->returnType = TypeInfo(Types::ID::Double);

	for (int i = 0; i < numArgs; i++)
	{
		Identifier id("a" + String(i + 1));
		f->addArgs(id, TypeInfo(Types::ID::Double));
	}

	f->inliner = Inliner::createFromType({}, type, inliner);

	NamespaceHandler::ScopedNamespaceSetter sns(c.getNamespaceHandler(), f->id.getParent());

	c.getNamespaceHandler().addSymbol(f->id, f->returnType, NamespaceHandler::Function, {});

	return f;
}

juce::Result InbuiltTypeLibraryBuilder::registerTypes()
{
	registerBuiltInFunctions();

	REGISTER_CPP_CLASS(c, sfloat);
	REGISTER_CPP_CLASS(c, sdouble);
	REGISTER_ORIGINAL_CPP_CLASS(c, OscProcessDataJit, OscProcessData);
	REGISTER_ORIGINAL_CPP_CLASS(c, PrepareSpecsJIT, PrepareSpecs);
	REGISTER_ORIGINAL_CPP_CLASS(c, EventWrapper, HiseEvent);
	REGISTER_ORIGINAL_CPP_CLASS(c, ExternalDataJIT, ExternalData);

	auto eventType = c.getComplexType(NamespacedIdentifier("HiseEvent"));
	auto eventBufferType = new DynType(TypeInfo(eventType));
	eventBufferType->setAlias(NamespacedIdentifier("HiseEventBuffer"));

	c.registerExternalComplexType(eventBufferType);
	createFrameProcessor();
	createProcessData(TypeInfo(eventBufferType));

	TemplateClassBuilder polyDataBuilder(c, NamespacedIdentifier("PolyData"));
	polyDataBuilder.addTypeTemplateParameter("T");
	polyDataBuilder.addIntTemplateParameter("NumVoices");
	polyDataBuilder.setInitialiseStructFunction([](const TemplateObject::ConstructData& cd, StructType* st)
	{
		auto dataType = cd.handler->registerComplexTypeOrReturnExisting(new SpanType(cd.tp[0].type, cd.tp[1].constant));

		st->addMember("data", TypeInfo(dataType));
		st->addMember("voiceIndex", TypeInfo(Types::ID::Pointer, true));
		st->setDefaultValue("voiceIndex", InitialiserList::makeSingleList(VariableStorage(nullptr, 0)));
	});

	polyDataBuilder.addFunction([](StructType* st)
	{
		FunctionData f;

		f.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::BeginIterator));
		f.returnType = st->getTemplateInstanceParameters()[0].type.withModifiers(false, true, false);
		
		f.inliner = Inliner::createAsmInliner({}, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();
			d->target->setCustomMemoryLocation(d->object->getMemoryLocationForReference(), d->object->isGlobalMemory());
			return Result::ok();
		});

		return f;
	});

	polyDataBuilder.addFunction([](StructType* st)
	{
		FunctionData f;

		f.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::SizeFunction));
		f.returnType = TypeInfo(Types::ID::Integer);

		f.inliner = Inliner::createAsmInliner({}, [](InlineData* b)
		{
			auto d = b->toAsmInlineData();
			auto pdType = d->object->getTypeInfo().getTypedComplexType<StructType>();
			d->target->setImmediateValue(pdType->getTemplateInstanceParameters()[1].constant);
			return Result::ok();
		});

		return f;
	});


	polyDataBuilder.flush();

	return Result::ok();
}

}
}